# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this project is

lessampler is a work-in-progress singing-voice synthesizer that implements the UTAU resampler command-line protocol on top of the WORLD vocoder. C++26, CMake, LGPL-3.0. A voicebank is a folder of WAV samples; analysis results are cached next to each sample as `.lessaudio` binary files.

## Build

Everything under `lib/` is a git submodule (WORLD, portable-file-dialogs, libsndfile, toml++). Configure requires initialized submodules:

```bash
git submodule update --init --recursive

cmake --preset release-gcc
cmake --build --preset release-gcc -j
ctest --preset release-gcc --output-on-failure
```

To build and test the optional LLSM backend:

```bash
cmake --preset release-gcc -DLESSAMPLER_ENABLE_LLSM=ON
cmake --build --preset release-gcc -j
ctest --preset release-gcc --output-on-failure
```

- Requires CMake 3.30+ (3.31.x recommended; CMake 4.x works through the `CMAKE_POLICY_VERSION_MINIMUM` shim in the root `CMakeLists.txt`).
- Requires GCC 15+, Clang 19+, or MSVC in `/std:c++latest` mode. CMake does not map `CXX_STANDARD 26` for MSVC or AppleClang; the Windows preset uses MSVC's latest mode, and the macOS preset uses Homebrew GCC 15.
- Presets: `debug-gcc`, `release-gcc`, `asan-gcc`, `ci-gcc`, `ci-clang`, `ci-msvc`. The `build/<preset>` directories are gitignored.
- CMakeLists use explicit source lists. Adding a source file requires editing the relevant `CMakeLists.txt`.
- `lessampler_common` is an INTERFACE target carrying C++26 features, include paths, warnings, and compile definitions. Link it through the library targets; do not add global `include_directories`.
- `LESSAMPLER_BUILD_TESTS`, `LESSAMPLER_BUILD_TOOLS`, `LESSAMPLER_ENABLE_SANITIZERS`, `LESSAMPLER_ENABLE_LLSM`, `LESSAMPLER_DUMP_DATA`, `LESSAMPLER_TIME_EVAL`, and `LESSAMPLER_WERROR` are the project options.
- `LESSAMPLER_ENABLE_LLSM=ON` builds the optional libllsm2 vocoder backend from the `lib/llsm` and `lib/ciglet` submodules. libllsm2 is GPLv3, so the resulting binary is GPLv3; the default WORLD-only binary stays LGPL-3.0.
- `build/<preset>/lessconfig.h` is generated from `config.h.in`; `PROJECT_GIT_HASH` is the short commit hash.

## Running

`lessampler.fex` (TOML) is created next to the executable on first run; delete it to regenerate defaults. Sections: `[config]`, `[audio_model]`, `[f0]`, `[ap]`. `[audio_model] backend` is `"world"` or `"llsm"`; selecting `"llsm"` in a build without `LESSAMPLER_ENABLE_LLSM` throws a `parameter_error`. Older INI files are migrated automatically and kept as `lessampler.fex.bak`. File names are defined in `src/Constants.h`.

Invocation modes are decided in `lessampler::ParseArgs` (`src/lessampler.cpp`):

| Arguments | Behavior |
| --- | --- |
| `--help`, `--version` | print usage or the version banner |
| none | logo, usage, and desktop notification |
| `lessampler <dir>` | analysis-only: models every `.wav` in `<dir>` in parallel, writing `<name>.lessaudio` beside each WAV |
| `lessampler <in.wav> <out.wav> <note> ...` (>= 4 args) | UTAU resampler mode: render `<in.wav>` to `<out.wav>` |

UTAU argument positions, parsed by `src/Shine/Binding/libUTAU/UTAUParameterParser.cpp`:

```
argv[1] input wav      argv[2] output wav      argv[3] note (A4, A#3, ...)
argv[4] time percent   argv[5] flags string    argv[6] offset ms
argv[7] required ms    argv[8] fixed ms        argv[9] last-unused ms (negative counts from the end)
argv[10] volume %      argv[11] modulation %   argv[12] tempo (e.g. !120)
argv[13] pitchbend (UTAU 64-char encoding, optional)
```

