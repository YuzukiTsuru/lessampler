#!/usr/bin/env python3
"""Standardized quality evaluation for lessampler renders.

The objective metrics use only the Python standard library. SingMOS-Pro is
optional; it needs torch, torchaudio and s3prl, and the weights are downloaded
by torch.hub on first use. Use test/quality/setup_singmos.sh to create that
environment.

The JSON report schema is versioned (SCHEMA_VERSION). The MOS protocol is
fixed to SingMOS-Pro v1.1.2, mono 16 kHz, 10 s chunks, and an RMS gate that
drops silent chunks so rests do not dominate the score.
"""
from __future__ import annotations

import argparse
import json
import math
import shutil
import subprocess
import sys
import tempfile
import wave
from array import array
from bisect import bisect_right
from dataclasses import dataclass
from pathlib import Path

SCHEMA_VERSION = 1
SINGMOS_MODEL = "South-Twilight/SingMOS:v1.1.2"
SINGMOS_ENTRY = "singmos_pro"
SINGMOS_RATE = 16000
DEFAULT_CHUNK_SECONDS = 10.0
DEFAULT_MIN_CHUNK_RMS = 1e-3
DEFAULT_F0_MARGIN_MS = 30.0

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))

from render_ust import (  # noqa: E402
    TICKS_PER_QUARTER,
    default_expression,
    effective_vbr,
    parse_ust,
    pitch_points,
    pitch_value,
    vibrato_cents,
)


class EvaluationError(RuntimeError):
    """An input or model error that should be reported without a traceback."""


@dataclass(frozen=True)
class WavData:
    path: Path
    sample_rate: int
    channels: int
    samples: array


def read_wav(path: Path) -> WavData:
    try:
        with wave.open(str(path), "rb") as handle:
            sample_rate = handle.getframerate()
            channels = handle.getnchannels()
            width = handle.getsampwidth()
            frames = handle.readframes(handle.getnframes())
    except (OSError, wave.Error) as error:
        raise EvaluationError(f"{path}: cannot read WAV: {error}") from error

    if width != 2:
        raise EvaluationError(f"{path}: only 16-bit PCM WAV is supported")
    if channels < 1:
        raise EvaluationError(f"{path}: invalid channel count {channels}")

    raw = array("h")
    raw.frombytes(frames)
    if sys.byteorder == "big":
        raw.byteswap()

    scale = 1.0 / 32768.0
    if channels == 1:
        samples = array("f", (value * scale for value in raw))
    else:
        samples = array(
            "f",
            (
                sum(raw[index : index + channels]) * (scale / channels)
                for index in range(0, len(raw), channels)
            ),
        )
    return WavData(path=path, sample_rate=sample_rate, channels=channels, samples=samples)


def rms(samples: array, start: int, end: int) -> float:
    start = max(0, start)
    end = min(len(samples), end)
    if end <= start:
        return 0.0
    total = 0.0
    for index in range(start, end):
        value = samples[index]
        total += value * value
    return math.sqrt(total / (end - start))


def level_metrics(samples: array) -> dict[str, float | None]:
    if not samples:
        return {"peak_dbfs": None, "rms_dbfs": None}
    peak = max(abs(value) for value in samples)
    mean_square = sum(value * value for value in samples) / len(samples)
    return {
        "peak_dbfs": 20.0 * math.log10(max(peak, 1e-9)),
        "rms_dbfs": 20.0 * math.log10(max(math.sqrt(mean_square), 1e-9)),
    }


def note_targets(ust: Path) -> list[dict[str, object]]:
    tempo, _, notes = parse_ust(ust)
    if tempo <= 0.0:
        tempo = 120.0
    tick = 0
    targets: list[dict[str, object]] = []
    for note in notes:
        try:
            length = int(note.get("Length", "0"))
        except ValueError:
            continue
        if length <= 0:
            continue
        start_ms = tick * 60_000.0 / (tempo * TICKS_PER_QUARTER)
        tick += length
        lyric = note.get("Lyric", "").strip()
        if not lyric or lyric.upper() == "R" or lyric.lower() == "x":
            continue
        try:
            midi = int(note.get("NoteNum", "60"))
        except ValueError:
            continue
        duration_ms = length * 60_000.0 / (tempo * TICKS_PER_QUARTER)
        targets.append(
            {
                "start": start_ms,
                "end": start_ms + duration_ms,
                "midi": midi,
                "points": pitch_points(note),
                "vbr": effective_vbr(note, duration_ms, default_expression()),
            }
        )
    return targets


