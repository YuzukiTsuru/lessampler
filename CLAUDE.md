# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this project is

lessampler is a work-in-progress singing-voice synthesizer that implements the UTAU resampler command-line protocol. C++26, CMake, LGPL-3.0. A voicebank is a folder of WAV samples; note-independent analysis is cached next to each sample as binary sidecars (`.lessaudio` for WORLD, `.lessllsm` for LLSM).

The code is organized as layered domains. See `ARCHITECTURE.md` for the full module map. The dependency direction is:

```
App -> Engine -> Vocoder -> Config/Audio -> Core
                 ^ Host/UTAU and Platform are edge adapters
```

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
- Presets: `debug-gcc`, `release-gcc`, `asan-gcc`, `ci-gcc`, `ci-llsm`, `ci-clang`, `ci-msvc`. The `build/<preset>` directories are gitignored.
- CMakeLists use explicit source lists. Adding a source file requires editing the relevant domain `CMakeLists.txt`.
- CMake targets mirror the layers: `Core`, `Config`, `Audio`, `VocoderCommon`, `VocoderWorld`, `VocoderLlsm` (optional), `Vocoder`, `Engine`, `UTAU`, `Shine`, `Platform`, and the `lessampler` executable.
- `lessampler_common` is an INTERFACE target carrying C++26 features, include paths, warnings, and compile definitions. Link it through the library targets; do not add global `include_directories`.
- `LESSAMPLER_BUILD_TESTS`, `LESSAMPLER_BUILD_TOOLS`, `LESSAMPLER_ENABLE_SANITIZERS`, `LESSAMPLER_ENABLE_LLSM`, `LESSAMPLER_DUMP_DATA`, `LESSAMPLER_TIME_EVAL`, and `LESSAMPLER_WERROR` are the project options.
- `LESSAMPLER_ENABLE_LLSM=ON` builds the optional libllsm2 vocoder from the `lib/llsm` and `lib/ciglet` submodules. libllsm2 is GPLv3, so the resulting binary is GPLv3; the default WORLD-only binary stays LGPL-3.0.
- `build/<preset>/lessconfig.h` is generated from `config.h.in`; `PROJECT_GIT_HASH` is the short commit hash.

## Running

`lessampler.less` (TOML) is created next to the executable on first run; delete it to regenerate defaults. The executable now lands in `build/<preset>/bin/lessampler`, so its config is `build/<preset>/bin/lessampler.less`. Sections: `[config]`, `[audio_model]`, `[f0]`, `[ap]`. `[audio_model] backend` is `"world"` or `"llsm"`; selecting `"llsm"` in a build without `LESSAMPLER_ENABLE_LLSM` throws a `parameter_error`. Old `lessampler.fex` files, both TOML and legacy INI, are migrated to `lessampler.less` and kept as `lessampler.fex.bak`; an unparseable `lessampler.less` is backed up as `lessampler.less.bak` before INI migration. File names are defined in `src/Core/Constants.h`.

Invocation modes are parsed by `Application` (`src/App/Application.cpp`):

| Arguments | Behavior |
| --- | --- |
| `--help`, `--version` | print usage or the version banner |
| none | logo, usage, and desktop notification |
| `lessampler <dir>` | analysis-only: models every `.wav` in `<dir>` in parallel, writing `<name>.lessaudio` beside each WAV |
| `lessampler <in.wav> <out.wav> <note> ...` (>= 4 args) | UTAU resampler mode: render `<in.wav>` to `<out.wav>` |

UTAU argument positions, parsed by `src/Host/UTAU/libUTAU/UTAUParameterParser.cpp`:

```
argv[1] input wav      argv[2] output wav      argv[3] note (A4, A#3, ...)
argv[4] time percent   argv[5] flags string    argv[6] offset ms
argv[7] required ms    argv[8] fixed ms        argv[9] last-unused ms (negative counts from the end)
argv[10] volume %      argv[11] modulation %   argv[12] tempo (e.g. !120)
argv[13] pitchbend (UTAU 64-char encoding, optional)
```

Example, from the repo root:

```bash
./build/release-gcc/bin/lessampler test/vaiueo2d.wav /tmp/out.wav A4 100 "" 0 1000 100 0 100 0 '!120' ""
```

If the input WAV does not exist, a silent WAV of the requested length is written instead (UTAU uses missing files such as `x.wav` for rests).

## Processing pipeline

`Application` is a thin host. The rendering work is owned by `RenderEngine` (`src/Engine/RenderEngine.cpp`):

1. **Config** — `ConfigLoader` loads or creates the global `lessampler.less`. `ConfigResolver` resolves a per-sample `Config`, applying `<voicebank>/lessvoice.less` and caching each voicebank directory for the session.
2. **Source** — `SampleSource` probes the WAV header immediately and exposes the PCM lazily. A cache hit never decodes the waveform.
3. **Analysis** — `RenderEngine::analysis_for` looks up an in-memory LRU keyed by backend kind/version, effective config fingerprint, canonical source path, and source size/mtime. On a miss it calls `VocoderBackend::Prepare`, which loads or creates the on-disk analysis.
4. **Synthesis** — `VocoderBackend::Synthesize` renders one note from the note-independent analysis and `RenderParams`.
5. **Amplitude** — `RenderEngine` applies `AutoAMP` in place (`0.5 * volume / max(abs)`, clamped to [-1, 1]).
6. **Output** — `WavIO::WriteWav` writes mono 16-bit PCM via libsndfile. `WavIO::ReadWav` returns a mono `WavData` (multi-channel input is averaged per frame).

