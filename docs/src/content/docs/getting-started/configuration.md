---
title: "Configuration"
description: "lessampler.less, model caches and performance."
---

`lessampler.less` is created next to the executable on first run. It is TOML. Delete it to regenerate defaults.

```toml
[config]
version = "git-hash"
debug = false

[audio_model]
backend = "world"           # "world", "llsm", or "neural"
frame_period = 5.805442176870748
fft_size = "auto"
model_amp = 0.85

[f0]
estimator = "auto"          # "auto", "harvest", "dio", "llsm", or "neural"
f0_speed = 1
f0_dio_floor = 40.0
f0_harvest_floor = 40.0
f0_ceil = 800.0
f0_cheap_trick_floor = 71.0
f0_allow_range = 0.1
neural_model_path = ""      # RMVPE ONNX; independent of [neural] model_path
neural_threads = 0
neural_threshold = 0.03
neural_floor = 40.0
neural_ceil = 1200.0

[ap]
ap_threshold = 0.10

[neural]
preset = "nhv"              # "nhv" or "pc-nsf-hifigan"
frontend = "world"          # "world", "world-spectral", "source-mel", or "source-wave"
source_hop = 128            # source-mel mel hop; fft_size >= source_hop
model_path = ""             # NHVSing- or PC-NSF-compatible ONNX file
threads = 0                 # 0 lets ONNX Runtime choose
target_rms = 0.111
sample_rate = 44100
hop_size = 256
fft_size = 2048
mel_bins = 128
mel_fmin = 40
mel_fmax = 16000
mel_base = "e"              # "e" or "10"
mel_scale = "slaney"        # "slaney" or "htk"
mel_layout = "time-major"   # "time-major" or "bin-major"
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

[style]
default = "natural"         # "natural", "clear", "cry", or "growl"
amount = 1.0                # 0..1, scales the default weight
morph_ms = 0.0              # 0 = spread a start>end morph over the note
```

`backend` selects the synthesis path. Selecting a backend that is not compiled in throws a `parameter_error`. `[neural] model_path` is required by the neural backend, `--polish`, and `--feature-plan`; model weights are not bundled. `[neural] preset = "pc-nsf-hifigan"` selects the OpenVPI 44.1 kHz / hop 512 / `mel`+`f0` contract; the remaining fields override the preset for other vocoders. Preset names are case-insensitive and stored canonical.

`[neural] frontend` selects how mel/F0 are built for the per-note neural backend. `"world"` (default) renders the time/pitch-transformed WORLD model and extracts mel from that waveform. `"world-spectral"` reads the transformed WORLD spectrogram directly. `"source-mel"` extracts mel from the original voicebank sample and maps it through the UTAU time/pitch controls; `source_hop` sets its mel hop (default 128). `"source-wave"` maps the source waveform through the UTAU time controls before extracting mel. The source-mel, source-wave, and world-spectral frontends are experimental: the current benchmark favors `world`, and their F0/seam metrics are worse on the reference excerpt.

`[f0] estimator` is `auto`, `harvest`, `dio`, `llsm`, or `neural`. `auto` follows `[audio_model] backend`: WORLD uses Harvest, LLSM uses Harvest followed by `llsm_refine_f0`, and neural uses RMVPE. If the optional estimator is not compiled, or the RMVPE model is missing, `auto` falls back to Harvest with a warning. An explicitly selected `llsm` or `neural` estimator that is unavailable throws instead of falling back. The old `[f0] f0_mode = "DIO" | "HARVEST"` key is still read as a deprecated alias for `estimator`; when both are present, `estimator` wins.

RMVPE requires an ONNX model at `[f0] neural_model_path`; weights are not bundled. The adapter accepts an `audio`/`waveform` input with either a 360-bin `salience` output or `f0` + `uv` outputs. `neural_threshold` defaults to 0.03 (some exports use 0.35). `neural_floor`/`neural_ceil` clamp decoded F0; the default ceiling is 1200 Hz to match the current WORLD transform average window, while RMVPE itself supports up to 2000 Hz.

Old `lessampler.fex` files, both TOML and legacy INI, are migrated automatically and kept as `lessampler.fex.bak`. An unparseable `lessampler.less` is backed up as `lessampler.less.bak` before INI migration.

A voicebank can override the global config with `<voicebank>/lessvoice.less`; `ConfigResolver` caches the result per directory.

`[style]` sets the default voice color. A note can override it with UST `Flags` (`C`, `Y`, `G`, and `>` for a start-to-end morph). Voice style is synthesis-only and is excluded from the analysis fingerprint, so changing it does not rebuild a model cache. The flag grammar and the automatic breath/vibrato/portamento settings are in [Expression and voice color](../guides/expression.md).

## Model caches

All caches are sidecars next to the source WAV. Both record a config fingerprint and the source size/mtime, so a settings change or a modified sample invalidates them.

| File | Owner | Contents |
| --- | --- | --- |
| `.lessaudio` | `Vocoder/World/WorldModelCache` | WORLD F0, spectrogram, aperiodicity; format v2 |
| `.lessllsm` | `Vocoder/Llsm/LlsmCache` | layer-1 F0, RD, VSPHSE, VTMAGN, noise model; format v1 |

The WORLD backend loads or rebuilds `.lessaudio`; the LLSM backend uses `.lessllsm` and never writes `.lessaudio`. The neural backend reuses `.lessaudio` and writes no cache of its own. Cache writes are best-effort: a read-only voicebank still renders, and batch modeling reports real analysis errors.

The resolved F0 estimator is part of the fingerprint, together with the RMVPE threshold/floor/ceil and the model path/size/mtime. Switching `[audio_model] backend` while `estimator = "auto"` therefore rebuilds `.lessaudio` when the resolved estimator changes; set a fixed estimator to share one cache across WORLD and neural. Existing caches rebuild once because the fingerprint format changed.

## Performance

- WORLD D4C uses a fixed eight-block frame partition; the partition and per-block random state do not depend on worker count, so analysis is reproducible. `max_workers` only controls concurrency.
- The LLSM layer-1 cache made warm note rendering roughly 10× faster on the reference UST: about 45 ms per note instead of 0.5–0.9 s.
- Warm WORLD note rendering is in the same range (tens of milliseconds per note).
- The neural backend is a quality mode, not a speed mode. A warm one-second note measured about 170 ms: roughly 35 ms preliminary WORLD synthesis, 11 ms mel extraction, 90 ms ONNX inference with four threads, and 20–40 ms model load. A UTAU host starts a fresh process per note, so the model load is paid per note.
- `estimator = "dio"` is substantially faster than `"harvest"` for first-time analysis. The default `auto` + WORLD still resolves to Harvest for quality.
- RMVPE adds a 16 kHz resample and one ONNX inference on a cold analysis; a warm `.lessaudio` load skips it. Set `[f0] neural_threads = 1` when `VoicebankModeler` processes many files concurrently to avoid ONNX thread oversubscription.
- `LESSAMPLER_TIME_EVAL=ON` prints every analysis, cache, transform, and synthesis stage.
- GPU offload was evaluated and is not used. UTAU starts a fresh resampler process per note; Vulkan context creation alone measured about 0.7 s, far above the tens-of-milliseconds warm render. GPU compute would need a persistent batch/server host or a different protocol.
