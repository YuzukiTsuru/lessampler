---
title: "Rendering a UST"
description: "lesswavtool, the UST host, placement and envelopes."
---

`scripts/render_ust.py` is a UTAU host. It reads a `.ust`, selects the closest `oto.ini` entry for each note, invokes `lessampler` once per note, and assembles the buffers with `lesswavtool`.

## lesswavtool

`lesswavtool` is the concatenation half of the pipeline:

```text
lesswavtool <out.wav> <in.wav> <skip_ms> <length_ms>
            [p1 p2 p3 v1 v2 v3 v4 overlap p4 p5 v5]
lesswavtool --plan plan.toml
```

The host places each rendered note at `note_start - preutterance`, applies the UST envelope over the used segment, and mixes overlapping tails with a seek-back. It honours UST `Velocity`, `PreUtterance`/`VoiceOverlap`, `prefix.map` pitch folders, the oto cutoff, and velocity-dependent preutterance adjustment. The batch plan places segments on an absolute timeline and mixes once in `double`, avoiding repeated 16-bit read-modify-write passes.

The envelope is piecewise linear:

```
0 → p1 → p1+p2 → p1+p2+p5 → L-p4-p3 → L-p4 → L
0 → v1 → v2     → v5        → v3       → v4    → 0
```

`%` in a UST envelope is the overlap placeholder. PBM shapes and VBR are not implemented; the bundled data UST uses neither.

## Host usage

If `data/` contains a UST archive and a voicebank archive:

```bash
./scripts/render_data.sh
# output: build/data-render/main.wav

LESSAMPLER_FEATURE=1 ./scripts/render_data.sh   # feature-level neural assembly
LESSAMPLER_POLISH=1 ./scripts/render_data.sh    # wavtool mix + whole-song polish
```

Or point the host at extracted files:

```bash
python3 scripts/render_ust.py \
  --ust path/to/song.ust \
  --voicebank path/to/voicebank \
  --output build/song.wav \
  --lessampler build/max-gcc/bin/lessampler \
  --wavtool build/max-gcc/bin/lesswavtool \
  --polish
```

`--wavtool` defaults to the `lesswavtool` next to the lessampler binary. `--polish` adds whole-song neural resynthesis and keeps the raw mix as `<output>-mix.wav`. `--feature` instead assembles per-note mel/F0 features and runs the vocoder once; it is mutually exclusive with `--polish`. Without either, the wavtool overlap is the final stage.

`lessampler --f0 <in.wav> <out.csv>` exports the configured F0 track as `time_ms,f0_hz` for external analysis.

## Limitations

- 13 `x` breath notes are skipped because the bundled voicebank has no matching oto alias.
- PBM shapes and VBR are not applied.
- The wavtool does not phase-align segments; the overlap crossfade is the classic behaviour.
- AutoAMP normalizes each rendered note to `0.5 * volume`, so relative dynamics are flattened.
- Whole-song inference holds the full timeline in memory; very long songs need chunking and crossfading.
