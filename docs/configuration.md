# Configuration

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
f0_mode = "HARVEST"         # "DIO" or "HARVEST"
f0_speed = 1
f0_dio_floor = 40.0
f0_harvest_floor = 40.0
f0_cheap_trick_floor = 71.0
f0_allow_range = 0.1

[ap]
ap_threshold = 0.10

[neural]
model_path = ""             # NHVSing-compatible ONNX file
threads = 0                 # 0 lets ONNX Runtime choose
target_rms = 0.111
```

`backend` selects the synthesis path. Selecting a backend that is not compiled in throws a `parameter_error`. `[neural] model_path` is required by the neural backend, `--polish`, and `--feature-plan`; model weights are not bundled.

Old `lessampler.fex` files, both TOML and legacy INI, are migrated automatically and kept as `lessampler.fex.bak`. An unparseable `lessampler.less` is backed up as `lessampler.less.bak` before INI migration.

A voicebank can override the global config with `<voicebank>/lessvoice.less`; `ConfigResolver` caches the result per directory.

## Model caches

All caches are sidecars next to the source WAV. Both record a config fingerprint and the source size/mtime, so a settings change or a modified sample invalidates them.

| File | Owner | Contents |
| --- | --- | --- |
| `.lessaudio` | `Vocoder/World/WorldModelCache` | WORLD F0, spectrogram, aperiodicity; format v2 |
| `.lessllsm` | `Vocoder/Llsm/LlsmCache` | layer-1 F0, RD, VSPHSE, VTMAGN, noise model; format v1 |

The WORLD backend loads or rebuilds `.lessaudio`; the LLSM backend uses `.lessllsm` and never writes `.lessaudio`. The neural backend reuses `.lessaudio` and writes no cache of its own. Cache writes are best-effort: a read-only voicebank still renders, and batch modeling reports real analysis errors.

## Performance

- WORLD D4C uses a fixed eight-block frame partition; the partition and per-block random state do not depend on worker count, so analysis is reproducible. `max_workers` only controls concurrency.
- The LLSM layer-1 cache made warm note rendering roughly 10× faster on the reference UST: about 45 ms per note instead of 0.5–0.9 s.
- Warm WORLD note rendering is in the same range (tens of milliseconds per note).
- The neural backend is a quality mode, not a speed mode. A warm one-second note measured about 170 ms: roughly 35 ms preliminary WORLD synthesis, 11 ms mel extraction, 90 ms ONNX inference with four threads, and 20–40 ms model load. A UTAU host starts a fresh process per note, so the model load is paid per note.
- `f0_mode = "DIO"` is substantially faster than `"HARVEST"` for first-time analysis. The default remains `HARVEST` for quality.
- `LESSAMPLER_TIME_EVAL=ON` prints every analysis, cache, transform, and synthesis stage.
- GPU offload was evaluated and is not used. UTAU starts a fresh resampler process per note; Vulkan context creation alone measured about 0.7 s, far above the tens-of-milliseconds warm render. GPU compute would need a persistent batch/server host or a different protocol.