The `VocoderBackend` contract deliberately separates the two operations:

```cpp
AnalysisPtr Prepare(const AnalysisRequest &);       // cacheable per sample
RenderResult Synthesize(const SynthesisRequest &);  // per note
```

`Analysis` is an opaque shared handle. WORLD wraps `WorldModel`; LLSM wraps an `llsm_chunk` plus its F0 track. The engine and hosts never inspect backend-specific data.

### WORLD path

`Vocoder/World/WorldModelStore` returns a `.lessaudio` model when valid, otherwise decodes the PCM, applies the configured model amplitude, runs `WorldAnalyzer` (DIO/Harvest + StoneMask, CheapTrick, D4C), and saves the cache. `WorldTransform` re-targets pitch/time into a new `WorldModel` with `RenderParams::required_frame` frames. `WorldSynthesizer` drives WORLD's realtime API (`InitializeSynthesizer`/`AddParameters`/`Synthesis2`) with a 64-sample buffer.

D4C uses a fixed eight-block frame partition. The partition and per-block random state are independent of the worker count, so output is reproducible; `max_workers` only controls concurrency.

### LLSM path

LLSM is not F0-free. Its harmonic/noise analysis and synthesis require an F0 contour; the shared DIO/Harvest + StoneMask front end supplies it, and libllsm2's internal refinement is disabled (`f0_refine = 0`). The `[f0]` settings therefore apply to both backends.

`LlsmBackend::Prepare` first tries `.lessllsm`, a layer-1 cache keyed by the config fingerprint and source size/mtime. A hit skips F0 estimation, `llsm_analyze`, and layer-1 conversion. On a miss it estimates F0 with the shared front end, runs `llsm_analyze`, converts to layer 1, applies source phase propagation, and saves the cache. `Synthesize` interpolates layer-1 frames along the UTAU time map, writes the target F0 per frame, converts back to layer 0, and calls `llsm_synthesize`.

libllsm2 is compiled with its native `FP_TYPE=float`; input PCM and F0 are converted once from double. `srand(1)` before synthesis makes libllsm2's `rand()`-based noise excitation reproducible. LLSM never writes `.lessaudio`.

## Core data types

- `WorldModel` (`src/Vocoder/World/WorldModel.h`) — the value-type WORLD analysis: `f0`, `spectrogram[frame][bin]`, `aperiodicity[frame][bin]`, `fs`, `frame_period` (ms), `fft_size`, `w_length = fft_size / 2 + 1`, and `x_length` (PCM is not retained). This is the serialized representation.
- `WorldPara` (`src/Vocoder/World/WorldPara.h`) — internal vector/Matrix-owned result with `time_axis`.
- `WorldAnalyzer` (`src/Vocoder/World/WorldAnalyzer.h`) — DIO/Harvest + StoneMask, CheapTrick, and D4C.
- `WorldModelBuilder` / `BuildWorldModel` — converts analyzer output into a `WorldModel`.
- `WorldModelStore` / `WorldModelCache` — disk-cache load/build/save for `.lessaudio`.
- `WorldTransform` — UTAU time/pitch transform over a `WorldModel`.
- `WorldSynthesizer` / `SynthesizeWorld` — WORLD render into a fixed output vector.
- `AutoAMP` (`src/Audio/AutoAMP.h`) — amplitude normalization.
- `F0Track` / `EstimateF0` (`src/Vocoder/Common/F0Estimator.h`) — shared DIO/Harvest + StoneMask front end.
- `SampleSource` (`src/Audio/SampleSource.h`) — path, `WavInfo`, `SourceFingerprint`, lazy PCM.
- `Config` (`src/Config/Config.h`) — all analysis options and `vocoder`. `fingerprint()` is an FNV-1a hash of the WORLD-analysis-affecting settings, stored in caches; the backend is not part of the fingerprint.
- `ConfigResolver` (`src/Config/ConfigResolver.h`) — global config plus cached per-voicebank overrides.
- `RenderParams` (`src/Core/RenderParams.h`) — derived UTAU rendering parameters: lengths in ms, pitch bend, volume (fraction), modulation.
- `VocoderKind` (`src/Config/VocoderKind.h`) — `world` / `llsm` config enum and parser.
- `VocoderBackend` (`src/Vocoder/VocoderBackend.h`) — backend factory and `Prepare`/`Synthesize` interface.
- `LlsmCache` (`src/Vocoder/Llsm/LlsmCache.h`) — RAII `llsm_chunk` owner and `.lessllsm` reader/writer.
- `RenderEngine` (`src/Engine/RenderEngine.h`) — config resolution, backend registry, analysis LRU, synthesis, and AutoAMP.
- `VoicebankModeler` (`src/Engine/VoicebankModeler.*`) — batch `.lessaudio` generation with per-file config resolution and a bounded worker budget.

