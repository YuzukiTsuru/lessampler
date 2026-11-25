---
title: "Neural backends"
description: "NHVSing, whole-song polish and feature-level assembly."
---

The optional neural path uses an NHVSing-compatible ONNX vocoder: 44.1 kHz, hop 256, 128-bin log-mel between 40 Hz and 16 kHz, plus continuous F0 and an unvoiced flag. NHVSing V3.3 is about 7.8 MB with 0.478 M parameters. The NHVSing code is MIT; its published weights are non-commercial and are not bundled.

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

## Whole-song polish

`lessampler --polish <mix.wav> <out.wav>` resynthesizes an assembled mix as one timeline: F0 from the shared WORLD front end, 128-bin mel from the mix, one NHVSing pass, RMS-matched output. On the 245 s reference song this is 42,198 frames, about 60 s and 5.5 GB peak memory. It removes per-note phase resets but regenerates the timbre.

## Feature-level assembly

`lessampler --feature-plan plan.toml` avoids double vocoding. It reuses the wavtool plan, slices each note WAV by `skip_ms`/`length_ms`, extracts raw mel and F0, applies the UST envelope in mel/F0 space, and sums the notes on one global timeline. A single global level shift maps the assembled mix to the model's target RMS while preserving per-note dynamics. NHVSing runs once over the whole song; the output is level-matched to a reference mix built from the same note buffers.

Feature-level assembly is meant for WORLD or LLSM note renders. Feeding it per-note neural renders would run the vocoder twice. `scripts/render_ust.py --feature` selects it instead of wavtool assembly.

## Settings

```toml
[audio_model]
backend = "neural"

[neural]
model_path = "/path/to/nhv_v3_3.onnx"
threads = 4
target_rms = 0.111
```

`threads = 0` lets ONNX Runtime choose. The model is fixed at 44.1 kHz; other voicebank rates are resampled for features and back to the source rate.
