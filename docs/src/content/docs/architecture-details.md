---
title: "Architecture details"
description: "Model caches, threading and adding a vocoder."
---

## Caches

All caches are sidecars next to the source WAV. Both formats are little-endian, versioned, and store a config fingerprint and the source size/mtime.

| File | Owner | Contents |
| --- | --- | --- |
| `.lessaudio` | `Vocoder/World/WorldModelCache` | WORLD F0, spectrogram, aperiodicity; format v2 |
| `.lessllsm` | `Vocoder/Llsm/LlsmCache` | layer-1 F0, RD, VSPHSE, VTMAGN, noise model; format v1 |

`.lessaudio` v2:

```
magic "LSAM" + format_version uint32 = 2
config fingerprint string
source WAV size uint64 + mtime int64
x_length uint64 + fs int32 + frame_period double + w_length uint32 + fft_size uint32
f0_length uint64 + f0 doubles
spectrogram frame count + per-frame row length + doubles
aperiodicity frame count + per-frame row length + doubles
trailer "END2"
```

`.lessllsm` v1 stores the layer-1 chunk header, F0/RD per frame, harmonic phase/magnitude envelopes, noise PSD/channel envelopes, and a `"L1EN"` trailer. Readers validate magic, version, fingerprint, source fingerprint, dimensions, row widths, harmonic counts, and truncation before allocating frames. Cache writes use a temporary file and rename.

## Threading

- WORLD D4C uses a fixed eight-block frame partition. The partition and per-block random state do not depend on worker count, so analysis output is reproducible; `max_workers` only controls concurrency.
- `VoicebankModeler` parallelizes across files and passes a per-file worker budget so the total thread count stays near hardware concurrency.
- `ConfigResolver` and the analysis LRU are internally synchronized; backends are used from one render at a time per engine.
- The neural backend loads one ONNX Runtime session per model path for the process lifetime. ONNX Runtime `Run` is thread-safe; the mel extractor is immutable after construction.

## Adding a vocoder

1. Add a `VocoderKind` value and config parser name.
2. Implement `Analysis` plus `VocoderBackend::Prepare` / `Synthesize` under `src/Vocoder/<Name>/`.
3. Put any persistent analysis format in the same directory and key it with the config and source fingerprints.
4. Link the implementation into the `Vocoder` target and extend the factory.
5. Add engine-level coverage. No `Engine`, `App`, or `Host` code should include backend-specific headers.

## GPU note

UTAU starts a fresh process per note. Vulkan context creation alone measured about 0.7 s on the reference machine, while warm rendering is tens of milliseconds. GPU acceleration would require a persistent engine/batch host first; `RenderEngine` already exposes that layer.