## Cache formats

### `.lessaudio` v2

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

PCM is not stored. Readers validate magic, version, checksum, source fingerprint, row widths, frame counts, and truncation. `CheckAudioModel()` returns false for any mismatch; `ReadAudioModel()` throws.

### `.lessllsm` v1

LLSM layer-1 cache, built only with `LESSAMPLER_ENABLE_LLSM=ON`:

```
magic "LSLL" + format_version uint32 = 1
config fingerprint string
source WAV size uint64 + mtime int64
fs int32 + thop double + nfft uint32 + nfrm uint64
npsd uint32 + nchannel uint32 + maxnhar uint32 + maxnhar_e uint32
fnyq double + noswarp double + lipradius double
channel frequency count uint32 + FP_TYPE floats
per frame: f0 FP_TYPE + rd FP_TYPE + has_envelope uint8
           if voiced: nhar uint32 + VSPHSE floats + VTMAGN floats
           noise PSD floats + noise channel DC floats
           per channel: envelope nhar uint32 + amplitude floats + phase floats
trailer "L1EN"
```

Readers validate magic, version, fingerprint, source fingerprint, dimensions, frame counts, harmonic counts, and truncation before allocating frames. `LoadLlsmCache()` returns nullptr for stale/malformed files; `SaveLlsmCache()` writes through a unique temporary file and rename.

## Logging and debugging

- Use the `LOG_*` macros from `src/Core/Log.h`: `LOG_INFO`, `LOG_WARN`, `LOG_ERROR`, `LOG_OK`, `LOG_EVAL`, `LOG_CRITICAL`, plus `LOG_DEBUG`/`LOG_DUMP`.
- The logger namespace is `lessampler::log`.
- `LOG_DEBUG` output is disabled unless `lessampler::log::SetDebug(true)` is called (or the Debug build's config defaults `debug=true`); `lessampler::log::DebugEnabled()` queries the state.
- `LESSAMPLER_DUMP_DATA` and `LESSAMPLER_TIME_EVAL` are CMake options translated into compile definitions. `DUMP_DATA` gates the per-sample dump file in `WorldTransform`. With `LESSAMPLER_TIME_EVAL=ON`, `LOG_EVAL` prints stage timings for F0/CheapTrick/D4C, WORLD model prepare/transform/synthesis, and LLSM cache load/save, layer conversions, transform, and synthesis.
- `tools/arg_parameter_export` is a dummy executable that appends its `argv` to `<its own path>.log`; use it to capture how a UTAU host invokes the resampler.
- `ConfigResolver` loads `<voice_dir>/lessvoice.less` in TOML and caches the result per directory.

## Tests

Tests are registered with CTest. Build and run:

```bash
cmake --build --preset release-gcc -j
ctest --preset release-gcc --output-on-failure
```

- Unit labels: `PitchBendTest`, `ScaleConvertTest`, `libUTAUTest`, `ConfigLoaderTest`, `LOGTest`, `TimerTest`, `WorldTransformTest`, `AutoAMPTest`, `WavIOTest`, `ShineTest`.
- Integration labels: `WorldModelCacheTest`, `WorldModelCacheVersionTest`, `WorldAnalyzerTest`, `VoicebankModelerTest`, `WorldPipelineTest`, `WorldModelTest`, `WorldSynthesizerTest`, `RenderEngineTest`, `EndToEndTest`.
- `RenderEngineTest` checks the backend factory, WORLD and LLSM renders, A4/A#4 pitch, in-memory analysis reuse, cold/warm LLSM cache equality, source-mtime and config-fingerprint invalidation, and corrupt-cache recovery. Without LLSM it checks that selecting `llsm` is rejected.
- `WorldAnalyzerTest` runs the fixture analysis twice and compares f0, spectrogram, and aperiodicity exactly, guarding the fixed D4C chunk partition.
- Test data path is passed as `LESSAMPLER_TEST_DATA_DIR`; tests read `vaiueo2d.wav` and write only to temporary directories.
- Sanitizer run: `cmake --preset asan-gcc && cmake --build --preset asan-gcc -j && ctest --preset asan-gcc`. Add `-DLESSAMPLER_ENABLE_LLSM=ON` to sanitize the optional backend too; `src/Vocoder/CMakeLists.txt` instruments the vendored C sources with the same flags.

## Conventions

- `.clang-format` (LLVM base, 4-space indent, no column limit, `Standard: Latest`); format with `clang-format -i <file>`.
- No test framework, linter, or package manager — CMake, CTest, and submodules only.
- Optional backend dependencies: `lib/llsm` (Dreamtonics/libllsm2, GPLv3) and `lib/ciglet` (BSD). Their C sources are compiled directly by `src/Vocoder/CMakeLists.txt` with `FP_TYPE=float`; `src/Vocoder/Llsm/compat/libihnm/ihnm.h` is an empty header for a stale libllsm2 include.
- Removed dependencies: ColorCout (ANSI constants now live in `Log.h`), FTXUI, rapidjson, inicpp, and the JSON export.
