---
title: "DNN deployment"
description: "Build the neural backend, fetch ONNX Runtime, download model weights, and point lessampler at them."
---

The neural path is optional. This page covers the build dependency, the three supported model families, their download locations and hashes, and the configuration that selects them. lessampler does not bundle model weights.

## Build

```bash
cmake --preset neural-gcc
cmake --build --preset neural-gcc -j
ctest --preset neural-gcc --output-on-failure
```

`max-gcc` enables both LLSM and neural:

```bash
cmake --preset max-gcc
cmake --build --preset max-gcc -j
ctest --preset max-gcc --output-on-failure
```

The executable is `build/<preset>/bin/lessampler`. A binary without the neural backend rejects `backend = "neural"`, `--polish`, and `--feature-plan` with a `parameter_error`.

## ONNX Runtime

With `LESSAMPLER_ENABLE_NEURAL=ON`, `cmake/OnnxRuntime.cmake` downloads the pinned CPU package and installs it into `lib/onnxruntime`:

| Item | Value |
| --- | --- |
| Version | 1.32.0 |
| Package | `https://api.nuget.org/v3-flatcontainer/microsoft.ml.onnxruntime/1.32.0/microsoft.ml.onnxruntime.1.32.0.nupkg` |
| SHA-256 | `a9150bc2992cbbb6722070da9910e1ffa989a8bfc67bdd5bb8e745ca755641db` |

The package is CPU-only. GPU execution providers are not used: UTAU starts a new resampler process per note, and context creation costs more than a warm note render.

For an offline build, place the `.nupkg` at `lib/onnxruntime/onnxruntime-1.32.0.nupkg` before configuring. To use an existing ONNX Runtime installation instead:

```bash
cmake --preset neural-gcc -DONNXRUNTIME_ROOT=/opt/onnxruntime
```

`ONNXRUNTIME_ROOT` must contain `include/onnxruntime_cxx_api.h` and the library under `lib`, `lib64`, or `runtimes/<rid>/native`. `LESSAMPLER_ONNXRUNTIME_ROOT` changes where the managed installation is kept.

On Windows, `onnxruntime.dll` is copied next to `lessampler.exe` when found. If you package the binary, ship the DLL with it. On Linux and macOS the managed installation creates the SONAME symlink (`libonnxruntime.so.1`, `libonnxruntime.1.dylib`); create the same link for a manual install if the loader cannot find the library.

## Models

