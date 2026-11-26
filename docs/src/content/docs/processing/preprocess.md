---
title: "Source preprocessing"
description: "High-pass, click/pop repair and spectral denoise before analysis."
---

Voicebank samples can contain DC offset, rumble, plosive thumps, isolated clicks and broadband hiss. lessampler can clean the decoded PCM before F0 and spectral analysis:

```text
decode WAV -> high-pass -> click/pop repair -> spectral denoise -> F0 / WORLD / LLSM
```

The stage runs only when an analysis cache is built. A cache hit returns the stored analysis without decoding or preprocessing the WAV.

## Configuration

```toml
[preprocess]
enabled = false
highpass_hz = 70.0        # 0 disables the high-pass
declick = true
declick_threshold = 12.0  # robust residual threshold; higher is less aggressive
denoise = true
denoise_db = 12.0         # maximum attenuation at the gain floor
```

`enabled = false` leaves the samples and the analysis fingerprint unchanged. When enabled, the settings are part of `Config::fingerprint()`, so a change rebuilds `.lessaudio` and `.lessllsm`.

## Stages

The high-pass is a second-order RBJ filter at `highpass_hz`. It removes DC offset and low-frequency handling/plosive thumps. Setting `highpass_hz = 0` keeps the original low end.

Click/pop repair uses the second difference of the waveform. It compares each residual with the median absolute deviation of the whole buffer, then linearly interpolates short flagged runs of at most 3 ms. The median/MAD scale keeps ordinary consonants and steady tones from being treated as clicks; isolated spikes and digital pops are removed.

Spectral denoise uses a 1024-point Hann STFT with a 256-sample hop. The noise floor is estimated from the 20th percentile of sampled frame magnitudes per bin. The second pass applies spectral subtraction with frequency and time smoothing and a gain floor of `10^(-denoise_db/20)`, then overlap-adds with window-squared normalization. The result is deterministic and needs no model weights.

## Scope

Preprocessing runs in the WORLD and LLSM `Prepare` paths, and therefore also feeds the per-note neural backend through its WORLD cache. `lessampler --f0` applies the same stage so its track matches the analysis.

It is not applied a second time to an assembled mix: `--polish` and `--feature-plan` work from note WAVs that were already rendered from preprocessed sources.

## Tuning

- `denoise_db = 0` or `denoise = false` disables spectral denoise.
- `declick = false` leaves transients untouched; raise `declick_threshold` if consonant attacks are being softened.
- A high-pass around 50–80 Hz is typical for solo voice. Too high thins low notes.
- Audition consonants and breath samples after changing denoise; the gain floor trades hiss removal against musical-noise artifacts.
- This is not a dereverberation or de-essing stage, and it does not train a neural denoiser.

Next: [Modeling](modeling.md).
