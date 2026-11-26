---
title: "Architecture details"
description: "Model caches, threading and adding a vocoder."
---

## Selection factories

Algorithm selection is split into small per-module factories. They share a shape, not a generic registry: a kind enum with case-insensitive parsing, a `Create` function, and an `Available` list.

| Axis | Config key | Kind | Factory | Implementations |
| --- | --- | --- | --- | --- |
| Vocoder backend | `[audio_model] backend` | `VocoderKind` | `CreateVocoderBackend` | WORLD, LLSM, neural |
| F0 estimator | `[f0] estimator` | `F0EstimatorKind` | `CreateF0Estimator` | Harvest, DIO, LLSM refine, RMVPE |
| Neural vocoder | `[neural] preset` | `NeuralVocoderPresetKind` | `CreateNeuralVocoderPreset` | NHVSing, PC-NSF-HiFiGAN |
| Master preset | `[master] preset` | `MasterPresetKind` | `CreateMasterOptions` | none, natural, warm, power |

F0 is the only axis with `auto`; it resolves against the selected backend and falls back to Harvest when the optional estimator is unavailable. The other three axes are explicit. Runtime-capability factories throw `parameter_error` for an unavailable implementation; value preset factories keep `std::invalid_argument`, and the config/plan parsers convert it to a field-specific error.

F0 targets keep the factory below the backends, which call it during `Prepare`:

```
VocoderF0Api      F0Track, F0Estimator, shared resampler, RMVPE decode
VocoderF0World    Harvest / DIO + StoneMask
VocoderF0Llsm     Harvest initial + llsm_refine_f0   (optional)
VocoderOnnx       shared ONNX Runtime environment    (optional)
VocoderF0Neural   RMVPE + ONNX                       (optional)
VocoderCommon     factory + availability
VocoderWorld / VocoderLlsm / VocoderNeural
Vocoder           backend factory
```

The optional F0 targets never link `VocoderCommon` back; `LlsmF0Estimator` constructs `WorldF0Estimator` directly, and `RmvpeModel` does not include the vocoder model. This keeps the target graph acyclic. The resolved estimator and its parameters are part of `Config::fingerprint()`; see [Source and cache](../processing/source.md).

## Expression layers

Voice color is not a factory axis. `RenderParams.style` carries additive clear/cry/growl weights; WORLD and LLSM apply the spectral part per output frame, and `RenderEngine` applies `GrowlShaper` after any backend. Style is synthesis-only and stays out of `Config::fingerprint()`, so changing a color or a per-note `C>Y` morph reuses the analysis cache.

Automatic breath, vibrato, and portamento live in `scripts/render_ust.py`, not in the resampler. They need the song context that a per-note process does not have, and they only fill gaps left by explicit UST data.

## Caches

All caches are sidecars next to the source WAV. Both formats are little-endian, versioned, and store a config fingerprint and the source size/mtime. An active `[preprocess]` configuration is part of that fingerprint; with preprocessing disabled the fingerprint and stored analyses are unchanged.

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

## Adding an algorithm

For a vocoder backend:

1. Add a `VocoderKind` value and config parser name.
2. Implement `Analysis` plus `VocoderBackend::Prepare` / `Synthesize` under `src/Vocoder/<Name>/`.
3. Put any persistent analysis format in the same directory and key it with the config and source fingerprints.
4. Link the implementation into the `Vocoder` target and extend the factory.
5. Add engine-level coverage. No `Engine`, `App`, or `Host` code should include backend-specific headers.

For an F0 estimator:

1. Add an `F0EstimatorKind` value and parser name.
2. Implement `F0Estimator::Estimate` as an optional target below `VocoderCommon`; do not link the factory back.
3. Add the target to `VocoderCommon`, define its build guard on the factory target, and add it to `AvailableF0Estimators`.
4. Add the resolved kind, parameters, and any model identity to `Config::fingerprint()`.
5. Add `F0EstimatorTest` coverage and update the auto-resolution table.

For a value preset (neural vocoder or master):

1. Add a kind parser and a `Create...` factory that returns the value product.
2. Keep the old `Preset` method as a forwarding shim.
3. Canonicalize the config value at parse time; keep the persisted string form.
4. Add the factory and round-trip tests to `PresetFactoryTest`.

## GPU note

UTAU starts a fresh process per note. Vulkan context creation alone measured about 0.7 s on the reference machine, while warm rendering is tens of milliseconds. GPU acceleration would require a persistent engine/batch host first; `RenderEngine` already exposes that layer.
