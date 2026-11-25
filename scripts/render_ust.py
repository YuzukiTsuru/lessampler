#!/usr/bin/env python3
"""Render a UTAU .ust file with lessampler and lesswavtool.

This is a UTAU host, not a resampler. It selects the oto.ini entry for each
note, computes the classic preutterance/overlap placement, invokes lessampler
once per note, and then hands all note buffers to lesswavtool with their UST
envelopes. The wavtool performs the overlap seek-back and the final mix.
"""
from __future__ import annotations

import argparse
import math
import os
import re
import shutil
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
DEFAULT_ENVELOPE = "0,5,35,0,100,100,0,%,0,10,100"


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
                    "pitch_tag": strip_pitch(alias)[1],
                    "folder": oto.parent.name,
                    "pitch": folder_pitch,
                    "offset": offset,
                    "consonant": consonant,
                    "cutoff": cutoff,
                    "preutter": preutter,
                    "overlap": overlap,
                }
            )
    return entries


def parse_prefix_map(voicebank: Path) -> dict[str, str]:
    candidates = [voicebank / "prefix.map"]
    candidates.extend(sorted(voicebank.rglob("prefix.map")))
    for path in candidates:
        if not path.is_file():
            continue
        mapping: dict[str, str] = {}
        for raw in read_text(path).splitlines():
            line = raw.strip()
            if not line or line.startswith(";"):
                continue
            parts = line.replace("=", " ").split()
            if len(parts) >= 2 and NOTE_NAME.fullmatch(parts[0]):
                mapping[parts[0]] = parts[1]
        if mapping:
            return mapping
    return {}


def lyric_matches(entry: dict[str, object], lyric: str) -> bool:
    return entry["base"] == lyric or entry["alias"] == lyric


def vcv_matches(entry: dict[str, object], lyric: str) -> bool:
    parts = str(entry["base"]).split()
    return bool(parts) and parts[-1] == lyric


def prefix_matches(entry: dict[str, object], prefix: str | None) -> bool:
    if not prefix:
        return False
    if entry["pitch_tag"] == prefix:
        return True
    return str(entry["folder"]).startswith(prefix)


def select_entry(
    entries: list[dict[str, object]],
    lyric: str,
    midi: int,
    prefix_map: dict[str, str],
) -> dict[str, object] | None:
    search_lyric = lyric[1:] if lyric.startswith("?") else lyric
    matches = [entry for entry in entries if lyric_matches(entry, search_lyric)]
    if not matches:
        # UTAU can omit the VCV prefix when a note inherits it from context.
        matches = [entry for entry in entries if vcv_matches(entry, search_lyric)]
    if not matches:
        return None

    prefix = None if lyric.startswith("?") else prefix_map.get(midi_to_name(midi))
    preferred = [entry for entry in matches if prefix_matches(entry, prefix)]
    if preferred:
        matches = preferred

    return min(
        matches,
        key=lambda entry: abs((entry["pitch"] or 0) - midi) if entry["pitch"] is not None else 999,
    )


def parse_float(value: str, default: float = 0.0) -> float:
    try:
        return float(value.strip())
    except ValueError:
        return default


def optional_float(value: str | None) -> float | None:
    if value is None or not value.strip():
        return None
    return parse_float(value)


def probe_wav(path: Path) -> tuple[int, int] | None:
    try:
        with wave.open(str(path), "rb") as handle:
            return handle.getframerate(), handle.getnframes()
    except (OSError, wave.Error):
        return None


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


def pitch_value(points: list[tuple[float, float]], time_ms: float, fallback: float) -> float:
    if not points:
        return fallback
    if time_ms <= points[0][0]:
        return fallback
    if time_ms >= points[-1][0]:
        return points[-1][1]
    for index in range(len(points) - 1):
        left = points[index]
        right = points[index + 1]
        if time_ms <= right[0]:
            span = right[0] - left[0]
            if span <= 0.0:
                return right[1]
            fraction = (time_ms - left[0]) / span
            return left[1] + (right[1] - left[1]) * fraction
    return points[-1][1]


