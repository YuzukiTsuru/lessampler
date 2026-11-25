---
title: lessampler
description: lessampler is a Singing Voice Synthesizer
tableOfContents: false
hero:
  tagline: lessampler is a Singing Voice Synthesizer
  image:
    file: ../../assets/logo.png
    alt: lessampler
  actions:
    - text: Quick start
      link: build/
      icon: right-arrow
      variant: primary
    - text: GitHub
      link: https://github.com/YuzukiTsuru/lessampler
      icon: external
      variant: minimal
---

lessampler renders singing from UTAU `.ust` projects. It implements the resampler protocol, ships its own concatenation host, and can synthesize through WORLD, LLSM, or an optional NHVSing-compatible neural vocoder. Analysis is cached next to each voicebank sample, so warm notes render in tens of milliseconds.

## Start here

| What you want to do | Where to go |
| --- | --- |
| Build the project | [Build](build/) |
| Configure a backend | [Configuration](configuration/) |
| Render a UST end to end | [Rendering a UST](rendering/) |
| Understand the neural paths | [Neural backends](neural/) |
| Measure render quality | [Quality evaluation](quality/) |
| Follow one note through the engine | [Processing pipeline](pipeline/) |
| Read the module map | [Architecture details](architecture-details/) |

## What is in the box

- WORLD analysis and synthesis with `.lessaudio` sidecars.
- Optional LLSM and NHVSing-compatible neural backends.
- `lessampler`, a single binary with the resampler, the UTAU wavtool and the CLI.
- Whole-song neural polish and feature-level neural assembly.
- A standardized SingMOS-Pro and objective quality report.

The source lives at [YuzukiTsuru/lessampler](https://github.com/YuzukiTsuru/lessampler). Release archives for Linux, macOS and Windows are on the [releases page](https://github.com/YuzukiTsuru/lessampler/releases).
