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

- Requires CMake 3.30+ (3.31.x recommended; CMake 4.x works through the `CMAKE_POLICY_VERSION_MINIMUM` shim in the root `CMakeLists.txt`).
- Requires GCC 15+, Clang 19+, or MSVC in `/std:c++latest` mode. CMake does not map `CXX_STANDARD 26` for MSVC or AppleClang; the Windows preset uses MSVC's latest mode, and the macOS preset uses Homebrew GCC 15.
- Presets: `debug-gcc`, `release-gcc`, `asan-gcc`, `ci-gcc`, `ci-clang`, `ci-msvc`. The `build/<preset>` directories are gitignored.
- CMakeLists use explicit source lists. Adding a source file requires editing the relevant `CMakeLists.txt`.
- `lessampler_common` is an INTERFACE target carrying C++26 features, include paths, warnings, and compile definitions. Link it through the library targets; do not add global `include_directories`.
- `LESSAMPLER_BUILD_TESTS`, `LESSAMPLER_BUILD_TOOLS`, `LESSAMPLER_ENABLE_SANITIZERS`, `LESSAMPLER_DUMP_DATA`, `LESSAMPLER_TIME_EVAL`, and `LESSAMPLER_WERROR` are the project options.
- `build/<preset>/lessconfig.h` is generated from `config.h.in`; `PROJECT_GIT_HASH` is the short commit hash.

## Running

`lessampler.fex` (TOML) is created next to the executable on first run; delete it to regenerate defaults. Sections: `[config]`, `[audio_model]`, `[f0]`, `[ap]`. Older INI files are migrated automatically and kept as `lessampler.fex.bak`. File names are defined in `src/Constants.h`.

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

1. **Config** — `ConfigUnit` loads or creates `lessampler.fex`; `ConfigVoiceBank` applies `lessvoice.fex` from the input sample's directory when present.
2. **Model cache** — `AudioModelIO` maps the input WAV to a `.lessaudio` sidecar. If it is missing, version-mismatched, config-mismatched, or the source WAV size/mtime changed, `GenerateAudioModel` re-analyzes the WAV.
3. **Analysis** — `WorldModule` runs DIO (or Harvest) + StoneMask for F0, CheapTrick for the spectral envelope, and D4C for aperiodicity. Results are stored in vector-owned `WorldPara` and moved into `lessAudioModel`.
4. **Parameter translation** — `Shine` turns UTAU arguments into `ShinePara`: the note name becomes Hz via `ScaleConvert` (`std::expected` on invalid input), velocity is `2^(time%/100 - 1)`, and `PitchBendDecoder` decodes the pitchbend string (including `#N#` runs).
5. **Transform** — `AudioProcess` re-targets average pitch and builds a new model with `ShinePara::required_frame` frames, clamping all frame and pitch-bend indexes.
6. **Synthesis** — `Synthesis` drives WORLD's realtime API (`InitializeSynthesizer`/`AddParameters`/`Synthesis2`) with a 64-sample buffer into a fixed `output_samples` vector. `DestroySynthesizer` is called by an RAII guard.
7. **Amplitude** — `AutoAMP` scales by `0.5 * (volume% / 100) / max(abs)` and clamps to [-1, 1]. A zero max produces silence.
8. **Output** — `WavIO::WriteWav` writes mono 16-bit PCM via libsndfile. `WavIO::ReadWav` returns a mono `WavData` (multi-channel input is averaged per frame).

## Core data types

- `lessAudioModel` (`src/AudioModel/lessAudioModel.h`) — the value-type model passed between stages: raw PCM `x`, `f0`, `spectrogram[frame][bin]`, `aperiodicity[frame][bin]`, `fs`, `frame_period` (ms), `fft_size`, and `w_length = fft_size / 2 + 1`. This is the serialized representation.
- `WorldPara` (`src/AudioModel/WorldModule/WorldPara.h`) — vector-owned WORLD result. `WorldModule` adapts to WORLD's C API with local row-pointer arrays; there is no manual `delete` and copies are safe.
- `lessConfigure` (`src/ConfigUnit/lessConfigure.h`) — all analysis options. `fingerprint()` is an FNV-1a hash of the analysis-affecting settings, stored in every `.lessaudio`; changing frame_period/fft_size/F0/AP settings invalidates cached models.
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

- Unit labels: `PitchBendTest`, `ScaleConvertTest`, `libUTAUTest`, `ConfigUnitTest`, `LOGTest`, `TimerTest`.
- Integration labels: `AudioModelBinTest`, `AudioModelBinVersionTest`, `WorldModuleTest`, `GenerateAudioModelTest`, `AudioProcessTest`.
- Test data path is passed as `LESSAMPLER_TEST_DATA_DIR`; tests read `vaiueo2d.wav` and write only to temporary directories.
- Sanitizer run: `cmake --preset asan-gcc && cmake --build --preset asan-gcc -j && ctest --preset asan-gcc`.

## Conventions

- `.clang-format` (LLVM base, 4-space indent, no column limit, `Standard: Latest`); format with `clang-format -i <file>`.
- No test framework, linter, or package manager — CMake, CTest, and submodules only.
- Removed dependencies: ColorCout (ANSI constants now live in `LOG.h`), FTXUI, rapidjson, inicpp, and the JSON export.
