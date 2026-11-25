# Architecture

lessampler is split into layers with a single dependency direction. A layer
may depend on the layers below it, never on the layers above it.

```
App
 ├── Engine
 │    ├── Vocoder
 │    │    ├── Vocoder/Common
 │    │    ├── Vocoder/World
 │    │    ├── Vocoder/Neural (optional, ONNX Runtime)
 │    │    └── Vocoder/Llsm (optional)
 │    ├── Config
 │    └── Audio
 ├── Host/UTAU
 └── Host/Wavtool

Vocoder  ──> Config, Audio, Core
Config   ──> Core
Audio    ──> Core
Wavtool  ──> Audio, Core
Core     ──> C++ standard library
```

`Host/UTAU` implements the resampler side of the UTAU protocol; `Host/Wavtool`
implements the concatenation side. Both are adapters at the edge. Neither is
used by the engine or by any analysis/synthesis code.

## Source tree

```
src/
  App/                 CLI entry point and Application command dispatch
  Audio/               WAV I/O, lazy SampleSource, AutoAMP
  Config/              Config value type, loader, parser, voicebank resolver
  Core/                Matrix, logging, timing, exceptions, fingerprints, params
  Engine/              RenderEngine, analysis LRU, voicebank modeler
  Host/UTAU/           UTAU parameter parser, Shine, libUTAU binding
  Host/Wavtool/        UTAU envelope parser, note assembly, wavtool CLI
  Vocoder/             vocoder interface and implementations
    Common/            F0 estimation shared by WORLD and LLSM
    World/             WORLD model, analyzer, transform, synthesizer, caches
    Neural/            ONNX neural vocoder, mel front end, model cache
    Llsm/              LLSM backend, layer-1 cache, compatibility header
```

Tests mirror the same vocabulary: `WorldModelTest`, `WorldModelCacheTest`,
`WorldAnalyzerTest`, `WorldTransformTest`, `WorldSynthesizerTest`,
`VoicebankModelerTest`, `RenderEngineTest`, `EnvelopeTest`,
`WavAssemblyTest`, `WavtoolProtocolTest`, `WavtoolCliTest`, and `EndToEndTest`.

## Engine flow

`RenderEngine` is the only component that knows how to turn a sample plus a
note into audio:

```
SampleSource
   │  (header now, PCM lazily)
   ▼
ConfigResolver ──> effective Config (global + lessvoice.less)
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

## Neural backend

`Vocoder/Neural` is an optional backend built with `LESSAMPLER_ENABLE_NEURAL`.
It reuses the WORLD analysis store and `WorldTransform`, renders the transformed
model once to obtain a waveform, and derives NHVSing's 128-bin log-mel features
from that waveform. The ONNX graph then produces the final note:

```
WorldModel (disk cache) -> WorldTransform -> preliminary waveform
                        -> MelSpectrogram -> NHVSing ONNX -> output
```

The mel front end matches NHVSing's training preprocessing (44.1 kHz, hop 256,
2048-point STFT, Slaney-normalized 128-bin mel, natural log); its output was
verified against librosa to float precision. F0 comes from the transformed
WORLD track; voicing is derived from low-band aperiodicity because WORLD's
transformed F0 is always positive.

ONNX Runtime is managed by `cmake/OnnxRuntime.cmake`: when the backend is
enabled, CMake downloads a pinned prebuilt NuGet package into
`lib/onnxruntime` and exposes the include directory, library, and Windows DLL.
No model weights are stored in the repository, and the neural backend never
writes a cache format of its own; it reuses `.lessaudio`.

ONNX Runtime rather than ncnn is deliberate. The exported singing vocoders
(NHVSing and OpenVPI NSF-HiFiGAN) use dynamic `Scan`/`CumSum`, random excitation,
and shape operators that `onnx2ncnn` cannot import. A ncnn implementation would
need a feedforward model or a native C++ port of the excitation/filter DSP.

## Wavtool assembly

`Host/Wavtool` implements the UTAU concatenation semantics that the resampler
cannot perform in a single-note process. The UST host computes, for each note,
the effective preutterance `P` and overlap `O` (oto values, UST overrides, ACPT
short-note adjustment, and velocity pull), then asks lessampler for
`required = max(L + STP, fixed / velocity_factor)` rounded up on the UTAU 50 ms
grid, where `L = D + P - P_next + O_next`.

The rendered buffer starts at the oto offset; its preutterance point is at
`STP + P` output milliseconds. `lesswavtool` skips `STP`, applies the UST
envelope over the remaining segment, and mixes at `note_start - P`. In the
classic protocol it seeks back by the overlap from the current output end; in
plan mode it places segments on an absolute timeline. The envelope is a
piecewise-linear gain:

```
0 -> p1 -> p1+p2 -> p1+p2+p5 -> L-p4-p3 -> L-p4 -> L
0 -> v1 -> v2     -> v5        -> v3       -> v4    -> 0
```

`%` in a UST envelope is the overlap placeholder, not a gain. Mixing accumulates
in `double` and clamps only when writing 16-bit PCM. The wavtool does not
phase-align segments; the overlap crossfade is the classic behaviour.

## Neural polish

`lessampler --polish <mix.wav> <out.wav>` is the optional song-level stage. It
uses `Vocoder/Neural/NeuralPolisher` to resample the assembled mix to 44.1 kHz,
estimate F0 with the shared DIO/Harvest + StoneMask front end, extract the
NHVSing 128-bin log-mel features, and run one model pass over the whole
timeline. The output is RMS-matched to the mix, resampled back to the voicebank
rate, and trimmed to the input length.

The per-note neural backend still resets model state for every note. The polish
stage exists to remove that limitation: the model sees the whole phrase, so a
seam between two separately rendered notes is no longer a model boundary. It
re-synthesizes timbre, so it is opt-in. `backend::NeuralPolishAvailable()`
answers whether the build has it; `PolishWithNeural` throws when the model path
is empty or missing. `scripts/render_ust.py --polish` calls it after
`lesswavtool` and keeps the raw mix at `<output>-mix.wav`.

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
- The neural backend loads one ONNX Runtime session per model path and caches
  it for the process lifetime. ONNX Runtime `Run` calls are thread-safe; the
  mel extractor is immutable after construction.

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
