#!/usr/bin/env python3
"""Render a UTAU .ust file with lessampler.

This is a UTAU host, not a resampler. It selects the oto.ini entry for each
note, computes the classic preutterance/overlap placement, invokes lessampler
once per note, and then hands all note buffers back to the same binary in
wavtool mode with their UST envelopes. The wavtool performs the overlap
seek-back and the final mix.
"""
from __future__ import annotations

import argparse
import math
import random
import re
import struct
import subprocess
import sys
import tempfile
import wave
from pathlib import Path

try:
    import tomllib
except ModuleNotFoundError:  # Python < 3.11
    tomllib = None

NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
NOTE_OFFSETS = {name: index for index, name in enumerate(NOTE_NAMES)}
PITCH_SUFFIX = re.compile(r"([A-G]#?-?\d)$")
NOTE_NAME = re.compile(r"([A-G]#?)(-?\d)")
TICKS_PER_QUARTER = 480
UTAU64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
DEFAULT_ENVELOPE = "0,5,35,0,100,100,0,%,0,10,100"
# Some oto entries have a fixed/consonant part longer than the short note
# that uses them. Without a cap the wavtool segment ends inside the consonant
# and the vowel never sounds. Keep at least this much vowel in the segment.
MIN_VOWEL_MS = 30.0
BREATH_ALIAS_TOKENS = ("R息", "R吐")
BREATH_LYRIC_TOKENS = {"x", "breath", "息", "吸", "吐", "・"}

_KANA_ROWS = (
    ("あいうえお", "aiueo"),
    ("かきくけこ", "aiueo"),
    ("がぎぐげご", "aiueo"),
    ("さしすせそ", "aiueo"),
    ("ざじずぜぞ", "aiueo"),
    ("たちつてと", "aiueo"),
    ("だぢづでど", "aiueo"),
    ("なにぬねの", "aiueo"),
    ("はひふへほ", "aiueo"),
    ("ばびぶべぼ", "aiueo"),
    ("ぱぴぷぺぽ", "aiueo"),
    ("まみむめも", "aiueo"),
    ("やゆよ", "auo"),
    ("らりるれろ", "aiueo"),
    ("わを", "ao"),
    ("ん", "n"),
    ("ぁぃぅぇぉ", "aiueo"),
    ("ゃゅょ", "auo"),
    ("ゔ", "u"),
    ("ゐ", "i"),
    ("ゑ", "e"),
)
KANA_VOWELS = {
    kana: vowel
    for row, vowels in _KANA_ROWS
    for kana, vowel in zip(row, vowels)
}


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


def parse_vbr(note: dict[str, str]) -> tuple[float, float, float, float, float, float] | None:
    """Return (length%, period_ms, depth_cents, fade_in%, fade_out%, phase%)."""
    text = note.get("VBR", "").strip()
    if not text:
        return None
    fields = [part.strip() for part in text.split(",")]
    if len(fields) < 3:
        return None
    try:
        length = max(0.0, min(100.0, parse_float(fields[0])))
        period = max(5.0, min(500.0, parse_float(fields[1])))
        depth = max(0.0, min(200.0, parse_float(fields[2])))
        fade_in = max(0.0, min(100.0, parse_float(fields[3]))) if len(fields) > 3 else 0.0
        fade_out = max(0.0, min(100.0, parse_float(fields[4]))) if len(fields) > 4 else 0.0
        phase = max(0.0, min(100.0, parse_float(fields[5]))) if len(fields) > 5 else 0.0
    except ValueError:
        return None
    if length <= 0.0 or depth <= 0.0:
        return None
    return length, period, depth, fade_in, fade_out, phase