Example, from the repo root (the config file is written to `build/release-gcc/src/lessampler.fex`):

```bash
./build/release-gcc/src/lessampler test/vaiueo2d.wav /tmp/out.wav A4 100 "" 0 1000 100 0 100 0 '!120' ""
```

If the input WAV does not exist, a silent WAV of the requested length is written instead (UTAU uses missing files such as `x.wav` for rests).

## Processing pipeline

`lessampler::run` (`src/lessampler.cpp`) orchestrates:

1. **Config** — `ConfigUnit` loads or creates `lessampler.fex`; `ConfigVoiceBank` applies `lessvoice.fex` from the input sample's directory when present. `configure.vocoder` selects the backend.
2. **Parameter translation** — `Shine` turns UTAU arguments into `ShinePara`: the note name becomes Hz via `ScaleConvert` (`std::expected` on invalid input), velocity is `2^(time%/100 - 1)`, and `PitchBendDecoder` decodes the pitchbend string (including `#N#` runs). `Shine::CheckPara` fills output length and frame counts from an `AudioInfo` rather than a model, so both backends can use the UTAU path.
3. **Backend** — `CreateVocoderBackend(configure.vocoder)` returns a `VocoderBackend`; `RenderRequest` carries the input path, PCM, sample rate, `ShinePara`, and `lessConfigure`. Both backends share `F0Estimator` (DIO/Harvest + StoneMask).
4. **WORLD path** — `AudioModelIO` maps the input WAV to a `.lessaudio` sidecar. If it is missing, version-mismatched, config-mismatched, or the source WAV size/mtime changed, `GenerateAudioModel` re-analyzes the WAV with `WorldModule` (DIO/Harvest + StoneMask, CheapTrick, D4C). `AudioProcess` re-targets pitch/time into a new `lessAudioModel` with `ShinePara::required_frame` frames, and `Synthesis` drives WORLD's realtime API (`InitializeSynthesizer`/`AddParameters`/`Synthesis2`) with a 64-sample buffer. `DestroySynthesizer` is called by an RAII guard.
5. **LLSM path** — `LlsmBackend` runs `llsm_analyze`, converts to layer 1, interpolates layer-1 frames along the UTAU time map, writes the target F0 per frame, converts back to layer 0, and calls `llsm_synthesize`. libllsm2 is compiled with its native `FP_TYPE=float`; input PCM and F0 are converted once from double. The backend does not use or write `.lessaudio`.
6. **Amplitude** — `AutoAMP` scales by `0.5 * (volume% / 100) / max(abs)` and clamps to [-1, 1]. A zero max produces silence.
7. **Output** — `WavIO::WriteWav` writes mono 16-bit PCM via libsndfile. `WavIO::ReadWav` returns a mono `WavData` (multi-channel input is averaged per frame). `WavIO::ProbeWav` reads only the header; the CLI uses it for UTAU lengths and loads PCM only when the selected backend (LLSM) needs it.

## Core data types

- `lessAudioModel` (`src/AudioModel/lessAudioModel.h`) — the value-type WORLD model passed between stages: `f0`, `spectrogram[frame][bin]`, `aperiodicity[frame][bin]`, `fs`, `frame_period` (ms), `fft_size`, `w_length = fft_size / 2 + 1`, and `x_length` (the PCM is not retained). This is the serialized representation.
- `WorldPara` (`src/AudioModel/WorldModule/WorldPara.h`) — vector/Matrix-owned WORLD result. `WorldModule` adapts to WORLD's C API with local row-pointer arrays; there is no manual `delete` and copies are safe.
- `AudioInfo` (`src/AudioModel/AudioInfo.h`) — `{fs, x_length, frame_period}` used by `Shine::CheckPara` before either backend has produced a model.
- `F0Track` / `EstimateF0` (`src/AudioModel/WorldModule/F0Estimator.h`) — shared DIO/Harvest + StoneMask F0 front end with the configured frame period.
- `VocoderKind` (`src/Backend/VocoderKind.h`) — `world` / `llsm` and the case-insensitive config parser.
- `VocoderBackend` (`src/Backend/VocoderBackend.h`) — abstract `Render(RenderRequest) -> RenderResult` interface. `WorldBackend` and `LlsmBackend` are the implementations; `CreateVocoderBackend` is the factory.
- `lessConfigure` (`src/ConfigUnit/lessConfigure.h`) — all analysis options and `vocoder`. `fingerprint()` is an FNV-1a hash of the WORLD-analysis-affecting settings, stored in every `.lessaudio`; changing frame_period/fft_size/F0/AP settings invalidates cached models. The backend is not part of the fingerprint.
- `ShinePara` (`src/Shine/ShinePara.h`) — derived UTAU rendering parameters: lengths in ms, pitch bend, volume (fraction, defaults to 1.0), modulation.

