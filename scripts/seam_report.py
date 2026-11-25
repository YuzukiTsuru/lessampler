#!/usr/bin/env python3
"""Report energy dips around UST note starts.

This is a verification helper, not part of the render pipeline. It compares a
short window centred on each note start with the windows on either side and
counts boundaries whose centre is much quieter than both sides.
"""
from __future__ import annotations

import argparse
import math
import struct
import sys
import wave
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from render_ust import TICKS_PER_QUARTER, parse_ust  # noqa: E402


def read_mono(path: Path) -> tuple[int, list[float]]:
    with wave.open(str(path), "rb") as handle:
        fs = handle.getframerate()
        channels = handle.getnchannels()
        width = handle.getsampwidth()
        frames = handle.readframes(handle.getnframes())
    if width != 2:
        raise SystemExit(f"{path}: only 16-bit PCM is supported")
    values = struct.unpack(f"<{len(frames) // 2}h", frames)
    if channels == 1:
        return fs, [value / 32768.0 for value in values]
    return fs, [
        sum(values[frame * channels : (frame + 1) * channels]) / (channels * 32768.0)
        for frame in range(len(values) // channels)
    ]


def rms(samples: list[float], start: int, end: int) -> float:
    start = max(0, start)
    end = min(len(samples), end)
    if end <= start:
        return 0.0
    return math.sqrt(sum(value * value for value in samples[start:end]) / (end - start))


def note_starts(ust: Path) -> list[float]:
    tempo, _, notes = parse_ust(ust)
    tick = 0
    starts: list[float] = []
    for note in notes:
        try:
            length = int(note.get("Length", "0"))
        except ValueError:
            continue
        if length <= 0:
            continue
        start_ms = tick * 60_000.0 / (tempo * TICKS_PER_QUARTER)
        tick += length
        lyric = note.get("Lyric", "")
        if lyric and lyric.upper() != "R":
            starts.append(start_ms)
    return starts


def longest_silence(samples: list[float], threshold: float = 1.0 / 32768.0) -> int:
    best = 0
    run = 0
    for value in samples:
        run = run + 1 if abs(value) <= threshold else 0
        best = max(best, run)
    return best


def report(wav: Path, ust: Path) -> None:
    fs, samples = read_mono(wav)
    starts = note_starts(ust)
    centre = int(0.010 * fs)
    side = int(0.050 * fs)
    gaps = 0
    dips = 0
    for start_ms in starts:
        frame = int(start_ms * fs / 1000.0)
        if frame <= side or frame + side >= len(samples):
            continue
        middle = rms(samples, frame - centre, frame + centre)
        left = rms(samples, frame - side, frame - centre)
        right = rms(samples, frame + centre, frame + side)
        reference = min(left, right)
        if middle < 0.25 * reference and reference > 0.001:
            dips += 1
        if middle < 1e-4:
            gaps += 1
    silence = longest_silence(samples)
    print(f"{wav}: {len(samples) / fs:.3f}s, {len(starts)} note starts, "
          f"centre dips {dips}, near-silent centres {gaps}, "
          f"longest near-zero run {silence / fs * 1000:.1f} ms")


def main() -> int:
    parser = argparse.ArgumentParser(description="Report UST note-boundary dips")
    parser.add_argument("--ust", required=True, type=Path)
    parser.add_argument("wav", nargs="+", type=Path)
    args = parser.parse_args()
    for wav in args.wav:
        report(wav, args.ust)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