def default_expression() -> dict[str, dict[str, object]]:
    """Automatic expression defaults. Every feature only fills a gap."""
    return {
        "breath": {
            "enabled": True,
            "min_gap_ms": 180.0,
            "max_ms": 600.0,
            "level": 0.30,
        },
        "vibrato": {
            "enabled": True,
            "min_duration_ms": 600.0,
            "delay_ms": 180.0,
            "fade_ms": 200.0,
            "rate_hz": 5.4,
            "depth_cents": 28.0,
        },
        "portamento": {
            "enabled": True,
            "max_gap_ms": 200.0,
            "max_semitones": 12.0,
            "glide_ms": 70.0,
        },
    }


def load_expression(path: Path | None) -> dict[str, dict[str, object]]:
    """Load an optional expression.toml on top of the built-in defaults."""
    expression = default_expression()
    if path is None:
        return expression
    if tomllib is None:
        print("[warn] tomllib unavailable; ignoring --expression", file=sys.stderr)
        return expression
    try:
        data = tomllib.loads(path.read_text(encoding="utf-8"))
    except (OSError, ValueError, tomllib.TOMLDecodeError) as error:
        raise SystemExit(f"cannot read expression config {path}: {error}") from error

    for section, defaults in expression.items():
        table = data.get(section)
        if not isinstance(table, dict):
            continue
        for key, default in defaults.items():
            if key not in table:
                continue
            value = table[key]
            if isinstance(default, bool):
                if isinstance(value, bool):
                    defaults[key] = value
            else:
                try:
                    defaults[key] = float(value)
                except (TypeError, ValueError):
                    continue

    expression["breath"]["min_gap_ms"] = max(0.0, float(expression["breath"]["min_gap_ms"]))
    expression["breath"]["max_ms"] = max(1.0, float(expression["breath"]["max_ms"]))
    expression["breath"]["level"] = max(0.0, min(1.0, float(expression["breath"]["level"])))
    vibrato = expression["vibrato"]
    vibrato["min_duration_ms"] = max(0.0, float(vibrato["min_duration_ms"]))
    vibrato["delay_ms"] = max(0.0, float(vibrato["delay_ms"]))
    vibrato["fade_ms"] = max(0.0, float(vibrato["fade_ms"]))
    vibrato["rate_hz"] = max(0.5, min(20.0, float(vibrato["rate_hz"])))
    vibrato["depth_cents"] = max(0.0, min(200.0, float(vibrato["depth_cents"])))
    portamento = expression["portamento"]
    portamento["max_gap_ms"] = max(0.0, float(portamento["max_gap_ms"]))
    portamento["max_semitones"] = max(0.0, min(48.0, float(portamento["max_semitones"])))
    portamento["glide_ms"] = max(0.0, float(portamento["glide_ms"]))
    return expression


def has_pitch_curve(note: dict[str, str]) -> bool:
    return bool(pitch_points(note))


def auto_vibrato(duration_ms: float,
                 options: dict[str, object]) -> tuple[float, float, float, float, float, float] | None:
    if not bool(options.get("enabled", True)):
        return None
    if duration_ms < float(options.get("min_duration_ms", 600.0)):
        return None
    delay = max(0.0, float(options.get("delay_ms", 180.0)))
    vibrato_ms = duration_ms - delay
    if vibrato_ms <= 0.0:
        return None
    fade_ms = max(0.0, float(options.get("fade_ms", 200.0)))
    rate = max(0.5, min(20.0, float(options.get("rate_hz", 5.4))))
    depth = max(0.0, min(200.0, float(options.get("depth_cents", 28.0))))
    if depth <= 0.0:
        return None
    length = 100.0 * vibrato_ms / duration_ms
    period = 1000.0 / rate
    fade_in = 100.0 * min(fade_ms, vibrato_ms) / vibrato_ms
    return length, period, depth, fade_in, 0.0, 0.0


def effective_vbr(note: dict[str, str],
                  duration_ms: float,
                  expression: dict[str, dict[str, object]] | None = None
                  ) -> tuple[float, float, float, float, float, float] | None:
    """Explicit UST VBR wins; otherwise synthesize one for a long plain note."""
    if note.get("VBR", "").strip():
        # A present VBR field is a manual decision, even when it is zero or
        # malformed. Do not override it with automatic vibrato.
        return parse_vbr(note)
    if has_pitch_curve(note):
        return None
    options = expression["vibrato"] if expression is not None else default_expression()["vibrato"]
    return auto_vibrato(duration_ms, options)


