#!/usr/bin/env python3
"""Render a UTAU .ust file with lessampler.

This is a small host, not a full UTAU wavtool replacement. It selects the
closest oto.ini entry for each note, applies the UST pitch-bend curve, invokes
lessampler once per note, and mixes the rendered notes at their UST tick
positions. Envelopes, overlaps, and PBM pitch shapes are intentionally not
applied yet.
"""
from __future__ import annotations

import argparse
import math
import re
import struct
import subprocess
import sys
import tempfile
import wave
from pathlib import Path

NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
NOTE_OFFSETS = {name: index for index, name in enumerate(NOTE_NAMES)}
PITCH_SUFFIX = re.compile(r"([A-G]#?-?\d)$")
NOTE_NAME = re.compile(r"([A-G]#?)(-?\d)")
TICKS_PER_QUARTER = 480
UTAU64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"


def decode_text(data: bytes) -> str:
    for encoding in ("utf-8-sig", "utf-8", "cp932", "shift_jis", "utf-16"):
        try:
            return data.decode(encoding)
        except UnicodeDecodeError:
            continue
    return data.decode("cp932", errors="replace")


def read_text(path: Path) -> str:
    return decode_text(path.read_bytes())


def note_to_midi(name: str) -> int:
    match = NOTE_NAME.fullmatch(name)
    if not match:
        raise ValueError(f"invalid note name: {name}")
    return (int(match.group(2)) + 1) * 12 + NOTE_OFFSETS[match.group(1)]


def midi_to_name(number: int) -> str:
    return f"{NOTE_NAMES[number % 12]}{number // 12 - 1}"


def strip_pitch(alias: str) -> tuple[str, str | None]:
    match = PITCH_SUFFIX.search(alias)
    if match is None:
        return alias, None
    return alias[: match.start()], match.group(1)


def parse_ust(path: Path) -> tuple[float, str, list[dict[str, str]]]:
    tempo = 120.0
    flags = ""
    notes: list[dict[str, str]] = []
    current: dict[str, str] = {}
    section: str | None = None

    for raw in read_text(path).splitlines():
        line = raw.strip()
        if not line or line.startswith(";"):
            continue
        if line.startswith("[") and line.endswith("]"):
            if section is not None and section.startswith("#") and section[1:].isdigit():
                notes.append(current)
            section = line[1:-1].strip()
            current = {}
            continue
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        key = key.strip()
        value = value.strip()
        if section == "#SETTING":
            if key == "Tempo":
                try:
                    tempo = float(value)
                except ValueError:
                    pass
            elif key == "Flags":
                flags = value
        elif section is not None and section.startswith("#") and section[1:].isdigit():
            current[key] = value

    if section is not None and section.startswith("#") and section[1:].isdigit():
        notes.append(current)
    return tempo, flags, notes


def parse_oto(voicebank: Path) -> list[dict[str, object]]:
    entries: list[dict[str, object]] = []
    for oto in sorted(voicebank.rglob("oto.ini")):
        folder_pitch = None
        match = NOTE_NAME.search(oto.parent.name)
        if match:
            try:
                folder_pitch = note_to_midi(match.group(0))
            except ValueError:
                folder_pitch = None

        for raw in read_text(oto).splitlines():
            line = raw.strip()
            if not line or line.startswith(";") or "=" not in line:
                continue
            wav_name, rest = line.split("=", 1)
            fields = rest.split(",")
            if len(fields) < 2:
                continue
            alias = fields[0].strip()
            try:
                offset = float(fields[1])
                consonant = float(fields[2]) if len(fields) > 2 and fields[2] else 0.0
                cutoff = float(fields[3]) if len(fields) > 3 and fields[3] else 0.0
                preutter = float(fields[4]) if len(fields) > 4 and fields[4] else 0.0
                overlap = float(fields[5]) if len(fields) > 5 and fields[5] else 0.0
            except ValueError:
                continue
            entries.append(
                {
                    "wav": oto.parent / wav_name.strip(),
                    "alias": alias,
                    "base": strip_pitch(alias)[0],
                    "pitch": folder_pitch,
                    "offset": offset,
                    "consonant": consonant,
                    "cutoff": cutoff,
                    "preutter": preutter,
                    "overlap": overlap,
                }
            )
    return entries


