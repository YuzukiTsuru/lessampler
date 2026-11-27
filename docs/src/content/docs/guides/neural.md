---
title: "Neural backends"
description: "NHVSing, whole-song polish and feature-level assembly."
---

The optional neural path uses an ONNX vocoder. The default is NHVSing: 44.1 kHz, hop 256, 128-bin log-mel between 40 Hz and 16 kHz, plus continuous F0 and an unvoiced flag. NHVSing V3.3 is about 7.6 MB with 0.478 M parameters. The NHVSing code is MIT; its published weights are non-commercial and are not bundled.

Step-by-step build setup, ONNX Runtime deployment, model URLs and hashes are in [DNN deployment](neural-deployment.md). This page describes how the neural paths behave once they are configured.

A vocoder descriptor now carries the sample rate, hop/FFT size, mel range/base/scale, tensor names, F0/uv inputs and mel layout. `[neural] preset = "pc-nsf-hifigan"` selects the OpenVPI PC-NSF-HiFiGAN contract: 44.1 kHz, hop 512, 2048-point window, 128 bins, 40–16 kHz natural-log Slaney mel, `mel` + `f0` inputs and a `waveform` output. Its weights are CC BY-NC-SA 4.0 and are not bundled. Other fields can override the preset when a model's `vocoder.yaml` differs.

Build:

```bash
cmake --preset neural-gcc
cmake --build --preset neural-gcc -j
ctest --preset neural-gcc --output-on-failure
```

CMake downloads a pinned ONNX Runtime NuGet package into gitignored `lib/onnxruntime`. `ONNXRUNTIME_ROOT` overrides it. The model path is configured with `[neural] model_path`.

## Per-note backend

`backend = "neural"` keeps WORLD analysis and the UTAU time/pitch transform, renders the transformed model once, extracts mel from that preliminary waveform, derives F0/uv from the transformed WORLD track, and runs the model per note. It removes WORLD's buzzy excitation and phase artefacts but still starts a fresh model for every note.

The mel front end matches NHVSing's training preprocessing and was verified against librosa to float precision. ONNX Runtime is used instead of ncnn because the exported graphs rely on dynamic `Scan`, `CumSum`, and random-excitation operators that `onnx2ncnn` cannot import.

## PC-NSF-HiFiGAN

The OpenVPI PC-NSF-HiFiGAN vocoder uses a different feature contract. The `pc-nsf-hifigan` preset sets:

```toml
[neural]
preset = "pc-nsf-hifigan"
model_path = "/path/to/pc_nsf_hifigan_44.1k_hop512_128bin_2025.02.onnx"
threads = 4
```

The preset uses hop 512, natural-log Slaney mel, `mel` + `f0` inputs and no uv input. Mel is time-major; the model adapter reads the ONNX input shapes and accepts a 2-D `[1,T]` or 3-D `[1,1,T]` F0 tensor. Mel RMS normalization is disabled because the PC-NSF mel comes from an acoustic model, not a normalized waveform.

On the 245 s reference song, PC-NSF whole-song polish scores about the same as NHVSing (3.18 full, 3.60 on the 35–80 s high-note excerpt vs 3.69). The per-note neural path improves the high excerpt from 2.99 to 3.26 but scores lower overall (2.82). PC-NSF is an alternative, not the default; A/B it on the target voicebank.

## RMVPE F0 estimator

The optional neural F0 estimator runs RMVPE instead of WORLD DIO/Harvest. It is available in neural builds (ONNX Runtime is already linked) and needs its own model:

```toml
[f0]
estimator = "neural"        # or "auto" with backend = "neural"
neural_model_path = "/path/to/rmvpe.onnx"
neural_threads = 4
neural_threshold = 0.03
neural_floor = 40
neural_ceil = 1200
```

The adapter accepts two common ONNX contracts:

- `audio` / `waveform` input and a 360-bin `salience` output (`[1,T,360]` or `[1,360,T]`). Salience is decoded with the RMVPE cents mapping, a local weighted average around the peak, and `neural_threshold` for voicing.
- `waveform` + `threshold` inputs and `f0` + `uv` outputs, as used by OpenUtau's RMVPE package. `f0` is Hz and `uv` marks unvoiced frames.

The OpenUtau `rmvpe` dependency package (`230917`, `rmvpe.oudep`) is the tested source for the second contract; extract its `rmvpe.onnx` and point `[f0] neural_model_path` at it. The download URL and hashes are in [DNN deployment](neural-deployment.md). The weights are not bundled and remain subject to their own license.

