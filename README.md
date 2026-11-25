<div align="center"><img width="80" src="assets/icon_128.gif" alt="lessampler logo"></div>
<h1 align="center"><b>lessampler</b></h1>
<p align="center">
  lessampler is a Singing Voice Synthesizer
</p>

[![forthebadge](https://forthebadge.com/images/badges/contains-cat-gifs.svg)](https://forthebadge.com)

## Download

***Currently lessampler is still under development, there are many bugs that need to be fixed, but welcome to participate in the test.***

You can find the alpha version at [Release](https://github.com/YuzukiTsuru/lessampler/releases)

## Features

- Multiple models
  - [x] World
  - [x] llsm (optional build)
  - [x] NHVSing-compatible neural vocoder (optional build)
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
  - [x] Whole-song neural resynthesis
  - [x] Feature-level neural assembly
  - [ ] Voice Conversion and Audio Style Transfer
  - [ ] Noise reduction enhanced MOS
- Quality Evaluation
  - [x] Standardized SingMOS-Pro MOS report
  - [x] F0, seam, silence and level metrics

## Build status

| Ubuntu | Windows | macOS |
| --- | --- | --- |
| [![CMake Ubuntu](https://github.com/YuzukiTsuru/lessampler/actions/workflows/cmake-ubuntu.yml/badge.svg)](https://github.com/YuzukiTsuru/lessampler/actions/workflows/cmake-ubuntu.yml) | [![CMake Windows](https://github.com/YuzukiTsuru/lessampler/actions/workflows/cmake-windows.yml/badge.svg)](https://github.com/YuzukiTsuru/lessampler/actions/workflows/cmake-windows.yml) | [![CMake macOS](https://github.com/YuzukiTsuru/lessampler/actions/workflows/cmake-macos.yml/badge.svg)](https://github.com/YuzukiTsuru/lessampler/actions/workflows/cmake-macos.yml) |

## Quick start

Requires CMake 3.30+, a C++26 compiler (GCC 15+, Clang 19+, or MSVC in latest mode), and initialized submodules. See [docs/src/content/docs/build.md](docs/src/content/docs/build.md) for every preset and optional dependency.

```bash
git clone --recursive https://github.com/YuzukiTsuru/lessampler
cd lessampler

cmake --preset max-gcc
cmake --build --preset max-gcc -j
ctest --preset max-gcc --output-on-failure
```

The executable lands in `build/max-gcc/bin/lessampler`; it is the resampler, the wavtool and the CLI. `lessampler.less` is created next to the binary on first run.

## Backends

WORLD is always built. LLSM (`libllsm2`, GPLv3) and the neural NHVSing backend are optional; the maximal preset enables both. The neural path can render per note, resynthesize a whole assembled mix (`--polish`), or assemble per-note mel/F0 features and run the vocoder once (`--feature-plan`). See [docs/neural.md](docs/neural.md).

## Configuration

`lessampler.less` is TOML. `[audio_model] backend` is `"world"`, `"llsm"`, or `"neural"`; the neural backend also needs `[neural] model_path`. Caches and performance notes are in [docs/configuration.md](docs/configuration.md).

## Rendering a UST

`scripts/render_ust.py` renders a `.ust` with `lessampler` and assembles the notes with the same binary in wavtool mode. The host applies preutterance/overlap placement, UST envelopes, velocity, `prefix.map` pitch folders, and the oto cutoff:

```bash
./scripts/render_data.sh

python3 scripts/render_ust.py \
  --ust path/to/song.ust \
  --voicebank path/to/voicebank \
  --output build/song.wav \
  --lessampler build/max-gcc/bin/lessampler \
  --polish
```

`--feature` selects feature-level neural assembly instead; `--polish` runs whole-song resynthesis after the wavtool. The wavtool mode is selected by the classic 15-argument UTAU form or by `--plan`; the protocol, plan format, placement geometry, and remaining limitations are in [docs/rendering.md](docs/rendering.md).

## Documentation

The documentation is published at <https://yuzukitsuru.github.io/lessampler/>. The site and source pages are in [`docs/`](docs/):

- [Build](docs/src/content/docs/build.md)
- [Configuration](docs/src/content/docs/configuration.md)
- [Rendering a UST](docs/src/content/docs/rendering.md)
- [Neural backends](docs/src/content/docs/neural.md)
- [Quality evaluation](docs/src/content/docs/quality.md)
- [Processing pipeline](docs/src/content/docs/pipeline.md)
- [Architecture details](docs/src/content/docs/architecture-details.md)

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

The default build is LGPL-3.0. Builds configured with `-DLESSAMPLER_ENABLE_LLSM=ON` link the GPLv3 libllsm2 library and are distributed under GPLv3. The optional neural backend links ONNX Runtime (MIT) and does not bundle model weights.
