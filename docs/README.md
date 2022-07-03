<div align="center"><img width="80" src="assets/icon_128.gif" alt="lessampler logo"></div>
<h1 align="center">lessampler</h1>
<p align="center">lessampler is a Singing Voice Synthesizer</p>

# Overview

lessampler is a Singing Voice Synthesizer. It provides complete pitch shifting, time stretching and other functions. Support multiple interface calls such as UTAU, Library, and Shine.

## Build Status

| Ubuntu                                                       | Windows                                                      | macOS                                                        |
| ------------------------------------------------------------ | ------------------------------------------------------------ | ------------------------------------------------------------ |
| [![CMake Ubuntu](https://github.com/YuzukiTsuru/lessampler/actions/workflows/cmake-ubuntu.yml/badge.svg)](https://github.com/YuzukiTsuru/lessampler/actions/workflows/cmake-ubuntu.yml) | [![CMake Windows](https://github.com/YuzukiTsuru/lessampler/actions/workflows/cmake-windows.yml/badge.svg)](https://github.com/YuzukiTsuru/lessampler/actions/workflows/cmake-windows.yml) | [![CMake macOS](https://github.com/YuzukiTsuru/lessampler/actions/workflows/cmake-macos.yml/badge.svg)](https://github.com/YuzukiTsuru/lessampler/actions/workflows/cmake-macos.yml) |

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

# Download

***Currently lesssampler is still very unstable, there are many bugs that need to be fixed, but you are very welcome to participate in the test.***

You can find the alpha version at [Release](https://github.com/YuzukiTsuru/lessampler/releases)

# Contributing to lessampler

This section contains articles that are relevant to anyone who wants to contribute to lessampler and help improve the project.

It provides an introduction to contributing, along with a few ideas for how you can help.

## lessaudio file format

lessaudio is the audio source model format file used by lessampler, which contains the audio f0, spectrogram, aperiodicity and some basic parameters corresponding to them. The following table lists the index order and its function.

> ! Note: the current version of lessaudio only saves the data of lessaudiomodel, if there is any modification in the future, it will be explained here

| index | data | data size |
| ----- | ---- | --------- |
| 1     |      |           |
| 2     |      |           |
| 3     |      |           |
| 4     |      |           |
| 5     |      |           |
| 6     |      |           |
| 7     |      |           |
| 8     |      |           |
| 9     |      |           |