def pitchbend_string(
    note: dict[str, str],
    required_ms: float,
    fs: int,
    tempo: float,
    start_ms: float,
    previous_end: float,
) -> str:
    points = pitch_points(note)
    if not points:
        # The buffer head is the previous note's VCV vowel. Without a UST
        # pitch curve it must still carry the previous pitch until the note
        # start, otherwise the overlap pulls the previous note's tail to this
        # note's pitch.
        if previous_end == 0.0 or start_ms >= 0.0:
            return ""
        points = [(start_ms, previous_end), (0.0, 0.0)]
    elif points[0][0] > 0.0:
        points = [(0.0, 0.0)] + points

    pitch_step = int(math.floor(60.0 / 96.0 / tempo * fs + 0.5))
    if pitch_step <= 0:
        return ""
    output_samples = int(required_ms * 0.001 * fs + 1.0)
    count = output_samples // pitch_step + 2
    step_ms = pitch_step * 1000.0 / fs

    encoded: list[str] = []
    for index in range(count):
        time_ms = start_ms + index * step_ms
        value = pitch_value(points, time_ms, previous_end)
        cents = max(-2048, min(2047, int(round(value * 10.0))))
        raw = cents & 0xFFF
        encoded.append(UTAU64[raw >> 6])
        encoded.append(UTAU64[raw & 63])
    return "".join(encoded)


def previous_pitch_context(
    note: dict[str, str],
    midi: int,
    previous_midi: int | None,
    previous_note: dict[str, str] | None,
    previous_duration: float,
) -> float:
    """Return the tenth-of-a-semitone value to hold before this note's curve."""
    if previous_midi is None or previous_note is None:
        points = pitch_points(note)
        return points[0][1] if points else 0.0

    previous_points = pitch_points(previous_note)
    if previous_points:
        previous_value = pitch_value(previous_points, previous_duration, previous_points[0][1])
    else:
        previous_value = 0.0
    return previous_value + (previous_midi - midi) * 10.0


def toml_quote(text: str) -> str:
    return '"' + text.replace("\\", "\\\\").replace('"', '\\"') + '"'


def find_wavtool(lessampler: Path, explicit: Path | None) -> Path:
    if explicit is not None:
        return explicit
    name = "lesswavtool.exe" if os.name == "nt" else "lesswavtool"
    sibling = lessampler.with_name(name)
    if sibling.is_file():
        return sibling
    found = shutil.which("lesswavtool")
    if found:
        return Path(found)
    return sibling