def auto_portamento_glide(note: dict[str, str],
                          previous_midi: int | None,
                          midi: int,
                          gap_ms: float,
                          duration_ms: float,
                          options: dict[str, object]) -> float | None:
    """Return a glide length when the note has no explicit pitch curve."""
    if not bool(options.get("enabled", True)) or previous_midi is None:
        return None
    if has_pitch_curve(note):
        return None
    if gap_ms > float(options.get("max_gap_ms", 200.0)):
        return None
    if abs(previous_midi - midi) > float(options.get("max_semitones", 12.0)):
        return None
    glide = max(0.0, float(options.get("glide_ms", 70.0)))
    if glide <= 0.0 or duration_ms < glide:
        return None
    return glide


def fallback_pitch_points(start_ms: float,
                          previous_end: float,
                          glide_ms: float | None) -> list[tuple[float, float]]:
    """Hold the previous pitch, then glide into the note when auto is on."""
    default = [(start_ms, previous_end), (0.0, 0.0)]
    if glide_ms is None or previous_end == 0.0:
        return default
    glide_start = max(start_ms, -glide_ms)
    if glide_start <= start_ms:
        return default
    return [(start_ms, previous_end), (glide_start, previous_end), (0.0, 0.0)]


def vibrato_cents(vbr: tuple[float, float, float, float, float, float] | None,
                  time_ms: float, duration_ms: float) -> float:
    if vbr is None or duration_ms <= 0.0 or time_ms <= 0.0 or time_ms > duration_ms:
        return 0.0
    length, period, depth, fade_in, fade_out, phase = vbr
    vibrato_ms = duration_ms * length / 100.0
    if vibrato_ms <= 0.0 or time_ms < duration_ms - vibrato_ms:
        return 0.0
    position = (time_ms - (duration_ms - vibrato_ms)) / vibrato_ms
    gain = 1.0
    fade_in_fraction = fade_in / 100.0
    fade_out_fraction = fade_out / 100.0
    if fade_in_fraction > 0.0 and position < fade_in_fraction:
        gain *= position / fade_in_fraction
    if fade_out_fraction > 0.0 and position > 1.0 - fade_out_fraction:
        gain *= (1.0 - position) / fade_out_fraction
    gain = max(0.0, min(1.0, gain))
    angle = 2.0 * math.pi * ((time_ms - (duration_ms - vibrato_ms)) / period + phase / 100.0)
    return depth * gain * math.sin(angle)


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
    duration_ms: float,
    expression: dict[str, dict[str, object]] | None = None,
    portamento_ms: float | None = None,
) -> str:
    points = pitch_points(note)
    vbr = effective_vbr(note, duration_ms, expression)
    if not points:
        # The buffer head is the previous note's VCV vowel. Without a UST
        # pitch curve it must still carry the previous pitch until the note
        # start, otherwise the overlap pulls the previous note's tail to this
        # note's pitch.
        if vbr is None and (previous_end == 0.0 or start_ms >= 0.0):
            return ""
        points = fallback_pitch_points(start_ms, previous_end, portamento_ms)
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
        value += vibrato_cents(vbr, time_ms, duration_ms) / 10.0
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
    """The wavtool is the lessampler binary itself; --wavtool overrides it."""
    return explicit if explicit is not None else lessampler


def is_breath_lyric(lyric: str) -> bool:
    return lyric.strip().lstrip("?").lower() in BREATH_LYRIC_TOKENS


def breath_vowel(lyric: str) -> str | None:
    """Extract the vowel used by a voicebank's breath aliases."""
    text = lyric.strip().lstrip("?")
    if not text:
        return None
    token = text.split()[-1]
    for char in reversed(token):
        code = ord(char)
        if 0x30A1 <= code <= 0x30F6:
            char = chr(code - 0x60)
        if char in KANA_VOWELS:
            return KANA_VOWELS[char]
    lower = token.lower()
    for char in reversed(lower):
        if char in "aiueo":
            return char
    return "n" if lower.endswith("n") else None