## Model file format (`.lessaudio` v2)

Custom binary, fixed-width little-endian:

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

The PCM samples are not stored; only `x_length`, and a read model's `x` vector is resized but left zero-filled. Readers validate magic, version, checksum, source fingerprint, row widths, frame counts, and truncation. `CheckAudioModel()` returns false for any mismatch; `ReadAudioModel()` throws.

## Logging and debugging

- Use the `YALL_*` macros from `src/Utils/LOG.h`: `YALL_INFO_`, `YALL_WARN_`, `YALL_ERROR_`, `YALL_OK_`, `YALL_EVAL_`, plus `YALL_DEBUG_`/`YALL_DUMP_`.
- The logger namespace is `lessampler_log` (not `lessampler`) because the class `lessampler` occupies that name.
- `YALL_DEBUG_` output is disabled unless `YALL_DEBUG_.EnableDebug()` is called; the Debug build's config defaults `debug=true`.
- `LESSAMPLER_DUMP_DATA` and `LESSAMPLER_TIME_EVAL` are CMake options translated into compile definitions. `DUMP_DATA` also gates the per-sample dump file in `AudioProcess`.
- `tools/arg_parameter_export` is a dummy executable that appends its `argv` to `<its own path>.log`; use it to capture how an UTAU host invokes the resampler.
- `ConfigVoiceBank` loads `<voice_dir>/lessvoice.fex` in TOML and overrides the global config.

## Tests

Tests are registered with CTest. Build and run:

```bash
cmake --build --preset release-gcc -j
ctest --preset release-gcc --output-on-failure
```

- Unit labels: `PitchBendTest`, `ScaleConvertTest`, `libUTAUTest`, `ConfigUnitTest`, `LOGTest`, `TimerTest`, `AudioProcessUnitTest`, `AutoAMPTest`, `WavIOTest`, `ShineTest`.
- Integration labels: `AudioModelBinTest`, `AudioModelBinVersionTest`, `WorldModuleTest`, `GenerateAudioModelTest`, `AudioProcessTest`, `AudioModelTest`, `SynthesisTest`, `BackendTest`, `EndToEndTest`.
- `BackendTest` checks the factory, the WORLD render, and (when built with `LESSAMPLER_ENABLE_LLSM=ON`) the LLSM render: output length, finite samples, energy, and an autocorrelation pitch estimate near A4. Without LLSM it checks that selecting `llsm` is rejected.
- Test data path is passed as `LESSAMPLER_TEST_DATA_DIR`; tests read `vaiueo2d.wav` and write only to temporary directories.
- Sanitizer run: `cmake --preset asan-gcc && cmake --build --preset asan-gcc -j && ctest --preset asan-gcc`. Add `-DLESSAMPLER_ENABLE_LLSM=ON` to sanitize the optional backend too; `src/Backend/CMakeLists.txt` instruments the vendored C sources with the same flags.

## Conventions

- `.clang-format` (LLVM base, 4-space indent, no column limit, `Standard: Latest`); format with `clang-format -i <file>`.
- No test framework, linter, or package manager — CMake, CTest, and submodules only.
- Optional backend dependencies: `lib/llsm` (Dreamtonics/libllsm2, GPLv3) and `lib/ciglet` (BSD). Their C sources are compiled directly by `src/Backend/CMakeLists.txt` with `FP_TYPE=float`; `src/Backend/compat/libihnm/ihnm.h` is an empty header for a stale libllsm2 include.
- Removed dependencies: ColorCout (ANSI constants now live in `LOG.h`), FTXUI, rapidjson, inicpp, and the JSON export.