Input is resampled to 16 kHz; RMVPE frames are 10 ms and are resampled to `[audio_model] frame_period`. The default `neural_ceil = 1200` matches the current WORLD transform average window; RMVPE itself supports up to 2000 Hz. Weights are not bundled. `estimator = "auto"` falls back to Harvest when the model path is missing; an explicit `estimator = "neural"` throws. `--polish` and `--feature-plan` also follow `[audio_model] backend`, so set `estimator = "neural"` explicitly if you want RMVPE on a WORLD-configured song.

## Per-note frontends

`[neural] frontend` controls what the per-note vocoder sees:

| Value | Feature source |
| --- | --- |
| `world` (default) | Time/pitch-transformed WORLD model, rendered to a preliminary waveform, then mel extraction |
| `world-spectral` | Mel computed directly from the transformed WORLD power spectrogram; experimental |
| `source-mel` | Mel extracted from the original voicebank sample at `source_hop` (default 128), then time-mapped and driven by the UTAU pitch track |
| `source-wave` | Source waveform mapped through the UTAU time controls before mel extraction; preserves source phase/transients but not target pitch in the preliminary waveform |

The source-mel frontend is the hifisampler-style source-constrained idea: the voicebank sample stays the articulation source and the vocoder receives the target F0 from UTAU. In the current benchmark it did not beat the default `world` frontend: on the 45 s reference excerpt, NHVSing scored 2.96 versus 3.40 and PC-NSF 3.13 versus 3.35, with higher F0 MAE and more seam dips. `source-wave` scored 2.88/3.05, and `world-spectral` was worse still. All three remain experimental and are not the default.

## Whole-song polish

`lessampler --polish <mix.wav> <out.wav>` resynthesizes an assembled mix as one timeline: F0 from the selected estimator, 128-bin mel from the mix, one NHVSing pass, RMS-matched output. On the 245 s reference song this is 42,198 frames, about 60 s and 5.5 GB peak memory. It removes per-note phase resets but regenerates the timbre.

## Feature-level assembly

`lessampler --feature-plan plan.toml` avoids double vocoding. It reuses the wavtool plan, slices each note WAV by `skip_ms`/`length_ms`, extracts raw mel and F0, applies the UST envelope in mel/F0 space, and sums the notes on one global timeline. A single global level shift maps the assembled mix to the model's target RMS while preserving per-note dynamics. NHVSing runs once over the whole song; the output is level-matched to a reference mix built from the same note buffers.

Feature-level assembly is meant for WORLD or LLSM note renders. Feeding it per-note neural renders would run the vocoder twice. `scripts/render_ust.py --feature` selects it instead of wavtool assembly.

Per-note voice color is already baked into the note WAVs, but both `--polish` and `--feature-plan` re-estimate F0 and mel from those WAVs, so a clear/cry/growl color can be weakened. The wavtool path keeps the full color; see [Expression and voice color](expression.md).

## Settings

```toml
[audio_model]
backend = "neural"

[f0]
estimator = "auto"              # or "harvest", "dio", "llsm", "neural"
neural_model_path = ""          # RMVPE; separate from [neural] model_path
neural_threads = 0
neural_threshold = 0.03
neural_floor = 40
neural_ceil = 1200

[neural]
preset = "nhv"                  # or "pc-nsf-hifigan"
model_path = "/path/to/nhv_v3_3.onnx"
threads = 4
target_rms = 0.111
sample_rate = 44100
hop_size = 256
fft_size = 2048
mel_bins = 128
mel_fmin = 40
mel_fmax = 16000
mel_base = "e"                  # or "10"
mel_scale = "slaney"            # or "htk"
mel_layout = "time-major"       # or "bin-major"
has_f0 = true
has_uv = true
normalize_mel = true
pitch_controllable = false
f0_min = 40
f0_max = 1200
mel_input = "mel"
f0_input = "f0"
uv_input = "uv"
waveform_output = "waveform"
```

Preset names are case-insensitive and stored canonical. Fields after `preset` override it, so a model whose `vocoder.yaml` differs can be described without code changes. `threads = 0` lets ONNX Runtime choose. Input rates other than the model rate are resampled for features and back to the source rate. The F0 track is sampled by time, so hop-512 vocoders do not reuse the WORLD frame index. `[f0] neural_model_path` is independent of `[neural] model_path`; RMVPE weights are not bundled.
