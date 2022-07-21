<div align="center"><img width="80" src="assets/icon_128.gif" alt="lessampler logo"></div>
<h1 align="center"><b>lessampler</b></h1>
<p align="center">
  lessampler is a Singing Voice Synthesizer [WIP]
</p>

[![forthebadge](https://forthebadge.com/images/badges/contains-cat-gifs.svg)](https://forthebadge.com)

## Download
***Currently lesssampler is still under development, there are many bugs that need to be fixed, but welcome to participate in the test.***

You can find the alpha version at [Release](https://github.com/YuzukiTsuru/lessampler/releases)

## Feature
- Multiple models
  - [x] World
  - [ ] llsm
- Global Transform Control
  - [x] Precise Pitch Shift and Time Stretch
  - [x] FFT Size, Samples Number, Estimation Floor and so on
  - [ ] Support breath sound generation
  - [ ] Support timbre gender change
- Multiple Interactive Interfaces
  - [x] [UTAU](http://utau2008.xrea.jp/) Interface
  - [ ] [OpenUTAU](https://github.com/stakira/OpenUtau) Interface
  - [ ] Library Interface
- Nerual Network Vocoder
  - [ ] Voice Conversion and Audio Style Transfer
  - [ ] Noise reduction enhanced MOS



## Build Status

| Ubuntu                                                       | Windows                                                      | macOS                                                        |
| ------------------------------------------------------------ | ------------------------------------------------------------ | ------------------------------------------------------------ |
| [![CMake Ubuntu](https://github.com/YuzukiTsuru/lessampler/actions/workflows/cmake-ubuntu.yml/badge.svg)](https://github.com/YuzukiTsuru/lessampler/actions/workflows/cmake-ubuntu.yml) | [![CMake Windows](https://github.com/YuzukiTsuru/lessampler/actions/workflows/cmake-windows.yml/badge.svg)](https://github.com/YuzukiTsuru/lessampler/actions/workflows/cmake-windows.yml) | [![CMake macOS](https://github.com/YuzukiTsuru/lessampler/actions/workflows/cmake-macos.yml/badge.svg)](https://github.com/YuzukiTsuru/lessampler/actions/workflows/cmake-macos.yml) |

## Build from source

```
git clone --recursive --depth 1 https://github.com/YuzukiTsuru/lessampler
mkdir build && cd build
cmake .. && make -j
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
