---
title: "Quality evaluation"
description: "SingMOS-Pro and objective metrics with a versioned JSON report."
---

`test/quality/evaluate.py` is the standardized evaluation entry point. It reports SingMOS-Pro MOS and objective metrics for one or more renders in a versioned JSON document.

## Protocol

- MOS model: SingMOS-Pro `South-Twilight/SingMOS:v1.1.2`, `singmos_pro`.
- Input to the MOS model: mono, 16 kHz. Chunks are 10 s; chunks with RMS below 0.001 are dropped so rests do not dominate. The report gives the mean, p10 and minimum over the remaining chunks.
- Objective metrics: F0 MAE, median error, gross pitch error and VUV error against the UST note/PBS/PBY target; note-boundary seam dips; longest silence; peak and RMS level.
- The report schema is versioned (`schema_version: 1`). The model version, chunk length and silence gate are written into the report.

The F0 target is the UST curve. Inside a preutterance or overlap two notes can sound at once; a single F0 estimate can match only one of them. Read GPE and VUV together with the seam metric instead of treating them as a per-note score.

## Setup

The objective metrics need only the Python standard library. For MOS:

```bash
./test/quality/setup_singmos.sh
```

The script creates `build/quality-venv` with CPU PyTorch 2.5.1, torchaudio, s3prl, librosa and soundfile, then downloads the SingMOS weights. The weights are non-commercial and are not bundled with lessampler.

## Run

```bash
python3 test/quality/evaluate.py \
  --ust "data/シャノワールの冒険書/main_調声済み.ust" \
  --lessampler build/max-gcc/bin/lessampler \
  --mos singmos \
  --json build/quality/report.json \
  data/render/main-wavtool.wav \
  data/render/main-feature.wav
```

`--mos` defaults to `none`, which produces the objective metrics without torch. `--min-mos` makes the command exit with status 2 when a file scores below the threshold. WAV input must be 16-bit PCM, which is what `lessampler` and `lesswavtool` write.

## CTest

`QualityEvaluationTest` runs the objective path on the repository fixture and is registered whenever CMake finds Python 3:

```bash
ctest --preset max-gcc -L evaluation --output-on-failure
```

The real MOS test is opt-in because it needs the SingMOS environment:

```bash
cmake --preset max-gcc \
  -DLESSAMPLER_ENABLE_SINGMOS_TESTS=ON \
  -DLESSAMPLER_SINGMOS_PYTHON="$PWD/build/quality-venv/bin/python"
cmake --build --preset max-gcc -j
ctest --preset max-gcc -L evaluation --output-on-failure
```

## JSON report

```json
{
  "schema_version": 1,
  "parameters": {
    "ust": "song.ust",
    "lessampler": "build/max-gcc/bin/lessampler",
    "mos_backend": "singmos",
    "singmos_model": "South-Twilight/SingMOS:v1.1.2",
    "chunk_seconds": 10.0,
    "min_chunk_rms": 0.001,
    "f0_margin_ms": 30.0,
    "min_mos": null
  },
  "files": [
    {
      "path": "main.wav",
      "duration_s": 244.964,
      "sample_rate": 44100,
      "channels": 1,
      "peak_dbfs": -0.4,
      "rms_dbfs": -17.9,
      "f0": {
        "frames": 40211,
        "mae_cents": 13.7,
        "median_cents": 6.1,
        "gpe": 0.114,
        "vuv": 0.02
      },
      "seams": {
        "dips": 9,
        "evaluated": 675,
        "median": 0.97,
        "longest_silence_ms": 250.0
      },
      "mos": {
        "backend": "singmos",
        "model": "South-Twilight/SingMOS:v1.1.2",
        "mean": 3.12,
        "p10": 2.8,
        "min": 2.4,
        "chunks": 24
      }
    }
  ]
}
```

`f0`, `seams` and `mos` are `null` when the matching input was not requested. With `--min-mos`, each MOS block also contains `passed`.

## Limits

- SingMOS is an automatic proxy for subjective MOS, not a replacement for a listening panel.
- The F0 target is ambiguous inside overlaps, as described above.
- MOS runs on the whole render in 10 s chunks; very long songs are not streamed.
- The CTest fixture is a short voice sample. Use full `data/` renders for quality tracking.
