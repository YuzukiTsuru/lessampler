#!/usr/bin/env python3
"""Build a synchronized side-by-side audio comparison page.

The manifest lists tracks as WAV files plus display metadata. This script
computes waveform peaks, optionally encodes 128 kbps MP3s, and writes a
self-contained HTML page into the output directory.
"""

from __future__ import annotations

import argparse
import array
import html
import json
import math
import re
import shutil
import sys
import wave
from pathlib import Path

try:
    import soundfile as sf
except ImportError:  # MP3 encoding is optional; WAV copy still works.
    sf = None


DEFAULT_BUCKETS = 1400
ID_PATTERN = re.compile(r"^[A-Za-z0-9_-]+$")


def db(value: float) -> float:
    return 20.0 * math.log10(max(value, 1e-12))


def read_wav_stdlib(path: Path) -> tuple[list[float], int, int]:
    """Return (mono samples in -1..1, sample rate, frames)."""
    with wave.open(str(path), "rb") as handle:
        channels = handle.getnchannels()
        width = handle.getsampwidth()
        rate = handle.getframerate()
        frames = handle.getnframes()
        if width != 2:
            raise ValueError(f"{path}: only 16-bit PCM WAV is supported without soundfile")
        raw = handle.readframes(frames)
    values = array.array("h")
    values.frombytes(raw)
    if channels == 1:
        return [value / 32768.0 for value in values], rate, frames
    mono: list[float] = []
    for frame in range(frames):
        base = frame * channels
        mono.append(sum(values[base + channel] for channel in range(channels)) / (channels * 32768.0))
    return mono, rate, frames


def read_wav(path: Path) -> tuple[list[float], int, int]:
    if sf is None:
        return read_wav_stdlib(path)
    data, rate = sf.read(str(path), dtype="float64", always_2d=True)
    if data.shape[1] == 1:
        mono = data[:, 0].tolist()
    else:
        mono = data.mean(axis=1).tolist()
    return mono, rate, int(data.shape[0])


def bucket_peaks(samples: list[float], buckets: int) -> list[float]:
    if not samples or buckets <= 0:
        return []
    step = max(1, len(samples) // buckets)
    peaks: list[float] = []
    for index in range(buckets):
        start = index * step
        if start >= len(samples):
            break
        end = min(len(samples), start + step)
        peak = 0.0
        for value in samples[start:end]:
            magnitude = abs(value)
            if magnitude > peak:
                peak = magnitude
        peaks.append(peak)
    maximum = max(peaks) if peaks else 0.0
    if maximum <= 0.0:
        return [0.0 for _ in peaks]
    return [round(value / maximum, 3) for value in peaks]


def encode_audio(source: Path, destination: Path, mono: list[float], rate: int, requested: str) -> str:
    """Write MP3 when possible; return the published relative extension."""
    if requested in ("auto", "mp3") and sf is not None:
        try:
            data, _ = sf.read(str(source), dtype="float64", always_2d=True)
            sf.write(str(destination.with_suffix(".mp3")), data, rate,
                     format="MP3", subtype="MPEG_LAYER_III")
            return ".mp3"
        except Exception as error:  # fall through to WAV
            print(f"[warn] MP3 encode failed for {source.name}: {error}", file=sys.stderr)
    shutil.copyfile(source, destination.with_suffix(".wav"))
    return ".wav"


def load_manifest(path: Path) -> tuple[dict, Path]:
    try:
        manifest = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise SystemExit(f"cannot read manifest {path}: {error}") from error
    if not isinstance(manifest, dict):
        raise SystemExit("manifest must be a JSON object")
    root = (path.parent / manifest.get("root", ".")).resolve()
    return manifest, root


def build(manifest_path: Path, output: Path, requested_format: str, buckets: int) -> None:
    manifest, root = load_manifest(manifest_path)
    tracks = manifest.get("tracks")
    if not isinstance(tracks, list) or not tracks:
        raise SystemExit("manifest has no tracks")

    output.mkdir(parents=True, exist_ok=True)
    audio_dir = output / "audio"
    audio_dir.mkdir(parents=True, exist_ok=True)

    page_tracks: list[dict] = []
    peaks: dict[str, list[float]] = {}
    duration = 0.0
    seen: set[str] = set()

    for track in tracks:
        if not isinstance(track, dict):
            raise SystemExit("each track must be an object")
        track_id = str(track.get("id", ""))
        if not ID_PATTERN.fullmatch(track_id):
            raise SystemExit(f"invalid track id: {track_id!r}")
        if track_id in seen:
            raise SystemExit(f"duplicate track id: {track_id}")
        seen.add(track_id)

        source = (root / str(track.get("source", ""))).resolve()
        if not source.is_file():
            raise SystemExit(f"track {track_id}: source not found: {source}")
        mono, rate, frames = read_wav(source)
        track_duration = frames / rate if rate else 0.0
        duration = max(duration, track_duration)

        peak = db(max((abs(value) for value in mono), default=0.0))
        active = [value for value in mono if abs(value) > 0.01]
        rms = db(math.sqrt(sum(value * value for value in active) / len(active))) if active else -120.0

        extension = encode_audio(source, audio_dir / track_id, mono, rate, requested_format)
        page_tracks.append({
            **{key: value for key, value in track.items() if key != "source"},
            "file": f"audio/{track_id}{extension}",
            "peak": track.get("peak", f"{peak:.2f}"),
            "rms": track.get("rms", f"{rms:.2f}"),
            "dips": track.get("dips", "—"),
            "mos": track.get("mos", "—"),
        })
        peaks[track_id] = bucket_peaks(mono, buckets)
        print(f"{track_id:10s} {track_duration:8.3f}s  {extension:4s}  {source}")

    page_data = {
        "title": manifest.get("title", "音频对比"),
        "heading": manifest.get("heading", manifest.get("title", "音频对比")),
        "kicker": manifest.get("kicker", "audio comparison"),
        "lede": manifest.get("lede", ""),
        "facts": manifest.get("facts", []),
        "stats": manifest.get("stats", []),
        "notes_html": manifest.get("notes_html", ""),
        "duration": round(duration, 3),
        "tracks": page_tracks,
        "peaks": peaks,
    }

    template_path = Path(__file__).with_name("template.html")
    template = template_path.read_text(encoding="utf-8")
    if "__DATA__" not in template:
        raise SystemExit(f"template is missing __DATA__: {template_path}")
    payload = json.dumps(page_data, ensure_ascii=False, separators=(",", ":")).replace("</", "<\\/")
    page = template.replace("__TITLE__", html.escape(str(page_data["title"]))).replace("__DATA__", payload)
    (output / "index.html").write_text(page, encoding="utf-8")
    print(f"Wrote {output / 'index.html'} ({duration:.3f}s, {len(page_tracks)} tracks)")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path,
                        default=Path("tools/audio_compare/manifest.json"),
                        help="track manifest (default: tools/audio_compare/manifest.json)")
    parser.add_argument("--output", type=Path, default=Path("build/audio-compare"),
                        help="output directory (default: build/audio-compare)")
    parser.add_argument("--format", choices=["auto", "mp3", "wav"], default="auto",
                        help="audio format; auto uses MP3 when soundfile is available")
    parser.add_argument("--buckets", type=int, default=DEFAULT_BUCKETS,
                        help=f"waveform buckets per track (default: {DEFAULT_BUCKETS})")
    args = parser.parse_args()
    build(args.manifest, args.output, args.format, max(80, args.buckets))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