def surrounding_lyric(notes: list[dict[str, str]], index: int) -> str | None:
    for direction in (1, -1):
        position = index + direction
        while 0 <= position < len(notes):
            lyric = notes[position].get("Lyric", "").strip()
            if lyric and lyric.upper() != "R" and not is_breath_lyric(lyric):
                return lyric
            position += direction
    return None


def closest_pitch_entry(entries: list[dict[str, object]], midi: int) -> dict[str, object] | None:
    if not entries:
        return None
    return min(
        entries,
        key=lambda entry: abs((entry["pitch"] or 0) - midi) if entry["pitch"] is not None else 999,
    )


def find_breath_entry(
    entries: list[dict[str, object]],
    vowel: str | None,
    midi: int,
    prefix_map: dict[str, str],
) -> dict[str, object] | None:
    aliases: list[str] = []
    if vowel:
        aliases.extend(f"{vowel} {token}" for token in BREATH_ALIAS_TOKENS)
    aliases.extend(BREATH_ALIAS_TOKENS)
    aliases.extend(("・", "breath"))
    for alias in aliases:
        entry = select_entry(entries, alias, midi, prefix_map)
        if entry is not None:
            return entry

    for token in (*BREATH_ALIAS_TOKENS, "・", "breath"):
        matches = [
            entry
            for entry in entries
            if token in str(entry["base"]) or token in str(entry["alias"])
        ]
        if matches:
            return closest_pitch_entry(matches, midi)
    return None


def synthesize_breath(path: Path, sample_rate: int, duration_ms: float, seed: int,
                      peak: float) -> None:
    """Deterministic filtered-noise breath for voicebanks without a sample."""
    frames = max(1, int(round(duration_ms * 0.001 * sample_rate)))
    attack = max(1, int(round(0.025 * sample_rate)))
    release = max(1, int(round(0.060 * sample_rate)))
    rng = random.Random(seed)
    low_pass = 0.0
    slow = 0.0
    values: list[float] = []
    for index in range(frames):
        white = rng.uniform(-1.0, 1.0)
        low_pass += 0.25 * (white - low_pass)
        slow += 0.002 * (low_pass - slow)
        envelope = min(1.0, index / attack)
        envelope *= min(1.0, (frames - 1 - index) / release)
        values.append((low_pass - slow) * envelope)

    magnitude = max(abs(value) for value in values)
    if magnitude > 0.0:
        scale = peak / magnitude
        values = [value * scale for value in values]
    encoded = struct.pack(f"<{frames}h", *(max(-32768, min(32767, int(round(value * 32767.0)))) for value in values))
    with wave.open(str(path), "wb") as handle:
        handle.setnchannels(1)
        handle.setsampwidth(2)
        handle.setframerate(sample_rate)
        handle.writeframes(encoded)


