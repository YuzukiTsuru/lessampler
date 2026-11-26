---
title: "Processing pipeline"
description: "One note, stage by stage: argv and UST parsing, source and cache, F0 and modeling, pitch and time modification, single-frame synthesis and output."
---

This page follows one note through the engine in execution order: arguments first, then the source and cache, then F0 and modeling, then pitch and time modification, then single-frame synthesis, then output. The backend boundary is `VocoderBackend::Prepare` for the cacheable analysis and `VocoderBackend::Synthesize` for the per-note render. The engine and hosts never inspect backend data.

```mermaid
flowchart TD
    A["argv, UST, oto and lessampler.less"] --> B["RenderParams"]
    B --> C["SampleSource and cache key"]
    C --> P["source preprocessing"]
    P --> D["selected F0 estimator"]
    D --> E["WORLD, LLSM or neural modeling"]
    E --> F["analysis cache"]
    F --> G["pitch and time modification"]
    G --> H["single-frame synthesis"]
    H --> I["growl, de-click, AutoAMP and WAV"]
```

## Steps

| Step | Page |
| --- | --- |
| 1 | [Arguments in](arguments.md) |
| 2 | [Source and cache](source.md) |
| 2.5 | [Source preprocessing](preprocess.md) |
| 3 | [Modeling](modeling.md) |
| F0 | [F0 estimation](f0.md) |
| 4 | [Pitch and time](transform.md) |
| 5 | [Single-frame synthesis](synthesis.md) |
| 6 | [Output](output.md) |
