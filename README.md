<div align="center"><img width="80" src="assets/icon_128.gif" alt="lessampler logo"></div>
<h1 align="center"><b>lessampler</b></h1>
<p align="center">
  lessampler is a Singing Voice Synthesizer
</p>

[![forthebadge](https://forthebadge.com/images/badges/contains-cat-gifs.svg)](https://forthebadge.com)

## Download

***Currently lessampler is still under development, there are many bugs that need to be fixed, but welcome to participate in the test.***

You can find the alpha version at [Release](https://github.com/YuzukiTsuru/lessampler/releases)

## Demo

<https://www.gloomyghost.com/lessampler/#/demo/>

## Feature

- Multiple models
  - [x] World
  - [x] llsm (optional build, see below)
  - [x] NHVSing-compatible neural vocoder (optional build, see below)
- Global Transform Control
  - [x] Precise Pitch Shift and Time Stretch
  - [x] FFT Size, Samples Number, Estimation Floor and so on
  - [ ] Support breath sound generation
  - [ ] Support timbre gender change
- Multiple Interactive Interfaces
  - [x] [UTAU](http://utau2008.xrea.jp/) Interface
  - [ ] [OpenUTAU](https://github.com/stakira/OpenUtau) Interface
  - [ ] Library Interface
- Neural Network Vocoder
  - [x] Optional singing-voice neural vocoder backend
  - [ ] Voice Conversion and Audio Style Transfer
  - [ ] Noise reduction enhanced MOS

## Build Status

| Ubuntu | Windows | macOS |
| --- | --- | --- |
| [![CMake Ubuntu](https://github.com/YuzukiTsuru/lessampler/actions/workflows/cmake-ubuntu.yml/badge.svg)](https://github.com/YuzukiTsuru/lessampler/actions/workflows/cmake-ubuntu.yml) | [![CMake Windows](https://github.com/YuzukiTsuru/lessampler/actions/workflows/cmake-windows.yml/badge.svg)](https://github.com/YuzukiTsuru/lessampler/actions/workflows/cmake-windows.yml) | [![CMake macOS](https://github.com/YuzukiTsuru/lessampler/actions/workflows/cmake-macos.yml/badge.svg)](https://github.com/YuzukiTsuru/lessampler/actions/workflows/cmake-macos.yml) |

## Architecture

The source is organized into explicit layers: `App` → `Engine` → `Vocoder` →
`Config`/`Audio` → `Core`, with `Host/UTAU` as an adapter at the edge. The
engine separates note-independent analysis (`VocoderBackend::Prepare`, cached
in memory and on disk) from per-note synthesis, so a session can reuse a sample
without touching disk again. See [ARCHITECTURE.md](ARCHITECTURE.md) for the
module map, cache contracts, threading rules, and the backend extension
checklist.

## Build from source

Requirements:

- CMake 3.30 or newer. CMake 3.31.x is recommended; CMake 4.x is supported through the compatibility shim in the root `CMakeLists.txt`.
- A C++26 compiler: GCC 15+, Clang 19+, or MSVC driven with `/std:c++latest` (the Windows preset uses MSVC in latest mode). AppleClang and MSVC are not mapped to C++26 by CMake itself.
- Git. All dependencies under `lib/` are submodules; initialize them before configuring.

```bash
git clone --recursive https://github.com/YuzukiTsuru/lessampler
cd lessampler

# Linux / macOS with GCC 15
cmake --preset release-gcc
cmake --build --preset release-gcc -j
ctest --preset release-gcc
```

The executable is written to `build/<preset>/bin/lessampler`; its `lessampler.less` is created next to it.

Other presets: `debug-gcc`, `asan-gcc`, `ci-gcc`, `ci-llsm` (GCC with the optional LLSM backend), `neural-gcc` (GCC with the optional neural backend), `ci-clang`, `ci-msvc`. On Windows:

```bat
cmake --preset ci-msvc
cmake --build --preset ci-msvc -j
ctest --preset ci-msvc
```

Do not use `--depth 1` for the clone. The submodules are pinned to exact commits that are not necessarily at the tip of their branches.

### Vocoder backends

WORLD is always built. The optional LLSM backend uses libllsm2 and its ciglet dependency, both of which live under `lib/` as submodules:

```bash
cmake --preset release-gcc -DLESSAMPLER_ENABLE_LLSM=ON
cmake --build --preset release-gcc -j
ctest --preset release-gcc --output-on-failure
```

Then select it in `lessampler.less`:

```toml
[audio_model]
backend = "llsm"   # "world" or "llsm"
```

### Release packages

CI builds the maximal configuration on Ubuntu, macOS, and Windows: WORLD plus every optional backend compiled on that platform. Tagged pushes (`v*`) run `.github/workflows/release.yml`, which uploads `lessampler-<version>-<os>-<arch>` archives and SHA-256 checksums to GitHub Releases.

Build the same archive locally with:

```bash
scripts/package-release.sh          # GCC preset max-gcc
scripts/package-release.ps1         # MSVC preset ci-msvc-max
```

The archives contain the executable, ONNX Runtime when the neural backend is enabled, the required OpenMP runtime on Linux, README/LICENSE, and third-party license files. Model weights are not bundled. To download the latest release for the current machine:

```bash
scripts/download-release.sh
```


`libllsm2` is GPLv3, so a binary built with `LESSAMPLER_ENABLE_LLSM=ON` is GPLv3. The default build keeps the project's LGPL-3.0 licensing. The WORLD `.lessaudio` cache is only used by the WORLD backend; switching backends does not invalidate it.

Both backends use the same `[f0]` settings. LLSM is not F0-free: its harmonic/noise model and synthesis all require an F0 contour. When the layer-1 cache misses, `LlsmBackend` runs the shared DIO/HARVEST + StoneMask front end and passes that contour to `llsm_analyze`; libllsm2's own refinement is disabled because StoneMask already ran. `f0_mode`, floors, speed, and frame period therefore affect both WORLD and LLSM, and changing them invalidates `.lessllsm`.

### Neural backend

The optional `neural` backend keeps WORLD for analysis and the UTAU time/pitch transform, then replaces the final synthesis stage with an NHVSing-compatible ONNX vocoder. This removes WORLD's buzzy excitation/phase artifacts while keeping the voicebank's spectral envelope and the note's F0 contour:

```
WORLD analysis -> UTAU transform -> preliminary waveform
               -> 128-bin ln-mel + F0 -> NHVSing -> waveform
```

Build it with:

```bash
cmake --preset neural-gcc
cmake --build --preset neural-gcc -j
ctest --preset neural-gcc --output-on-failure
```

On Windows, use `cmake --preset ci-msvc -DLESSAMPLER_ENABLE_NEURAL=ON` (or add the option to another preset). CMake downloads the pinned ONNX Runtime NuGet package and unpacks the headers and native library into `lib/onnxruntime`; that directory is gitignored. MSVC therefore needs no manual include/lib configuration. The first configure downloads about 176 MB (the package carries several platforms) and reuses the cached package afterwards. Set `ONNXRUNTIME_ROOT` only to use an existing ONNX Runtime installation instead.

The model itself is user-supplied because its weights are not redistributable under the project license. A suitable model is [NHVSing](https://github.com/wavtechyukky/NHVSing) V3/V3.3 (`exported_models/v3_3/nhv_v3_3.onnx`, about 7.8 MB, 0.478 M parameters). NHVSing code is MIT; the published weights are non-commercial and must be downloaded from the upstream project. Point `[neural] model_path` at the ONNX file:

```toml
[audio_model]
backend = "neural"

[neural]
model_path = "/path/to/nhv_v3_3.onnx"
threads = 4
target_rms = 0.111
```

The model is fixed at 44.1 kHz; other voicebank rates are resampled for feature extraction and back to the source rate. ONNX Runtime is used instead of ncnn because the exported NHVSing graph relies on dynamic `Scan`, `CumSum`, and random-excitation operators that `onnx2ncnn` does not import. I verified that both NHVSing and OpenVPI NSF-HiFiGAN ONNX graphs fail `onnx2ncnn` conversion for this reason; a ncnn path would need a feedforward model or a C++ port of the DSP excitation.

## Configuration

`lessampler.less` is created next to the executable on first run. It is a TOML file:

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
f0_mode = "HARVEST"
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

Config files now use the `.less` suffix. An old `lessampler.fex` (TOML or INI) is migrated automatically; the old file is kept as `lessampler.fex.bak`. Changing `[audio_model]`, `[f0]`, or `[ap]` settings invalidates the `.lessaudio` and `.lessllsm` caches; the next run regenerates them.

## Model cache

The WORLD backend stores analysis results next to each WAV as a `.lessaudio` file (format v2). The file records the analysis settings and the source WAV size/mtime, so either a configuration change or a modified sample invalidates the cache. Old v1 files are rejected and regenerated.

The LLSM backend has its own `.lessllsm` layer-1 cache. It is keyed by the same configuration fingerprint and source size/mtime, and it is independent of the UTAU note parameters. The first note rendered from a sample runs F0 estimation, LLSM analysis, and the layer-1 conversion; later notes loading the same sample skip all three. A missing or corrupt cache falls back to a full analysis and is rewritten. Cache writes are best-effort, so a read-only voicebank still renders correctly.

## Performance

- WORLD model generation splits the D4C frame loop across eight fixed chunks. The partition is independent of the machine's core count, so repeated analyses are reproducible. On the reference machine this cut a 44.1 kHz, one-second sample's D4C stage from roughly 0.45 s to about 0.03 s.
- In a 44.1 kHz UST benchmark, the LLSM layer-1 cache made warm note rendering roughly 10× faster: about 45 ms per note instead of 0.5–0.9 s. The first render still pays the analysis cost. Warm WORLD note rendering is in the same range (tens of milliseconds per note).
- The neural backend is a quality mode, not a speed mode. A warm one-second note measured about 170 ms on the reference machine: roughly 35 ms for the preliminary WORLD synthesis, 11 ms for mel extraction, 90 ms for ONNX inference with four threads, and 20–40 ms to load the 7.8 MB model. A normal UTAU host starts a fresh process per note, so the model load is paid per note.
- `WavIO::ProbeWav` reads only the WAV header for UTAU length calculations; the CLI never loads the PCM. LLSM decodes it only when its layer-1 cache misses.
- `[f0] f0_mode = "DIO"` is substantially faster than `"HARVEST"` for first-time analysis. The default remains `HARVEST` for quality.
- Enable compile-time stage timings with `-DLESSAMPLER_TIME_EVAL=ON`; the `[TIME]` lines report every analysis, cache, transform, and synthesis stage.
- GPU offload was evaluated and is not used. UTAU starts a fresh resampler process for every note; Vulkan context creation alone measured about 0.7 s on the reference machine, far above the tens-of-milliseconds warm render. GPU compute would require a persistent batch/server process or a different host protocol.

## Rendering a UST

`scripts/render_ust.py` is a small UTAU host: it reads a `.ust`, selects the closest `oto.ini` entry for each note, invokes lessampler once per note, and mixes the results at their tick positions. If `data/` contains a UST archive and a voicebank archive, the wrapper extracts both and renders the main UST:

```bash
./scripts/render_data.sh
# output: build/data-render/main.wav
```

Or point it at extracted files:

```bash
python3 scripts/render_ust.py \
  --ust path/to/song.ust \
  --voicebank path/to/voicebank \
  --output build/song.wav \
  --lessampler build/max-gcc/bin/lessampler
```

The host applies note pitch, tempo, volume, modulation, and short fades. It does not implement UTAU's full wavtool semantics yet: envelopes, crossfades, preutterance/overlap placement, and the PBS/PBY/PBW pitch-bend curve are not applied, so treated notes are rendered with their nominal pitch. Missing breath aliases such as `x` are skipped.

## Tests

Tests are registered with CTest. Run the whole suite with:

```bash
ctest --preset release-gcc --output-on-failure
```

The sanitizer preset builds the same tests with AddressSanitizer and UndefinedBehaviorSanitizer:

```bash
cmake --preset asan-gcc
cmake --build --preset asan-gcc -j
ctest --preset asan-gcc --output-on-failure
```

## Special Thanks

[@shine5402](https://github.com/shine5402)  
[@hyperzlib](https://github.com/hyperzlib)

## License

```
 Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.

  This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License v3.0 as published by
 the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

  You should have received a copy of the GNU Lesser General Public License v3.0
along with this program. If not, see <http://www.gnu.org/licenses/>.
```

The default build is LGPL-3.0. Builds configured with `-DLESSAMPLER_ENABLE_LLSM=ON` link the GPLv3 libllsm2 library and are therefore distributed under GPLv3.

The optional neural backend links ONNX Runtime (MIT). It does not bundle model weights; NHVSing's released weights are non-commercial and remain under the upstream terms.
