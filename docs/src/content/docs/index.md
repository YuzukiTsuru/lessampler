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
      link: getting-started/build/
      icon: right-arrow
      variant: primary
    - text: GitHub
      link: https://github.com/YuzukiTsuru/lessampler
      icon: external
      variant: minimal
---

lessampler renders singing from UTAU `.ust` projects. It implements the resampler protocol, ships its own concatenation host, and can synthesize through WORLD, LLSM, or an optional NHVSing-compatible neural vocoder. F0 is a separate selectable axis: Harvest, DIO, LLSM refinement, or RMVPE. Analysis is cached next to each voicebank sample, so warm notes render in tens of milliseconds.

## Start here

| What you want to do | Where to go |
| --- | --- |
| Build the project | [Build](getting-started/build/) |
| Configure a backend | [Configuration](getting-started/configuration/) |
| Render a UST end to end | [Rendering a UST](guides/rendering/) |
| Add breath, vibrato, and voice color | [Expression and voice color](guides/expression/) |
| Understand the neural paths | [Neural backends](guides/neural/) |
| Measure render quality | [Quality evaluation](guides/quality/) |
| Follow one note through the engine | [Processing pipeline](processing/) |
| Choose an F0 estimator | [F0 estimation](processing/f0/) |
| Read the module map | [Architecture details](architecture/) |

## What is in the box

- WORLD analysis and synthesis with `.lessaudio` sidecars.
- Selectable F0: Harvest, DIO, LLSM refinement, and optional RMVPE.
- Optional LLSM and NHVSing-compatible neural backends.
- Automatic breath, vibrato, and portamento in the UST host, plus clear/cry/growl voice colors with per-note morph.
- `lessampler`, a single binary with the resampler, the UTAU wavtool and the CLI.
- Whole-song neural polish and feature-level neural assembly.
- A standardized SingMOS-Pro and objective quality report.

The source lives at [YuzukiTsuru/lessampler](https://github.com/YuzukiTsuru/lessampler). Release archives for Linux, macOS and Windows are on the [releases page](https://github.com/YuzukiTsuru/lessampler/releases).