def select_entry(entries: list[dict[str, object]], lyric: str, midi: int) -> dict[str, object] | None:
    matches = [
        entry
        for entry in entries
        if entry["base"] == lyric or entry["alias"] == lyric
    ]
    if not matches:
        # UTAU can omit the VCV prefix when a note inherits it from context.
        matches = []
        for entry in entries:
            parts = str(entry["base"]).split()
            if parts and parts[-1] == lyric:
                matches.append(entry)
    if not matches:
        return None
    return min(
        matches,
        key=lambda entry: abs((entry["pitch"] or 0) - midi) if entry["pitch"] is not None else 999,
    )


def read_wav(path: Path) -> tuple[int, list[float]]:
    with wave.open(str(path), "rb") as handle:
        fs = handle.getframerate()
        channels = handle.getnchannels()
        width = handle.getsampwidth()
        frames = handle.readframes(handle.getnframes())
    if width != 2:
        raise RuntimeError(f"unsupported WAV width in {path}: {width * 8} bit")
    values = struct.unpack(f"<{len(frames) // 2}h", frames)
    if channels == 1:
        return fs, [value / 32768.0 for value in values]
    mono = [
        sum(values[frame * channels : (frame + 1) * channels]) / (channels * 32768.0)
        for frame in range(len(values) // channels)
    ]
    return fs, mono


def write_wav(path: Path, fs: int, samples: list[float]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), "wb") as handle:
        handle.setnchannels(1)
        handle.setsampwidth(2)
        handle.setframerate(fs)
        handle.writeframes(
            b"".join(struct.pack("<h", max(-32768, min(32767, round(value * 32767)))) for value in samples)
        )


