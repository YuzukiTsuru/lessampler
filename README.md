<div align="center"><img width="80" src="assets/icon_128.gif" alt="lessampler logo"></div>
<h1 align="center"><b>lessampler</b></h1>
<p align="center">
  lessampler is a Singing Voice Synthesizer [WIP]
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
  - [ ] Voice Conversion and Audio Style Transfer
  - [ ] Noise reduction enhanced MOS

## Build Status

| Ubuntu | Windows | macOS |
| --- | --- | --- |
| [![CMake Ubuntu](https://github.com/YuzukiTsuru/lessampler/actions/workflows/cmake-ubuntu.yml/badge.svg)](https://github.com/YuzukiTsuru/lessampler/actions/workflows/cmake-ubuntu.yml) | [![CMake Windows](https://github.com/YuzukiTsuru/lessampler/actions/workflows/cmake-windows.yml/badge.svg)](https://github.com/YuzukiTsuru/lessampler/actions/workflows/cmake-windows.yml) | [![CMake macOS](https://github.com/YuzukiTsuru/lessampler/actions/workflows/cmake-macos.yml/badge.svg)](https://github.com/YuzukiTsuru/lessampler/actions/workflows/cmake-macos.yml) |

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

Other presets: `debug-gcc`, `asan-gcc`, `ci-gcc`, `ci-llsm` (GCC with the optional LLSM backend), `ci-clang`, `ci-msvc`. On Windows:

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

Then select it in `lessampler.fex`:

```toml
[audio_model]
backend = "llsm"   # "world" or "llsm"
```

`libllsm2` is GPLv3, so a binary built with `LESSAMPLER_ENABLE_LLSM=ON` is GPLv3. The default build keeps the project's LGPL-3.0 licensing. The WORLD `.lessaudio` cache is only used by the WORLD backend; switching backends does not invalidate it.

## Configuration

`lessampler.fex` is created next to the executable on first run. It is a TOML file:

```toml
[config]
version = "git-hash"
debug = false

[audio_model]
backend = "world"           # "world" or "llsm"
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
```

Older INI-style `lessampler.fex` files are migrated automatically and the original is kept as `lessampler.fex.bak`. Changing `[audio_model]`, `[f0]`, or `[ap]` settings invalidates the `.lessaudio` and `.lessllsm` caches; the next run regenerates them.

## Model cache

The WORLD backend stores analysis results next to each WAV as a `.lessaudio` file (format v2). The file records the analysis settings and the source WAV size/mtime, so either a configuration change or a modified sample invalidates the cache. Old v1 files are rejected and regenerated.

The LLSM backend has its own `.lessllsm` layer-1 cache. It is keyed by the same configuration fingerprint and source size/mtime, and it is independent of the UTAU note parameters. The first note rendered from a sample runs F0 estimation, LLSM analysis, and the layer-1 conversion; later notes loading the same sample skip all three. A missing or corrupt cache falls back to a full analysis and is rewritten. Cache writes are best-effort, so a read-only voicebank still renders correctly.

## Performance

- WORLD model generation splits the D4C frame loop across eight fixed chunks. The partition is independent of the machine's core count, so repeated analyses are reproducible. On the reference machine this cut a 44.1 kHz, one-second sample's D4C stage from roughly 0.45 s to about 0.03 s.
- In a 44.1 kHz UST benchmark, the LLSM layer-1 cache made warm note rendering roughly 10× faster: about 45 ms per note instead of 0.5–0.9 s. The first render still pays the analysis cost. Warm WORLD note rendering is in the same range (tens of milliseconds per note).
- `WavIO::ProbeWav` reads only the WAV header for UTAU length calculations; the CLI never loads the PCM. LLSM decodes it only when its layer-1 cache misses.
- `[f0] f0_mode = "DIO"` is substantially faster than `"HARVEST"` for first-time analysis. The default remains `HARVEST` for quality.
- Enable compile-time stage timings with `-DLESSAMPLER_TIME_EVAL=ON`; the `[TIME]` lines report every analysis, cache, transform, and synthesis stage.

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