def make_breath_segment(
    lessampler: Path,
    temporary: Path,
    name: str,
    entry: dict[str, object] | None,
    sample_rate: int,
    position_ms: float,
    duration_ms: float,
    midi: int,
    tempo: float,
    volume_percent: float,
    seed: int,
    envelope: str,
) -> dict[str, object] | None:
    """Render one breath to a plan segment, using synthesis as a fallback."""
    duration_ms = max(0.0, duration_ms)
    if duration_ms < 20.0:
        return None
    volume_percent = max(0.0, min(100.0, volume_percent))
    source = None
    if entry is not None:
        probed = probe_wav(entry["wav"])
        if probed is not None and probed[0] == sample_rate:
            available_ms = probed[1] * 1000.0 / sample_rate - float(entry["offset"])
            if available_ms >= 20.0:
                duration_ms = min(duration_ms, available_ms)
                source = entry

    if source is not None:
        output = temporary / f"{name}.wav"
        fixed = min(float(source["consonant"]), max(0.0, duration_ms - 10.0))
        required = (math.floor(max(duration_ms, fixed) / 50.0 + 0.5) + 1.0) * 50.0
        command = [
            str(lessampler),
            str(source["wav"]),
            str(output),
            midi_to_name(midi),
            "100",
            "C0Y0G0",
            f"{float(source['offset']):.3f}",
            f"{required:.3f}",
            f"{fixed:.3f}",
            f"{float(source['cutoff']):.3f}",
            f"{volume_percent:.0f}",
            "0",
            f"!{tempo:g}",
            "",
        ]
        result = subprocess.run(command, capture_output=True, text=True)
        if result.returncode == 0 and output.is_file():
            return {
                "input": str(output),
                "position_ms": position_ms,
                "skip_ms": 0.0,
                "length_ms": duration_ms,
                "envelope": envelope,
            }
        message = result.stderr.strip() or result.stdout.strip()
        print(f"[warn] breath {name} failed: {message}; using synthesized noise", file=sys.stderr)

    output = temporary / f"{name}-synth.wav"
    synthesize_breath(output, sample_rate, duration_ms, seed, 0.5 * volume_percent / 100.0)
    return {
        "input": str(output),
        "position_ms": position_ms,
        "skip_ms": 0.0,
        "length_ms": duration_ms,
        "envelope": envelope,
    }


def auto_breath_slots(
    rendered_notes: list[dict[str, str]],
    breath_events: list[dict[str, object]],
    options: dict[str, object],
) -> list[dict[str, float | int]]:
    """Plan one breath per phrase gap, skipping gaps with explicit breaths."""
    if not bool(options.get("enabled", True)):
        return []
    min_gap = float(options.get("min_gap_ms", 180.0))
    max_breath = float(options.get("max_ms", 600.0))
    slots: list[dict[str, float | int]] = []
    previous_end = 0.0
    for idx, note in enumerate(rendered_notes):
        start_ms = float(note["_start"])
        duration_ms = float(note["_duration"])
        gap_ms = start_ms - previous_end
        if gap_ms >= min_gap:
            has_explicit = any(
                float(event["start"]) + float(event["duration"]) > previous_end + 1e-4
                and float(event["start"]) < start_ms - 1e-4
                for event in breath_events
            )
            if not has_explicit:
                next_position = start_ms - max(0.0, float(note["_p_eff"]))
                end_ms = next_position - 20.0
                available = end_ms - (previous_end + 10.0)
                if available >= 60.0:
                    breath_duration = min(gap_ms * 0.6, max_breath, available)
                    slots.append(
                        {
                            "index": idx,
                            "start": end_ms - breath_duration,
                            "duration": breath_duration,
                        }
                    )
        previous_end = start_ms + duration_ms
    return slots