def render(args: argparse.Namespace) -> None:
    tempo, default_flags, notes = parse_ust(args.ust)
    entries = parse_oto(args.voicebank)
    if not entries:
        raise SystemExit(f"no oto.ini entries found under {args.voicebank}")
    if not args.lessampler.is_file():
        raise SystemExit(f"lessampler binary not found: {args.lessampler}")
    wavtool = find_wavtool(args.lessampler, args.wavtool)
    if not wavtool.is_file():
        raise SystemExit(f"lesswavtool binary not found: {wavtool}")

    prefix_map = parse_prefix_map(args.voicebank)
    sample_rates: dict[Path, int] = {}
    fs = 0
    tick = 0
    rendered = 0
    total = len(notes)
    skipped: list[tuple[int, str]] = []

    with tempfile.TemporaryDirectory(prefix="lessampler-ust-") as temporary:
        temporary = Path(temporary)
        plan_notes: list[dict[str, object]] = []

        for index, note in enumerate(notes):
            try:
                length_ticks = int(note.get("Length", "0"))
            except ValueError:
                length_ticks = 0
            if length_ticks <= 0:
                continue
            start_ms = tick * 60_000.0 / (tempo * TICKS_PER_QUARTER)
            tick += length_ticks
            duration_ms = length_ticks * 60_000.0 / (tempo * TICKS_PER_QUARTER)
            lyric = note.get("Lyric", "")
            if not lyric or lyric.upper() == "R":
                continue
            try:
                midi = int(note.get("NoteNum", "60"))
            except ValueError:
                continue
            entry = select_entry(entries, lyric, midi, prefix_map)
            if entry is None:
                skipped.append((index, lyric))
                continue

            velocity = float(round(max(0.0, min(200.0, parse_float(note.get("Velocity", "100"), 100.0)))))
            note["_velocity"] = str(int(velocity))
            note["_entry"] = entry  # type: ignore[assignment]
            note["_midi"] = str(midi)
            note["_start"] = f"{start_ms:.6f}"
            note["_duration"] = f"{duration_ms:.6f}"

            wav_path = entry["wav"]
            if wav_path not in sample_rates:
                probed = probe_wav(wav_path)
                sample_rates[wav_path] = probed[0] if probed else 44100
            rate = sample_rates[wav_path]
            if fs == 0:
                fs = rate
            elif rate != fs:
                raise SystemExit(f"sample rate changed from {fs} to {rate}")

        # First pass: effective preutterance/overlap with ACPT.
        rendered_notes: list[dict[str, str]] = []
        for note in notes:
            if "_entry" not in note:
                continue
            rendered_notes.append(note)

        for idx, note in enumerate(rendered_notes):
            entry = note["_entry"]
            assert isinstance(entry, dict)
            midi = int(note["_midi"])
            start_ms = float(note["_start"])
            duration_ms = float(note["_duration"])
            velocity = float(note["_velocity"])
            rate = 2.0 ** (1.0 - velocity / 100.0)
            auto_p = float(entry["preutter"]) * rate
            auto_o = float(entry["overlap"]) * rate
            max_p = auto_p

            previous = rendered_notes[idx - 1] if idx > 0 else None
            if previous is not None:
                previous_start = float(previous["_start"])
                previous_duration = float(previous["_duration"])
                gap = start_ms - (previous_start + previous_duration)
                # Tick-derived times can differ by a few ULPs even for adjacent
                # notes; treat a sub-microsecond gap as zero.
                if gap <= 1e-4:
                    if auto_o > 0.0 and auto_p - auto_o > previous_duration / 2.0:
                        max_p = (previous_duration / 2.0) / (auto_p - auto_o) * auto_p
                    max_p = min(max_p, previous_duration)
                    if float(previous["_p_eff"]) < 5.0:
                        max_p = min(max_p, previous_duration + float(previous["_p_eff"]) - 5.0)
                elif gap < auto_p:
                    max_p = gap
                if auto_p > max_p:
                    auto_o *= max_p / auto_p
                    auto_p = max_p
                auto_p = max(0.0, auto_p)
                if auto_o < 0.0:
                    auto_o = max(auto_o, min(0.0, 35.0 - previous_duration + auto_p))

            p_override = optional_float(note.get("PreUtterance"))
            o_override = optional_float(note.get("VoiceOverlap"))
            p_eff = p_override if p_override is not None else auto_p
            o_eff = o_override if o_override is not None else auto_o
            if previous is not None:
                previous_duration = float(previous["_duration"])
                if previous_duration - p_eff < 5.0:
                    o_eff = max(o_eff, 5.0 - (previous_duration - p_eff))
            note["_p_eff"] = f"{max(0.0, p_eff):.6f}"
            note["_o_eff"] = f"{o_eff:.6f}"

        # Second pass: render each note and collect the wavtool plan.
        for idx, note in enumerate(rendered_notes):
            entry = note["_entry"]
            assert isinstance(entry, dict)
            midi = int(note["_midi"])
            start_ms = float(note["_start"])
            duration_ms = float(note["_duration"])
            velocity = float(note["_velocity"])
            rate = 2.0 ** (1.0 - velocity / 100.0)
            p_eff = float(note["_p_eff"])
            o_eff = float(note["_o_eff"])

            next_note = rendered_notes[idx + 1] if idx + 1 < len(rendered_notes) else None
            adjacent = next_note is not None and float(next_note["_start"]) <= start_ms + duration_ms + 1e-4
            if adjacent:
                next_p = float(next_note["_p_eff"])
                next_o = float(next_note["_o_eff"])
                tail_intrude = max(next_p, next_p - next_o)
                tail_overlap = max(next_o, 0.0)
            else:
                tail_intrude = 0.0
                tail_overlap = 0.0
            length_ms = max(0.0, duration_ms + p_eff - tail_intrude + tail_overlap)
            position_ms = start_ms - p_eff

            offset_oto = float(entry["offset"])
            consonant = float(entry["consonant"])
            cutoff = float(entry["cutoff"])
            preutter = float(entry["preutter"])
            pulled = max(0.0, min(round(p_eff / rate - preutter, 3), max(0.0, offset_oto)))
            offset_arg = offset_oto - pulled
            fixed_arg = consonant + pulled
            cutoff_arg = cutoff - pulled if cutoff < 0.0 else cutoff
            pitch_lead = (preutter + pulled) * rate
            skip = max(0.0, pitch_lead - p_eff)
            required = (math.floor(max(length_ms + skip, fixed_arg * rate) / 50.0 + 0.5) + 1.0) * 50.0

            flags = note.get("Flags", default_flags)
            volume = note.get("Intensity", "100")
            modulation = note.get("Modulation", note.get("Moduration", "0"))
            previous = rendered_notes[idx - 1] if idx > 0 else None
            previous_midi = int(previous["_midi"]) if previous is not None else None
            previous_duration = float(previous["_duration"]) if previous is not None else 0.0
            hold = previous_pitch_context(note, midi, previous_midi, previous, previous_duration)
            pitch = pitchbend_string(
                note,
                required,
                sample_rates[entry["wav"]],
                tempo,
                -(p_eff + skip),
                hold,
            )

            output = temporary / f"note-{idx:04d}.wav"
            command = [
                str(args.lessampler),
                str(entry["wav"]),
                str(output),
                midi_to_name(midi),
                note["_velocity"],
                flags,
                f"{offset_arg:.3f}",
                f"{required:.3f}",
                f"{fixed_arg:.3f}",
                f"{cutoff_arg:.3f}",
                volume,
                modulation,
                f"!{tempo:g}",
                pitch,
            ]
            result = subprocess.run(command, capture_output=True, text=True)
            if result.returncode != 0 or not output.is_file():
                message = result.stderr.strip() or result.stdout.strip()
                print(f"[warn] note {idx} failed: {message}", file=sys.stderr)
                continue

            plan_notes.append(
                {
                    "input": str(output),
                    "position_ms": position_ms,
                    "skip_ms": skip,
                    "length_ms": length_ms,
                    "envelope": note.get("Envelope", DEFAULT_ENVELOPE),
                }
            )
            rendered += 1

        if rendered == 0 or not plan_notes:
            raise SystemExit("no notes rendered")

        output_path = args.output.resolve()
        output_path.parent.mkdir(parents=True, exist_ok=True)
        plan_path = temporary / "plan.toml"
        lines = [
            f"output = {toml_quote(str(output_path))}",
            f"sample_rate = {fs}",
            "channels = 1",
            "peak_normalize = 0.99",
            "",
        ]
        for item in plan_notes:
            lines.extend(
                [
                    "[[notes]]",
                    f"input = {toml_quote(str(item['input']))}",
                    f"position_ms = {float(item['position_ms']):.6f}",
                    f"skip_ms = {float(item['skip_ms']):.6f}",
                    f"length_ms = {float(item['length_ms']):.6f}",
                    f"envelope = {toml_quote(str(item['envelope']))}",
                    "",
                ]
            )
        plan_path.write_text("\n".join(lines), encoding="utf-8")

        if args.feature:
            result = subprocess.run([str(args.lessampler), "--feature-plan", str(plan_path)],
                                    capture_output=True, text=True)
            if result.returncode != 0:
                raise SystemExit(result.stderr.strip() or "feature assembly failed")
        else:
            result = subprocess.run([str(wavtool), "--plan", str(plan_path)], capture_output=True, text=True)
            if result.returncode != 0:
                raise SystemExit(result.stderr.strip() or "lesswavtool failed")

            if args.polish:
                mix_path = output_path.with_name(output_path.stem + "-mix" + output_path.suffix)
                output_path.replace(mix_path)
                polish = subprocess.run([str(args.lessampler), "--polish", str(mix_path), str(output_path)],
                                        capture_output=True, text=True)
                if polish.returncode != 0:
                    mix_path.replace(output_path)
                    raise SystemExit(polish.stderr.strip() or "neural polish failed")

    with wave.open(str(output_path), "rb") as handle:
        duration = handle.getnframes() / handle.getframerate()

    for index, lyric in skipped:
        print(f"[warn] note {index}: no oto entry for {lyric!r}", file=sys.stderr)
    suffix = " (features)" if args.feature else (" (polished)" if args.polish else "")
    print(f"rendered {rendered}/{total} notes, {duration:.3f}s{suffix} -> {args.output}")


def main() -> int:
    parser = argparse.ArgumentParser(description="Render a UTAU .ust with lessampler and lesswavtool")
    parser.add_argument("--ust", required=True, type=Path)
    parser.add_argument("--voicebank", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--lessampler", type=Path, default=Path("build/max-gcc/bin/lessampler"))
    parser.add_argument("--wavtool", type=Path, default=None)
    parser.add_argument("--polish", action="store_true",
                        help="re-render the assembled mix through the neural vocoder (requires [neural] model_path)")
    parser.add_argument("--feature", action="store_true",
                        help="assemble mel/F0 features and run the vocoder once (requires [neural] model_path)")
    args = parser.parse_args()
    if args.polish and args.feature:
        parser.error("--polish and --feature are mutually exclusive")
    render(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
