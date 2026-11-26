---
title: "Rendering a UST"
description: "The wavtool mode, the UST host, placement and envelopes."
---

`scripts/render_ust.py` is a UTAU host. It reads a `.ust`, selects the closest `oto.ini` entry for each note, invokes `lessampler` once per note, and assembles the buffers with the same binary in wavtool mode.

## Wavtool mode

`lessampler` is both the resampler and the concatenation host. A UTAU resampler call passes at most 13 positional arguments; the classic wavtool protocol passes exactly 15, so the binary selects wavtool mode from the argument count. A plan file selects the batch mode explicitly:

```text
lessampler <out.wav> <in.wav> <skip_ms> <length_ms>
           <p1> <p2> <p3> <v1> <v2> <v3> <v4> <overlap> <p4> <p5> <v5>
lessampler --plan plan.toml
```

Wavtool mode does not load `lessampler.less`, so a broken analysis configuration cannot stop a UTAU host from concatenating. The host places each rendered note at `note_start - preutterance`, applies the UST envelope over the used segment, and mixes overlapping tails with a seek-back. It honours UST `Velocity`, `PreUtterance`/`VoiceOverlap`, `prefix.map` pitch folders, the oto cutoff, and velocity-dependent preutterance adjustment. The batch plan places segments on an absolute timeline and mixes once in `double`, avoiding repeated 16-bit read-modify-write passes.

The envelope is piecewise linear:

```
0 → p1 → p1+p2 → p1+p2+p5 → L-p4-p3 → L-p4 → L
0 → v1 → v2     → v5        → v3       → v4    → 0
```

`%` in a UST envelope is the overlap placeholder. PBM pitch-bend shapes are not applied. UST `VBR` vibrato is applied on the note tail; plain long notes receive automatic vibrato and adjacent plain notes receive automatic portamento unless disabled. See [Expression and voice color](expression.md).

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
  --polish
```

`--wavtool` overrides the concatenation binary and defaults to the `--lessampler` binary. `--polish` adds whole-song neural resynthesis and keeps the raw mix as `<output>-mix.wav`. `--feature` instead assembles per-note mel/F0 features and runs the vocoder once; it is mutually exclusive with `--polish`. Without either, the wavtool overlap is the final stage.

`--master` selects the final tone and level preset: `none`, `natural` (limiter only), `warm` (default), or `power`. Names are case-insensitive and the preset factory validates them; an invalid `master` in a plan fails at parse time. The warm chain adds a 40 Hz high-pass, a low shelf, a presence peak, gentle compression, parallel saturation, and a -1 dBFS look-ahead limiter. Use `--master none` to compare against the old raw mix.

`--expression <file.toml>` configures automatic breath, vibrato, and portamento. `--auto-breath`/`--auto-vibrato`/`--auto-portamento` and their `--no-...` forms override the file or built-in defaults. The features only fill gaps: an explicit UST `VBR`, `PBS`/`PBW`/`PBY`, or breath marker always wins. Voice color travels in UST `Flags` through `[style]` in `lessampler.less` or `<voicebank>/lessvoice.less`; see [Expression and voice color](expression.md).

`lessampler --f0 <in.wav> <out.csv>` exports the selected F0 estimator's track as `time_ms,f0_hz` for external analysis; `[f0] estimator` and `[audio_model] backend` control which estimator `auto` resolves to.

## Limitations

- Explicit breath notes (`x`, `息`, `吸`, `吐`, `・`, `breath`, or an alias lyric containing `R息`/`R吐`) are rendered from a matching breath alias when one exists; a voicebank without a breath sample gets synthesized filtered noise.
- PBM pitch-bend shapes are not applied. UST `VBR` vibrato is applied on the note tail; automatic vibrato and portamento are enabled by default but skip notes with explicit UST curves.
- `--feature` and `--polish` re-estimate features from the note WAVs. Per-note voice color is baked into those WAVs but may be weakened by the re-render.
- A fixed/consonant region longer than the wavtool segment is capped to keep at least 30 ms of vowel; the host prints a warning for each capped note.
- The wavtool does not phase-align segments; the overlap crossfade is the classic behaviour.
- AutoAMP normalizes each rendered note to `0.5 * volume`, so relative dynamics are flattened; the master chain restores macro dynamics.
- Whole-song inference holds the full timeline in memory; very long songs need chunking and crossfading.
