---
title: "Expression and voice color"
description: "Automatic breath, vibrato, portamento, and clear/cry/growl voice styles."
---

lessampler separates two kinds of expression:

- The Python UST host adds automatic breath, vibrato, and portamento from the song context. These features fill gaps and never overwrite explicit UST tuning.
- The resampler applies a voice color per note: `clear`, `cry`, or `growl`. A note can morph from one color to another inside its own duration.

## Voice color

A voice color has three additive weights, each from 0 to 1:

| Weight | Name | Effect |
| --- | --- | --- |
| `clear` | clear / 清亮 | up to -6 dB aperiodic noise, +0.5 dB harmonic energy; less breathy |
| `cry` | cry / 哭声 | +1 dB harmonic energy, a low-harmonic tension tilt, +0.5 dB noise |
| `growl` | growl / 嘶吼 | 27 Hz amplitude modulation and bounded tanh saturation after synthesis |

`clear` and `cry` are spectral operations in WORLD and LLSM. `growl` is a backend-independent post-shaper in `RenderEngine`, so WORLD, LLSM, and the per-note neural path all hear the same growl. A note with all weights at zero follows the natural path unchanged.

### Voicebank default

Set a default in `lessampler.less`, or override it for one voicebank in `<voicebank>/lessvoice.less`:

```toml
[style]
default = "natural"   # natural, clear, cry, growl
amount = 1.0          # scales the default weight
morph_ms = 0.0        # 0 = spread start>end over the whole note
```

When a note carries no style flag, `RenderEngine` applies this default. Voice style is synthesis-only and is not part of the analysis fingerprint, so changing it never rebuilds a `.lessaudio` or `.lessllsm` cache.

### Per-note flags

UTAU `Flags` carry the per-note color. The host passes the string through unchanged, and the resampler parses uppercase `C`, `Y`, and `G`:

```text
C100              clear only
Y60G40            cry 60 + growl 40
C100>G60          start clear, end growl, across the note
C50Y50>C100       start clear+cry, end clear
C0Y0G0            force natural, overriding the voicebank default
```

The number is a percentage. A missing number means 100. A `>` separates the start weights from the end weights; without it, start and end are equal. A style flag always replaces the voicebank default for that note, including `C0Y0G0`.

Morph timing comes from `[style] morph_ms`. Zero means the start color moves linearly to the end color over the whole note. A positive value reaches the end color after that many milliseconds and holds it for the rest of the note.

```bash
./build/max-gcc/bin/lessampler in.wav out.wav A4 100 'C100>G60' 0 1000 100 0 100 0 '!120' ""
```

## Automatic vibrato

The host synthesizes vibrato only when all of these are true:

- The note has no `VBR` field. A present `VBR`, even a zero or malformed one, is treated as a manual decision.
- The note has no `PBS`/`PBW`/`PBY` pitch curve.
- The note is at least `min_duration_ms` long (default 600 ms).

The default is a 180 ms delay, a 200 ms fade-in, 5.4 Hz, and 28 cents. It reuses the same pitch-encoding path as an explicit UST `VBR`, so `lessampler --f0` and the quality evaluation see the same target.

```bash
python3 scripts/render_ust.py --ust song.ust --voicebank bank --output out.wav --no-auto-vibrato
```

## Automatic portamento

Portamento is also a gap-fill. The host adds a short glide only when the note has no explicit pitch curve, the previous rendered note is within `max_gap_ms` (default 200 ms), the interval is at most `max_semitones` (default 12), and the note is long enough for the glide.

The shape holds the previous pitch, glides into the note over `glide_ms` (default 70 ms), then follows the note. Explicit `PBS`/`PBW`/`PBY` always wins. `--no-auto-portamento` restores the old fallback, which spreads the approach over the whole buffer head.

## Automatic breath

The host recognizes explicit breath notes by lyric: `x`, `息`, `吸`, `吐`, `・`, or `breath`, plus alias lyrics that contain `R息` or `R吐` such as `a R吐`. `R` stays a rest. Explicit breath notes are rendered even with `--no-auto-breath`, and they do not become the pitch context for the next sung note.

For an explicit alias lyric, the host uses that alias directly. Otherwise it looks at the nearest sung lyric and tries `<vowel> R息`, then `<vowel> R吐`, then a generic `R息`/`R吐`/`・`/`breath` entry. If no sample is usable at the render sample rate, it writes deterministic filtered noise as a fallback. The fallback is a placeholder, not a recorded breath.

Automatic insertion runs after preutterance handling, so it never changes the ACPT values of sung notes. A gap gets one breath when:

- the gap between rendered notes is at least `min_gap_ms` (default 180 ms);
- no explicit breath already occupies the gap;
- the next note is a sung note.

The breath length is the smallest of 60% of the gap, `max_ms`, and the room before the next note's plan position; a short sample can shorten it further. It is placed 20 ms before the next note's plan position. Automatic breath volume is `level` (default 0.30) of the normal note level, and its flags are fixed to `C0Y0G0` so a voicebank default style does not color it.

```toml
[breath]
enabled = true
min_gap_ms = 180
max_ms = 600
level = 0.30

[vibrato]
enabled = true
min_duration_ms = 600
delay_ms = 180
fade_ms = 200
rate_hz = 5.4
depth_cents = 28

[portamento]
enabled = true
max_gap_ms = 200
max_semitones = 12
glide_ms = 70
```

Save that as `expression.toml` and pass `--expression expression.toml`. The CLI switches `--auto-breath`, `--auto-vibrato`, and `--auto-portamento`, plus their `--no-...` forms, override the file and defaults.

## Limits

- `--feature` and `--polish` re-estimate F0 and mel from the note WAVs. The per-note style is already baked into those WAVs, but the re-render can weaken it. The wavtool path keeps the full per-note color.
- Clear lowers aperiodicity, which also changes the neural backend's unvoiced decision. That side effect is expected.
- Growl is a deterministic DSP effect, not a trained model. Its drive and modulation are bounded, and the master limiter is still the final guard.
- Automatic breath uses one segment per phrase gap. It does not alternate inhale/exhale or model a full breathing phrase.
- These features are single-voice expression. lessampler does not mix multiple singers or harmony tracks.
