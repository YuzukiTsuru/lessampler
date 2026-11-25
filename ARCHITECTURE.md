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
implements the concatenation side. Both are edge adapters; neither is used by
the engine or by analysis/synthesis code.

## Source tree

```
src/
  App/                 CLI entry point and command dispatch
  Audio/               WAV I/O, lazy SampleSource, AutoAMP, UST envelope
  Config/              Config value type, loader, parser, voicebank resolver
  Core/                Matrix, logging, timing, exceptions, fingerprints, params
  Engine/              RenderEngine, analysis LRU, voicebank modeler
  Host/UTAU/           UTAU parameter parser, Shine, libUTAU binding
  Host/Wavtool/        wavtool protocol, plan parser, note assembly
  Vocoder/             vocoder interface and implementations
    Common/            F0 estimation shared by WORLD and LLSM
    World/             WORLD model, analyzer, transform, synthesizer, caches
    Neural/            ONNX vocoder, mel front end, polish, feature assembly
    Llsm/              LLSM backend, layer-1 cache, compatibility header
```

## Engine flow

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

The backend contract separates the cacheable and per-note operations:

```cpp
AnalysisPtr Prepare(const AnalysisRequest &);       // cacheable per sample
RenderResult Synthesize(const SynthesisRequest &);  // per note
```

`Analysis` is an opaque shared handle. The engine never inspects
backend-specific data, so adding a backend does not change the engine.

## Assembly and neural paths

`scripts/render_ust.py` computes the UTAU placement, invokes the resampler per
note, and either hands the buffers to `lesswavtool` or to feature-level neural
assembly. The wavtool applies UST envelopes and overlap placement; the neural
path can resynthesize the finished mix (`--polish`) or assemble per-note
mel/F0 features and run the vocoder once (`--feature-plan`). See
[docs/rendering.md](docs/rendering.md) and [docs/neural.md](docs/neural.md).

## Details

Cache formats, threading rules, the backend extension checklist, and the GPU
note live in [docs/architecture-details.md](docs/architecture-details.md).