def fade(samples: list[float], fs: int, milliseconds: float = 5.0) -> list[float]:
    length = min(len(samples) // 2, int(fs * milliseconds / 1000.0))
    if length <= 0:
        return samples
    for index in range(length):
        gain = index / length
        samples[index] *= gain
        samples[-1 - index] *= gain
    return samples


def parse_float(value: str, default: float = 0.0) -> float:
    try:
        return float(value.strip())
    except ValueError:
        return default


def pitch_points(note: dict[str, str]) -> list[tuple[float, float]]:
    pbs = note.get("PBS", "").strip()
    pbw = note.get("PBW", "").strip()
    pby = note.get("PBY", "").strip()
    if not pbs and not pbw and not pby:
        return []

    if pbs:
        parts = pbs.split(";" if ";" in pbs else ",")
        start_x = parse_float(parts[0])
        start_y = parse_float(parts[1]) if len(parts) > 1 else 0.0
    else:
        start_x = 0.0
        start_y = 0.0

    widths = [parse_float(part) for part in pbw.split(",") if part.strip()] if pbw else []
    values = [parse_float(part) for part in pby.split(",") if part.strip()] if pby else []
    while len(values) < len(widths):
        values.append(0.0)

    points = [(start_x, start_y)]
    position = start_x
    for width, value in zip(widths, values):
        position += width
        points.append((position, value))
    return points


def pitchbend_string(note: dict[str, str], note_ms: float, fs: int, tempo: float) -> str:
    points = pitch_points(note)
    if len(points) < 2:
        return ""

    # UST pitch values are tenths of a semitone. lessampler's pitch-bend
    # protocol carries signed 12-bit cent values sampled at 1/96 beat.
    pitch_step = int(math.floor(60.0 / 96.0 / tempo * fs + 0.5))
    if pitch_step <= 0:
        return ""
    output_samples = int(note_ms * 0.001 * fs + 1.0)
    count = output_samples // pitch_step + 2
    step_ms = pitch_step * 1000.0 / fs

    encoded: list[str] = []
    segment = 0
    for index in range(count):
        time_ms = index * step_ms
        while segment + 1 < len(points) and time_ms > points[segment + 1][0]:
            segment += 1
        if time_ms <= points[0][0]:
            value = points[0][1]
        elif time_ms >= points[-1][0]:
            value = points[-1][1]
        else:
            left = points[segment]
            right = points[segment + 1]
            span = right[0] - left[0]
            fraction = 0.0 if span <= 0 else (time_ms - left[0]) / span
            value = left[1] + (right[1] - left[1]) * fraction
        cents = max(-2048, min(2047, int(round(value * 10.0))))
        raw = cents & 0xFFF
        encoded.append(UTAU64[raw >> 6])
        encoded.append(UTAU64[raw & 63])
    return "".join(encoded)


def render(args: argparse.Namespace) -> None:
    tempo, default_flags, notes = parse_ust(args.ust)
    entries = parse_oto(args.voicebank)
    if not entries:
        raise SystemExit(f"no oto.ini entries found under {args.voicebank}")
    if not args.lessampler.is_file():
        raise SystemExit(f"lessampler binary not found: {args.lessampler}")

    mixed: list[float] = []
    fs = 0
    tick = 0
    rendered = 0
    sample_rates: dict[Path, int] = {}

    with tempfile.TemporaryDirectory(prefix="lessampler-ust-") as temporary:
        temporary = Path(temporary)
        for index, note in enumerate(notes):
            try:
                length_ticks = int(note.get("Length", "0"))
            except ValueError:
                length_ticks = 0
            if length_ticks <= 0:
                continue
            start = tick * 60.0 / (tempo * TICKS_PER_QUARTER)
            tick += length_ticks
            lyric = note.get("Lyric", "")
            if not lyric or lyric == "R" or lyric.upper() == "R":
                continue
            try:
                midi = int(note.get("NoteNum", "60"))
            except ValueError:
                continue
            entry = select_entry(entries, lyric, midi)
            if entry is None:
                print(f"[warn] note {index}: no oto entry for {lyric!r}", file=sys.stderr)
                continue

            note_ms = length_ticks * 60_000.0 / (tempo * TICKS_PER_QUARTER)
            fixed_ms = min(float(entry["consonant"]), max(0.0, note_ms * 0.8))
            wav_path = entry["wav"]
            if wav_path not in sample_rates:
                try:
                    with wave.open(str(wav_path), "rb") as sample_wav:
                        sample_rates[wav_path] = sample_wav.getframerate()
                except (OSError, wave.Error):
                    sample_rates[wav_path] = 44100
            pitch = pitchbend_string(note, note_ms, sample_rates[wav_path], tempo)
            output = temporary / f"note-{index:04d}.wav"
            volume = note.get("Intensity", "100")
            modulation = note.get("Modulation", note.get("Moduration", "0"))
            flags = note.get("Flags", default_flags)
            command = [
                str(args.lessampler),
                str(entry["wav"]),
                str(output),
                midi_to_name(midi),
                "100",
                flags,
                f"{entry['offset']:.3f}",
                f"{note_ms:.3f}",
                f"{fixed_ms:.3f}",
                "0",
                volume,
                modulation,
                f"!{tempo:g}",
                pitch,
            ]
            result = subprocess.run(command, capture_output=True, text=True)
            if result.returncode != 0 or not output.is_file():
                print(f"[warn] note {index} failed: {result.stderr.strip() or result.stdout.strip()}", file=sys.stderr)
                continue

            note_fs, samples = read_wav(output)
            samples = fade(samples, note_fs)
            if fs == 0:
                fs = note_fs
            elif note_fs != fs:
                raise SystemExit(f"sample rate changed from {fs} to {note_fs}")
            start_sample = int(start * fs)
            needed = start_sample + len(samples)
            if len(mixed) < needed:
                mixed.extend([0.0] * (needed - len(mixed)))
            for offset, value in enumerate(samples):
                mixed[start_sample + offset] += value
            rendered += 1

    if rendered == 0:
        raise SystemExit("no notes rendered")
    peak = max(abs(value) for value in mixed) if mixed else 0.0
    if peak > 0.99:
        scale = 0.99 / peak
        mixed = [value * scale for value in mixed]
    write_wav(args.output, fs, mixed)
    print(f"rendered {rendered}/{len(notes)} notes, {len(mixed) / fs:.2f}s -> {args.output}")


def main() -> int:
    parser = argparse.ArgumentParser(description="Render a UTAU .ust with lessampler")
    parser.add_argument("--ust", required=True, type=Path)
    parser.add_argument("--voicebank", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--lessampler", type=Path, default=Path("build/max-gcc/bin/lessampler"))
    args = parser.parse_args()
    render(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