def render(args: argparse.Namespace) -> None:
    tempo, default_flags, notes = parse_ust(args.ust)
    entries = parse_oto(args.voicebank)
    if not entries:
        raise SystemExit(f"no oto.ini entries found under {args.voicebank}")
    if not args.lessampler.is_file():
        raise SystemExit(f"lessampler binary not found: {args.lessampler}")
    wavtool = find_wavtool(args.lessampler, args.wavtool)
    if not wavtool.is_file():
        raise SystemExit(f"wavtool binary not found: {wavtool}")

    expression = load_expression(args.expression)
    for name in ("breath", "vibrato", "portamento"):
        override = getattr(args, f"auto_{name}", None)
        if override is not None:
            expression[name]["enabled"] = override

    prefix_map = parse_prefix_map(args.voicebank)
    sample_rates: dict[Path, int] = {}
    fs = 0
    tick = 0
    rendered = 0
    total = len(notes)
    skipped: list[tuple[int, str]] = []
    breath_events: list[dict[str, object]] = []

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
            if is_breath_lyric(lyric):
                breath_events.append(
                    {
                        "index": index,
                        "start": start_ms,
                        "duration": duration_ms,
                        "midi": midi,
                        "volume": parse_float(note.get("Intensity", "100"), 100.0),
                        "envelope": note.get("Envelope", DEFAULT_ENVELOPE),
                    }
                )
                continue
            entry = select_entry(entries, lyric, midi, prefix_map)
            if entry is None:
                skipped.append((index, lyric))
                continue

            velocity = float(round(max(0.0, min(200.0, parse_float(note.get("Velocity", "100"), 100.0)))))
            note["_velocity"] = str(int(velocity))
            note["_entry"] = entry  # type: ignore[assignment]
            note["_lyric"] = lyric
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
            cutoff_arg = cutoff - pulled if cutoff < 0.0 else cutoff
            pitch_lead = (preutter + pulled) * rate
            skip = max(0.0, pitch_lead - p_eff)
            fixed_requested = consonant + pulled
            fixed_arg = min(fixed_requested, max(0.0, length_ms + skip - MIN_VOWEL_MS))
            if fixed_arg < fixed_requested - 1e-6:
                print(
                    f"[warn] note {idx} ({entry['alias']}): fixed {fixed_requested:.1f}ms "
                    f"capped to {fixed_arg:.1f}ms to keep {MIN_VOWEL_MS:.0f}ms of vowel",
                    file=sys.stderr,
                )
            required = (math.floor(max(length_ms + skip, fixed_arg * rate) / 50.0 + 0.5) + 1.0) * 50.0

            flags = note.get("Flags", default_flags)
            volume = note.get("Intensity", "100")
            modulation = note.get("Modulation", note.get("Moduration", "0"))
            previous = rendered_notes[idx - 1] if idx > 0 else None
            previous_midi = int(previous["_midi"]) if previous is not None else None
            previous_duration = float(previous["_duration"]) if previous is not None else 0.0
            hold = previous_pitch_context(note, midi, previous_midi, previous, previous_duration)
            portamento_ms = None
            if previous is not None and previous_midi is not None:
                gap_ms = start_ms - (float(previous["_start"]) + previous_duration)
                portamento_ms = auto_portamento_glide(
                    note,
                    previous_midi,
                    midi,
                    gap_ms,
                    duration_ms,
                    expression["portamento"],
                )
            pitch = pitchbend_string(
                note,
                required,
                sample_rates[entry["wav"]],
                tempo,
                -(p_eff + skip),
                hold,
                duration_ms,
                expression,
                portamento_ms,
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

        breaths_added = 0
        for event in breath_events:
            event_index = int(event["index"])
            lyric = surrounding_lyric(notes, event_index)
            vowel = breath_vowel(lyric) if lyric else None
            entry = find_breath_entry(entries, vowel, int(event["midi"]), prefix_map)
            segment = make_breath_segment(
                args.lessampler,
                temporary,
                f"breath-{event_index:04d}",
                entry,
                fs,
                float(event["start"]),
                float(event["duration"]),
                int(event["midi"]),
                tempo,
                float(event["volume"]),
                event_index,
                str(event["envelope"]),
            )
            if segment is not None:
                plan_notes.append(segment)
                breaths_added += 1

        breath_options = expression["breath"]
        level = float(breath_options["level"]) * 100.0
        for slot in auto_breath_slots(rendered_notes, breath_events, breath_options):
            idx = int(slot["index"])
            note = rendered_notes[idx]
            previous_lyric = ""
            if idx > 0:
                previous_lyric = str(rendered_notes[idx - 1].get("_lyric", ""))
            next_lyric = str(note.get("_lyric", ""))
            vowel = breath_vowel(next_lyric) or breath_vowel(previous_lyric)
            midi = int(note["_midi"])
            entry = find_breath_entry(entries, vowel, midi, prefix_map)
            segment = make_breath_segment(
                args.lessampler,
                temporary,
                f"breath-auto-{idx:04d}",
                entry,
                fs,
                float(slot["start"]),
                float(slot["duration"]),
                midi,
                tempo,
                level,
                len(notes) + idx,
                DEFAULT_ENVELOPE,
            )
            if segment is not None:
                plan_notes.append(segment)
                breaths_added += 1

        if rendered == 0 or not plan_notes:
            raise SystemExit("no notes rendered")

        output_path = args.output.resolve()
        output_path.parent.mkdir(parents=True, exist_ok=True)
        plan_path = temporary / "plan.toml"
        # --polish masters after the neural pass; keep its input raw so the
        # chain is applied exactly once. --feature applies the plan preset
        # inside the feature assembly path.
        plan_master = "none" if args.polish else args.master
        lines = [
            f"output = {toml_quote(str(output_path))}",
            f"sample_rate = {fs}",
            "channels = 1",
            "peak_normalize = 0.99",
            f"master = {toml_quote(plan_master)}",
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
                raise SystemExit(result.stderr.strip() or "wavtool assembly failed")

            if args.polish:
                mix_path = output_path.with_name(output_path.stem + "-mix" + output_path.suffix)
                output_path.replace(mix_path)
                polish = subprocess.run([str(args.lessampler), "--polish", str(mix_path), str(output_path), args.master],
                                        capture_output=True, text=True)
                if polish.returncode != 0:
                    mix_path.replace(output_path)
                    raise SystemExit(polish.stderr.strip() or "neural polish failed")

    with wave.open(str(output_path), "rb") as handle:
        duration = handle.getnframes() / handle.getframerate()

    for index, lyric in skipped:
        print(f"[warn] note {index}: no oto entry for {lyric!r}", file=sys.stderr)
    suffix = " (features)" if args.feature else (" (polished)" if args.polish else "")
    breath_text = f", {breaths_added} breaths" if breaths_added else ""
    print(f"rendered {rendered}/{total} notes{breath_text}, {duration:.3f}s{suffix} -> {args.output}")


def main() -> int:
    parser = argparse.ArgumentParser(description="Render a UTAU .ust with lessampler")
    parser.add_argument("--ust", required=True, type=Path)
    parser.add_argument("--voicebank", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--lessampler", type=Path, default=Path("build/max-gcc/bin/lessampler"))
    parser.add_argument("--wavtool", type=Path, default=None,
                        help="override the wavtool binary (defaults to --lessampler)")
    parser.add_argument("--polish", action="store_true",
                        help="re-render the assembled mix through the neural vocoder (requires [neural] model_path)")
    parser.add_argument("--feature", action="store_true",
                        help="assemble mel/F0 features and run the vocoder once (requires [neural] model_path)")
    parser.add_argument("--master", choices=["none", "natural", "warm", "power"], default="warm",
                        help="final tone/level preset (default: warm; use none for the old raw mix)")
    parser.add_argument("--expression", type=Path, default=None,
                        help="optional expression.toml for automatic breath/vibrato/portamento")
    parser.add_argument("--auto-breath", dest="auto_breath", action="store_true", default=None,
                        help="insert breath segments into phrase gaps (default: on)")
    parser.add_argument("--no-auto-breath", dest="auto_breath", action="store_false",
                        help="disable automatic breath insertion")
    parser.add_argument("--auto-vibrato", dest="auto_vibrato", action="store_true", default=None,
                        help="synthesize vibrato for plain long notes (default: on)")
    parser.add_argument("--no-auto-vibrato", dest="auto_vibrato", action="store_false",
                        help="disable automatic vibrato")
    parser.add_argument("--auto-portamento", dest="auto_portamento", action="store_true", default=None,
                        help="glide into notes without a UST pitch curve (default: on)")
    parser.add_argument("--no-auto-portamento", dest="auto_portamento", action="store_false",
                        help="keep the legacy first-note pitch fallback")
    args = parser.parse_args()
    if args.polish and args.feature:
        parser.error("--polish and --feature are mutually exclusive")
    render(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
