# Architecture

lessampler is split into layers with a single dependency direction. A layer
may depend on the layers below it, never on the layers above it.

```
App
 ├── Engine
 │    ├── Vocoder
 │    │    ├── Vocoder/Common
 │    │    ├── Vocoder/World
 │    │    └── Vocoder/Llsm (optional)
 │    ├── Config
 │    └── Audio
 ├── Host/UTAU
 └── Platform

Vocoder  ──> Config, Audio, Core
Config   ──> Core
Audio    ──> Core
Core     ──> C++ standard library
```

`Host/UTAU` and `Platform` are adapters at the edge. They are not used by the
engine or by any analysis/synthesis code.

## Source tree

```
src/
  App/                 CLI entry point and Application command dispatch
  Audio/               WAV I/O, lazy SampleSource, AutoAMP
  Config/              Config value type, loader, parser, voicebank resolver
  Core/                Matrix, logging, timing, exceptions, fingerprints, params
  Engine/              RenderEngine, analysis LRU, voicebank modeler
  Host/UTAU/           UTAU parameter parser, Shine, libUTAU binding
  Platform/            desktop dialogs
  Vocoder/             vocoder interface and implementations
    Common/            F0 estimation shared by WORLD and LLSM
    World/             WORLD model, analyzer, transform, synthesizer, caches
    Llsm/              LLSM backend, layer-1 cache, compatibility header
```

Tests mirror the same vocabulary: `WorldModelTest`, `WorldModelCacheTest`,
`WorldAnalyzerTest`, `WorldTransformTest`, `WorldSynthesizerTest`,
`VoicebankModelerTest`, `RenderEngineTest`, and `EndToEndTest`.

## Engine flow

`RenderEngine` is the only component that knows how to turn a sample plus a
note into audio:

```
SampleSource
   │  (header now, PCM lazily)
   ▼
ConfigResolver ──> effective Config (global + lessvoice.fex)
   ▼
VocoderBackend::Prepare
   │  disk cache hit  -> Analysis
   │  disk cache miss -> decode PCM, analyze, save cache
   ▼
in-memory LRU of AnalysisPtr  (note-independent; reused by later notes)
   ▼
VocoderBackend::Synthesize (note-dependent)
   ▼
AutoAMP
   ▼
RenderResult
```

The `VocoderBackend` contract has two separate operations:

```cpp
AnalysisPtr Prepare(const AnalysisRequest &);   // cacheable per sample
RenderResult Synthesize(const SynthesisRequest &); // per note
```

`Analysis` is an opaque, shared handle owned by the backend. WORLD wraps a
`WorldModel`; LLSM wraps a layer-1 `llsm_chunk` plus its F0 track. The engine
never inspects backend-specific data, so adding a backend does not change the
engine.

The in-memory analysis cache is keyed by:

- backend kind and backend `analysis_version()`
- effective config fingerprint
- canonical source path
- source size/mtime fingerprint

One engine can therefore render a whole session of notes without touching
disk again after the first note for each sample. The CLI currently constructs
one engine per process because of the UTAU protocol; a persistent batch/server
host can keep one engine alive and reuse the LRU.

## Caches

All caches are sidecars next to the source WAV:

| File | Owner | Contents |
| --- | --- | --- |
| `.lessaudio` | `Vocoder/World/WorldModelCache` | WORLD f0, spectrogram, aperiodicity |
| `.lessllsm` | `Vocoder/Llsm/LlsmCache` | layer-1 F0, RD, VSPHSE, VTMAGN, noise model |

Both formats are little-endian, versioned, and store a config fingerprint and
the source size/mtime. `Core/SourceFingerprint` is the shared implementation of
the latter. Cache writes are best-effort where rendering should continue on a
read-only or full disk; batch modeling still reports real analysis errors.

## Threading

- WORLD D4C uses a fixed eight-block frame partition. The partition and the
  per-block random state do not depend on the worker count, so analysis output
  is reproducible. The `max_workers` budget only controls how many of those
  blocks run concurrently.
- `VoicebankModeler` parallelizes across files and passes a per-file worker
  budget so the total thread count stays near the hardware concurrency instead
  of multiplying outer workers by inner D4C workers.
- `ConfigResolver` and the analysis LRU are internally synchronized; backends
  are currently used from one render at a time per engine.

## Adding a vocoder

1. Add a `VocoderKind` value and config parser name.
2. Implement `Analysis` plus `VocoderBackend::Prepare` / `Synthesize` under
   `src/Vocoder/<Name>/`.
3. Put any persistent analysis format under the same directory and key it with
   the config and source fingerprints.
4. Link the implementation into the `Vocoder` target and extend the factory.
5. Add engine-level coverage. No `Engine`, `App`, or `Host` code should need to
   include backend-specific headers.

## GPU note

The UTAU host starts a fresh process per note. Vulkan context creation alone
measured roughly 0.7 s on the reference machine, while warm rendering is tens of
milliseconds. GPU acceleration would require a persistent engine/batch host
first; the current architecture already exposes that layer through
`RenderEngine`.
