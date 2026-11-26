# audio_compare

Build a synchronized side-by-side listening page for several render WAVs. The page has a shared transport, solo/mute per track, per-track volume, A–B loop, waveform seek, and quick jumps.

## Quick start

From the repository root:

```bash
python3 tools/audio_compare/compare.py \
  --manifest tools/audio_compare/manifest.json \
  --output build/audio-compare
```

Open `build/audio-compare/index.html`. If the browser refuses `file://` media, serve the output:

```bash
python3 -m http.server 8000
# open http://localhost:8000/build/audio-compare/index.html
```

Without `soundfile`, the tool copies WAVs and the page references `audio/*.wav`. To encode the smaller 128 kbps MP3 version, use the quality venv or install the optional dependency:

```bash
build/quality-venv/bin/python tools/audio_compare/compare.py \
  --manifest tools/audio_compare/manifest.json \
  --output build/audio-compare
```

`soundfile` provides the MP3 encoder. `numpy` is not required by this tool.

## Manifest

`manifest.json` describes the page and the tracks. `root` is resolved relative to the manifest file; each `source` is relative to that root.

```json
{
  "title": "lessampler 音质对比",
  "heading": "音质对比台",
  "kicker": "lessampler / listening bench",
  "lede": "Short page description.",
  "root": "../..",
  "facts": ["244.964 s", "44.1 kHz · mono"],
  "stats": [
    {"label": "整曲 SingMOS · polish", "before": "2.68", "after": "3.18"}
  ],
  "notes_html": "<ul><li>How to listen.</li></ul>",
  "tracks": [
    {
      "id": "warm",
      "name": "warm",
      "tag": "默认",
      "sub": "WORLD wavtool · warm master",
      "source": "data/render/main-final.wav",
      "color": "--t-warm",
      "peak": "-3.00",
      "rms": "-17.17",
      "dips": "9",
      "mos": "2.70",
      "high": "2.99"
    }
  ]
}
```

- `id` is used as the audio filename and must match `[A-Za-z0-9_-]+`.
- `color` accepts a CSS variable name such as `--t-warm` (the template defines six) or any CSS color.
- `peak` and `rms` are optional; the tool computes them when omitted.
- `dips`, `mos`, and `high` are display-only. Use `"—"` when a value was not measured.
- Waveform peaks, duration, and the audio file extension are generated; do not put them in the manifest.
- The template maps the first track to the playback clock, so put the reference render first.

## Options

```text
--manifest PATH   manifest (default: tools/audio_compare/manifest.json)
--output PATH     output directory (default: build/audio-compare)
--format auto     auto, mp3, or wav
--buckets N       waveform buckets per track (default: 1400)
```

The generated `index.html` and `audio/` directory are build output. The manifest and template are the source; generated audio is not tracked.