def target_hz(note: dict[str, object], t_ms: float) -> float:
    points = note["points"]
    fallback = points[0][1] if points else 0.0
    time_ms = t_ms - float(note["start"])
    duration_ms = float(note["end"]) - float(note["start"])
    cents = pitch_value(points, time_ms, fallback) * 10.0
    cents += vibrato_cents(note.get("vbr"), time_ms, duration_ms)  # type: ignore[arg-type]
    return 440.0 * 2.0 ** ((int(note["midi"]) - 69) / 12.0) * 2.0 ** (cents / 1200.0)


def dump_f0(lessampler: Path, wav: Path, output: Path) -> None:
    result = subprocess.run(
        [str(lessampler), "--f0", str(wav), str(output)],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        detail = result.stderr.strip() or result.stdout.strip() or f"exit code {result.returncode}"
        raise EvaluationError(f"lessampler --f0 failed: {detail}")
    if not output.is_file():
        raise EvaluationError("lessampler --f0 did not write a CSV file")


def read_f0(path: Path) -> tuple[list[float], list[float]]:
    times: list[float] = []
    values: list[float] = []
    with path.open("r", encoding="utf-8") as handle:
        for line in handle:
            parts = line.strip().split(",")
            if len(parts) != 2:
                continue
            try:
                times.append(float(parts[0]))
                values.append(float(parts[1]))
            except ValueError:
                continue
    return times, values


def f0_metrics(
    lessampler: Path,
    wav: WavData,
    targets: list[dict[str, object]],
    margin_ms: float,
) -> dict[str, object]:
    if not targets:
        return {"frames": 0}

    with tempfile.TemporaryDirectory(prefix="lessampler-f0-") as temporary:
        work = Path(temporary)
        source = work / wav.path.name
        shutil.copyfile(wav.path, source)
        csv = work / (wav.path.stem + ".f0.csv")
        dump_f0(lessampler, source, csv)
        times, values = read_f0(csv)

    if not times:
        return {"frames": 0}

    starts = [float(note["start"]) for note in targets]
    cents_errors: list[float] = []
    vuv_errors = 0
    total = 0
    for t, f0 in zip(times, values):
        index = bisect_right(starts, t) - 1
        if index < 0 or index >= len(targets):
            continue
        note = targets[index]
        if not (float(note["start"]) + margin_ms <= t <= float(note["end"]) - margin_ms):
            continue
        total += 1
        if f0 <= 0.0:
            vuv_errors += 1
            continue
        reference = target_hz(note, t)
        if reference <= 0.0:
            continue
        cents_errors.append(1200.0 * math.log2(f0 / reference))

    result: dict[str, object] = {
        "frames": total,
        "mae_cents": None,
        "median_cents": None,
        "gpe": None,
        "vuv": vuv_errors / total if total else 0.0,
    }
    if cents_errors:
        absolute = sorted(abs(value) for value in cents_errors)
        result.update(
            {
                "mae_cents": sum(absolute) / len(absolute),
                "median_cents": absolute[len(absolute) // 2],
                "gpe": sum(value > 50.0 for value in absolute) / len(absolute),
            }
        )
    return result


def seam_metrics(ust: Path, samples: array, sample_rate: int) -> dict[str, object]:
    tempo, _, notes = parse_ust(ust)
    if tempo <= 0.0:
        tempo = 120.0
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
        lyric = note.get("Lyric", "").strip()
        if lyric and lyric.upper() != "R" and lyric.lower() != "x":
            starts.append(start_ms)

    window = max(1, int(0.002 * sample_rate))
    side_near = max(1, int(0.010 * sample_rate))
    side_far = max(side_near + 1, int(0.050 * sample_rate))
    dips = 0
    evaluated = 0
    ratios: list[float] = []
    for start_ms in starts:
        centre = int(start_ms * sample_rate / 1000.0)
        if centre - side_far < 0 or centre + side_far >= len(samples):
            continue
        reference = min(
            rms(samples, centre - side_far, centre - side_near),
            rms(samples, centre + side_near, centre + side_far),
        )
        if reference < 1e-6:
            continue
        local = min(
            rms(samples, centre + offset, centre + offset + window)
            for offset in range(-side_near, side_near, window)
        )
        ratio = local / reference
        ratios.append(ratio)
        evaluated += 1
        if ratio < 0.3:
            dips += 1

    run = 0
    longest = 0
    threshold = 1.0 / 32768.0
    for value in samples:
        run = run + 1 if abs(value) <= threshold else 0
        longest = max(longest, run)

    ratios.sort()
    return {
        "dips": dips,
        "evaluated": evaluated,
        "median": ratios[len(ratios) // 2] if ratios else 0.0,
        "longest_silence_ms": longest * 1000.0 / sample_rate,
    }


_singmos_predictor = None


def load_singmos():
    global _singmos_predictor
    if _singmos_predictor is not None:
        return _singmos_predictor
    try:
        import torch
        import torchaudio  # noqa: F401
    except Exception as error:  # noqa: BLE001
        raise EvaluationError(
            "SingMOS requires torch, torchaudio and s3prl; run test/quality/setup_singmos.sh"
        ) from error
    try:
        predictor = torch.hub.load(SINGMOS_MODEL, SINGMOS_ENTRY, trust_repo=True)
    except Exception as error:  # noqa: BLE001
        raise EvaluationError(f"cannot load SingMOS: {error}") from error
    if hasattr(predictor, "eval"):
        predictor.eval()
    _singmos_predictor = predictor
    return predictor


def singmos_scores(
    wav: WavData,
    chunk_seconds: float,
    min_chunk_rms: float,
) -> dict[str, object]:
    try:
        import torch
        import torchaudio
    except Exception as error:  # noqa: BLE001
        raise EvaluationError(
            "SingMOS requires torch, torchaudio and s3prl; run test/quality/setup_singmos.sh"
        ) from error

    predictor = load_singmos()
    waveform = torch.tensor(wav.samples, dtype=torch.float32)
    if wav.sample_rate != SINGMOS_RATE:
        waveform = torchaudio.functional.resample(waveform, wav.sample_rate, SINGMOS_RATE)

    chunk = max(1, int(round(chunk_seconds * SINGMOS_RATE)))
    chunks = [waveform[start : start + chunk] for start in range(0, waveform.numel(), chunk)]
    chunks = [
        item
        for item in chunks
        if torch.sqrt(torch.mean(item * item)) >= min_chunk_rms
    ]
    if not chunks:
        raise EvaluationError("SingMOS skipped: no active chunks")

    scores: list[float] = []
    batch_size = 4
    with torch.no_grad():
        for start in range(0, len(chunks), batch_size):
            batch = chunks[start : start + batch_size]
            lengths = torch.tensor([item.numel() for item in batch], dtype=torch.long)
            padded = torch.zeros(len(batch), 1, int(lengths.max()), dtype=torch.float32)
            for index, item in enumerate(batch):
                padded[index, 0, : item.numel()] = item
            output = predictor(padded, lengths)
            if isinstance(output, (tuple, list)):
                output = output[0]
            scores.extend(float(value) for value in output.reshape(-1).tolist())

    if not scores or not all(math.isfinite(value) for value in scores):
        raise EvaluationError("SingMOS returned an empty or non-finite score")

    scores.sort()
    return {
        "backend": "singmos",
        "model": SINGMOS_MODEL,
        "mean": sum(scores) / len(scores),
        "p10": scores[max(0, len(scores) // 10)],
        "min": scores[0],
        "chunks": len(scores),
    }


def evaluate_file(
    path: Path,
    targets: list[dict[str, object]],
    args: argparse.Namespace,
) -> dict[str, object]:
    wav = read_wav(path)
    entry: dict[str, object] = {
        "path": str(path),
        "duration_s": len(wav.samples) / wav.sample_rate,
        "sample_rate": wav.sample_rate,
        "channels": wav.channels,
        **level_metrics(wav.samples),
        "f0": None,
        "seams": None,
        "mos": None,
    }
    if args.ust:
        entry["f0"] = f0_metrics(args.lessampler, wav, targets, args.margin_ms)
        entry["seams"] = seam_metrics(args.ust, wav.samples, wav.sample_rate)
    if args.mos == "singmos":
        mos = singmos_scores(wav, args.chunk_seconds, args.min_chunk_rms)
        if args.min_mos is not None:
            mos["passed"] = bool(float(mos["mean"]) >= args.min_mos)
        entry["mos"] = mos
    return entry


def flat_value(entry: dict[str, object], key: str) -> object:
    if key == "mos":
        mos = entry.get("mos") or {}
        return mos.get("mean")
    if key in entry:
        return entry[key]
    seams = entry.get("seams") or {}
    f0 = entry.get("f0") or {}
    if key in seams:
        return seams[key]
    if key in f0:
        return f0[key]
    return None


def print_table(files: list[dict[str, object]]) -> None:
    columns = [
        ("duration_s", "dur", "{:.1f}s"),
        ("peak_dbfs", "peak", "{:.1f}"),
        ("rms_dbfs", "rms", "{:.1f}"),
        ("dips", "dips", "{:.0f}"),
        ("longest_silence_ms", "silence", "{:.0f}ms"),
        ("mae_cents", "F0 MAE", "{:.1f}c"),
        ("gpe", "GPE", "{:.1%}"),
        ("vuv", "VUV", "{:.1%}"),
        ("mos", "MOS", "{:.2f}"),
    ]
    header = ["file"] + [label for _, label, _ in columns]
    print(" | ".join(header))
    print(" | ".join("---" for _ in header))
    for entry in files:
        row = [Path(str(entry["path"])).name]
        for key, _, template in columns:
            value = flat_value(entry, key)
            row.append(template.format(value) if isinstance(value, (int, float)) else "-")
        print(" | ".join(row))


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Standardized MOS and objective quality evaluation for lessampler renders",
    )
    parser.add_argument("wav", nargs="+", type=Path, help="16-bit PCM WAV render(s)")
    parser.add_argument("--ust", type=Path, help="UST used to derive the target F0 and note seams")
    parser.add_argument(
        "--lessampler",
        type=Path,
        default=Path("build/max-gcc/bin/lessampler"),
        help="lessampler binary used for --f0 (default: build/max-gcc/bin/lessampler)",
    )
    parser.add_argument(
        "--mos",
        choices=("none", "singmos"),
        default="none",
        help="MOS backend; singmos needs torch, torchaudio and s3prl",
    )
    parser.add_argument("--json", type=Path, help="write the versioned JSON report to this path")
    parser.add_argument(
        "--chunk-seconds",
        type=float,
        default=DEFAULT_CHUNK_SECONDS,
        help=f"SingMOS chunk length in seconds (default: {DEFAULT_CHUNK_SECONDS})",
    )
    parser.add_argument(
        "--min-chunk-rms",
        type=float,
        default=DEFAULT_MIN_CHUNK_RMS,
        help=f"drop SingMOS chunks below this RMS (default: {DEFAULT_MIN_CHUNK_RMS})",
    )
    parser.add_argument(
        "--margin-ms",
        type=float,
        default=DEFAULT_F0_MARGIN_MS,
        help=f"skip this much at each note edge for F0 (default: {DEFAULT_F0_MARGIN_MS})",
    )
    parser.add_argument("--min-mos", type=float, help="exit 2 if any file scores below this MOS")
    parser.add_argument("--quiet", action="store_true", help="suppress the table")
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    if args.chunk_seconds <= 0.0:
        parser.error("--chunk-seconds must be positive")
    if args.min_chunk_rms < 0.0:
        parser.error("--min-chunk-rms must be non-negative")
    if args.min_mos is not None and args.mos != "singmos":
        parser.error("--min-mos requires --mos singmos")
    if args.ust and not args.ust.is_file():
        raise EvaluationError(f"UST not found: {args.ust}")

    if args.ust:
        binary = args.lessampler
        if not binary.is_file():
            resolved = shutil.which(str(binary))
            if resolved is None:
                raise EvaluationError(
                    f"lessampler not found: {binary} (pass --lessampler or build the project)"
                )
            args.lessampler = Path(resolved)

    targets = note_targets(args.ust) if args.ust else []
    files = [evaluate_file(path, targets, args) for path in args.wav]
    if not args.quiet:
        print_table(files)

    report = {
        "schema_version": SCHEMA_VERSION,
        "parameters": {
            "ust": str(args.ust) if args.ust else None,
            "lessampler": str(args.lessampler) if args.ust else None,
            "mos_backend": args.mos,
            "singmos_model": SINGMOS_MODEL if args.mos == "singmos" else None,
            "chunk_seconds": args.chunk_seconds,
            "min_chunk_rms": args.min_chunk_rms,
            "f0_margin_ms": args.margin_ms,
            "min_mos": args.min_mos,
        },
        "files": files,
    }
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(
            json.dumps(report, indent=2, ensure_ascii=False) + "\n",
            encoding="utf-8",
        )

    failed = [
        entry
        for entry in files
        if entry.get("mos") is not None and entry["mos"].get("passed") is False
    ]
    if failed:
        names = ", ".join(Path(str(entry["path"])).name for entry in failed)
        print(f"MOS below {args.min_mos}: {names}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except EvaluationError as error:
        print(f"error: {error}", file=sys.stderr)
        raise SystemExit(1)