| Model | File | Download | SHA-256 (ONNX) | License |
| --- | --- | --- | --- | --- |
| NHVSing V3.3 | `nhv_v3_3.onnx` | [raw GitHub](https://raw.githubusercontent.com/wavtechyukky/NHVSing/main/exported_models/v3_3/nhv_v3_3.onnx) | `37d22b928d041e42a017e07f652e59ce27c962f9c5128ef8ebaf4bf982305973` | NHVSing code MIT; published weights non-commercial |
| PC-NSF-HiFiGAN 2025.02 | `pc_nsf_hifigan_44.1k_hop512_128bin_2025.02.onnx` | [release zip](https://github.com/openvpi/vocoders/releases/download/pc-nsf-hifigan-44.1k-hop512-128bin-2025.02/pc_nsf_hifigan_44.1k_hop512_128bin_2025.02.zip) | `b297c0c790dbcfe6b2123d87cc20bf8718969f91ffa3170a198c8fb53686155d` | CC BY-NC-SA 4.0 |
| RMVPE | `rmvpe.onnx` | [OpenUtau package](https://github.com/yqzhishen/RMVPE/releases/download/230917/rmvpe.oudep) | `bb00595b185fe7cf557a349356f6583e84aed18eb85edb69be1dfa513250f30f` | OpenUtau RMVPE package; check its model terms |

The NHVSing and PC-NSF weights are non-commercial; the RMVPE package remains subject to its own terms. The ONNX Runtime package is MIT-licensed; its license files are copied into `lib/onnxruntime/licenses`.

The PC-NSF and RMVPE hashes above are for the extracted ONNX file, not the archive. The RMVPE `.oudep` package itself has SHA-256 `186194e1fdfbf7c19366b90d79642caef0f785f12f56b2ec54994a364c494682` and is a ZIP archive. The NHVSing URL follows upstream `main`; the hash pins the file this project tested.

### Download commands

```bash
mkdir -p models

# NHVSing V3.3, about 7.6 MB
curl -L --fail -o models/nhv_v3_3.onnx \
  https://raw.githubusercontent.com/wavtechyukky/NHVSing/main/exported_models/v3_3/nhv_v3_3.onnx
sha256sum models/nhv_v3_3.onnx

# PC-NSF-HiFiGAN, about 54 MiB after extraction
curl -L --fail -o /tmp/pc-nsf.zip \
  https://github.com/openvpi/vocoders/releases/download/pc-nsf-hifigan-44.1k-hop512-128bin-2025.02/pc_nsf_hifigan_44.1k_hop512_128bin_2025.02.zip
unzip -j /tmp/pc-nsf.zip '*.onnx' -d models
sha256sum models/pc_nsf_hifigan_44.1k_hop512_128bin_2025.02.onnx

# RMVPE, about 345 MiB after extraction
curl -L --fail -o /tmp/rmvpe.oudep \
  https://github.com/yqzhishen/RMVPE/releases/download/230917/rmvpe.oudep
unzip -j /tmp/rmvpe.oudep rmvpe.onnx -d models
sha256sum models/rmvpe.onnx
```

If `unzip` reports that no `.onnx` matched, list the archive with `unzip -l` and adjust the pattern. Model paths in `lessampler.less` should be absolute; `scripts/lessampler_config.py` resolves them for you.

## Configuration

### NHVSing per note

```toml
[audio_model]
backend = "neural"

[f0]
estimator = "harvest"

[neural]
preset = "nhv"
model_path = "/absolute/path/models/nhv_v3_3.onnx"
threads = 0
```

### PC-NSF-HiFiGAN

```toml
[neural]
preset = "pc-nsf-hifigan"
model_path = "/absolute/path/models/pc_nsf_hifigan_44.1k_hop512_128bin_2025.02.onnx"
threads = 0
```

The preset selects 44.1 kHz, hop 512, 2048-point FFT, 128 mel bins, 40 Hz–16 kHz Slaney mel with natural log, `mel` + `f0` inputs and a `waveform` output. A model whose `vocoder.yaml` differs can override the preset fields; see [Configuration](../getting-started/configuration.md).

### RMVPE F0

RMVPE is independent of the vocoder model:

```toml
[f0]
estimator = "neural"
neural_model_path = "/absolute/path/models/rmvpe.onnx"
neural_threads = 0
neural_threshold = 0.03
neural_floor = 40
neural_ceil = 1200
```

With the OpenUtau package, the adapter uses the `waveform` + `threshold` inputs and `f0` + `uv` outputs. The other supported contract is an `audio`/`waveform` input with a 360-bin `salience` output. `estimator = "auto"` uses RMVPE for `backend = "neural"` when the model is available and falls back to Harvest otherwise; an explicit `estimator = "neural"` without the model throws.

### Whole-song and feature paths

`lessampler --polish mix.wav out.wav` and `lessampler --feature-plan plan.toml` also use `[neural] model_path`. They do not require `backend = "neural"`: the usual setup is a WORLD or LLSM note pass followed by one neural pass. `scripts/render_ust.py --polish` and `--feature` select them.

## Runtime notes

- `threads = 0` lets ONNX Runtime choose. When modeling a folder in parallel, set `neural_threads = 1` for RMVPE to avoid oversubscription.
- A UTAU host starts a new process for each note, so the ONNX session and model load are paid per note. The per-note neural path is a quality mode, not a speed mode.
- Whole-song polish keeps the whole timeline and mel in memory; a long song needs chunking. Feature assembly is cheaper because it runs the vocoder once over the assembled feature timeline.
- The neural backend reuses `.lessaudio` and writes no cache of its own. Changing `[neural]` settings does not rebuild the WORLD analysis; only F0-estimator settings are part of its fingerprint.

## Troubleshooting

| Symptom | Check |
| --- | --- |
| `parameter_error` when selecting `neural` | The binary was built with `LESSAMPLER_ENABLE_NEURAL=ON` |
| ONNX Runtime download fails | Network access to nuget.org, or place the `.nupkg` in `lib/onnxruntime` for an offline build |
| `onnxruntime.dll` not found at runtime | Copy the DLL next to `lessampler.exe`, or add its directory to `PATH` |
| Model file not found | Use an absolute `[neural] model_path`; the configurator checks existence and warns |
| Input name or shape error | Match `preset` to the model contract, or override the mel/F0/uv input names and shape fields |
| RMVPE loads slowly or uses too much memory | The tested OpenUtau package is large; it is loaded only when the F0 estimator is neural |
| Neural output is distorted | Check the model's sample rate/hop/mel contract; use the configurator preset before overriding fields |

Related: [Neural backends](../guides/neural.md), [Choosing a synthesis path](../processing/paths.md), [Configuration](../getting-started/configuration.md).
