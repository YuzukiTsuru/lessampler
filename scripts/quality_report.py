#!/usr/bin/env python3
"""Objective quality report for lessampler renders.

The default metrics need no Python ML stack:
  * F0 accuracy against the UST target (via `lessampler --f0`)
  * note-boundary dips and silence runs
  * peak / RMS levels

With --singmos it also runs SingMOS-Pro when torch/s3prl are installed.
"""
from __future__ import annotations

import argparse
import json
import math
import struct
import subprocess
import sys
import tempfile
import wave
from bisect import bisect_right
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from render_ust import TICKS_PER_QUARTER, parse_ust, pitch_points, pitch_value  # noqa: E402


def read_wav(path: Path) -> tuple[int, list[float]]:
    with wave.open(str(path), "rb") as handle:
        fs = handle.getframerate()
        channels = handle.getnchannels()
        width = handle.getsampwidth()
        frames = handle.readframes(handle.getnframes())
    if width != 2:
        raise SystemExit(f"{path}: only 16-bit PCM is supported")
    values = list(struct.unpack(f"<{len(frames) // 2}h", frames))
    if channels == 1:
        return fs, [value / 32768.0 for value in values]
    mono = [
        sum(values[frame * channels : (frame + 1) * channels]) / (channels * 32768.0)
        for frame in range(len(values) // channels)
    ]
    return fs, mono


def note_targets(ust: Path) -> list[dict[str, object]]:
    tempo, _, notes = parse_ust(ust)
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
        lyric = note.get("Lyric", "")
        if not lyric or lyric.upper() == "R" or lyric.strip().lower() == "x":
            continue
        try:
            midi = int(note.get("NoteNum", "60"))
        except ValueError:
            continue
        points = pitch_points(note)
        targets.append(
            {
                "start": start_ms,
                "end": start_ms + length * 60_000.0 / (tempo * TICKS_PER_QUARTER),
                "midi": midi,
                "points": points,
            }
        )
    return targets


def target_hz(note: dict[str, object], t_ms: float) -> float:
    points = note["points"]
    fallback = points[0][1] if points else 0.0
    cents = pitch_value(points, t_ms - float(note["start"]), fallback) * 10.0
    return 440.0 * 2.0 ** ((int(note["midi"]) - 69) / 12.0) * 2.0 ** (cents / 1200.0)


def dump_f0(lessampler: Path, wav: Path, output: Path) -> None:
    result = subprocess.run([str(lessampler), "--f0", str(wav), str(output)], capture_output=True, text=True)
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or result.stdout.strip() or "lessampler --f0 failed")


def read_f0(path: Path) -> tuple[list[float], list[float]]:
    times: list[float] = []
    values: list[float] = []
    with path.open("r", encoding="utf-8") as handle:
        next(handle, None)
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


def f0_metrics(lessampler: Path, wav: Path, targets: list[dict[str, object]], work: Path, margin_ms: float) -> dict[str, object]:
    csv = work / (wav.stem + ".f0.csv")
    dump_f0(lessampler, wav, csv)
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
        error = 1200.0 * math.log2(f0 / reference)
        cents_errors.append(error)

    if not cents_errors:
        return {"frames": total, "vuv": vuv_errors / total if total else 0.0}
    absolute = sorted(abs(value) for value in cents_errors)
    return {
        "frames": total,
        "mae_cents": sum(absolute) / len(absolute),
        "median_cents": absolute[len(absolute) // 2],
        "gpe": sum(value > 50.0 for value in absolute) / len(absolute),
        "vuv": vuv_errors / total if total else 0.0,
    }


def rms(samples: list[float], start: int, end: int) -> float:
    start = max(0, start)
    end = min(len(samples), end)
    if end <= start:
        return 0.0
    return math.sqrt(sum(value * value for value in samples[start:end]) / (end - start))


def seam_metrics(wav: Path, ust: Path, samples: list[float], fs: int) -> dict[str, object]:
    tempo, _, notes = parse_ust(ust)
    tick = 0
    starts = []
    for note in notes:
        try:
            length = int(note.get("Length", "0"))
        except ValueError:
            continue
        if length <= 0:
            continue
        start_ms = tick * 60_000.0 / (tempo * TICKS_PER_QUARTER)
        tick += length
        if note.get("Lyric", "").upper() != "R":
            starts.append(start_ms)

    window = int(0.002 * fs)
    side_near = int(0.010 * fs)
    side_far = int(0.050 * fs)
    dips = 0
    evaluated = 0
    ratios: list[float] = []
    for start_ms in starts:
        centre = int(start_ms * fs / 1000.0)
        if centre - side_far < 0 or centre + side_far >= len(samples):
            continue
        reference = min(rms(samples, centre - side_far, centre - side_near),
                        rms(samples, centre + side_near, centre + side_far))
        if reference < 1e-6:
            continue
        local = min(rms(samples, centre + offset, centre + offset + window)
                    for offset in range(-side_near, side_near, window))
        ratio = local / reference
        ratios.append(ratio)
        evaluated += 1
        if ratio < 0.3:
            dips += 1

    run = 0
    longest = 0
    for value in samples:
        run = run + 1 if abs(value) <= 1.0 / 32768.0 else 0
        longest = max(longest, run)

    ratios.sort()
    return {
        "seam_dips": dips,
        "seam_evaluated": evaluated,
        "seam_median": ratios[len(ratios) // 2] if ratios else 0.0,
        "longest_silence_ms": longest * 1000.0 / fs,
    }


def level_metrics(samples: list[float]) -> dict[str, object]:
    if not samples:
        return {}
    peak = max(abs(value) for value in samples)
    mean = math.sqrt(sum(value * value for value in samples) / len(samples))
    return {
        "peak_dbfs": 20.0 * math.log10(max(peak, 1e-9)),
        "rms_dbfs": 20.0 * math.log10(max(mean, 1e-9)),
    }


_singmos_predictor = None


def singmos_scores(wav: Path, samples: list[float], fs: int, chunk_seconds: float = 10.0) -> dict[str, object]:
    global _singmos_predictor
    try:
        import torch
        import torchaudio
    except Exception as error:  # noqa: BLE001
        return {"error": f"torch/torchaudio unavailable: {error}"}

    if _singmos_predictor is None:
        try:
            _singmos_predictor = torch.hub.load("South-Twilight/SingMOS:v1.1.2", "singmos_pro", trust_repo=True)
        except Exception as error:  # noqa: BLE001
            return {"error": f"cannot load SingMOS: {error}"}
    predictor = _singmos_predictor

    waveform = torch.tensor(samples, dtype=torch.float32)
    waveform = torchaudio.functional.resample(waveform, fs, 16000)
    chunk = int(chunk_seconds * 16000)
    chunks = [waveform[start : start + chunk] for start in range(0, waveform.numel(), chunk)]
    # Skip rests; SingMOS on pure silence is not a singing-quality signal.
    chunks = [item for item in chunks if torch.sqrt(torch.mean(item * item)) >= 1e-3]
    if not chunks:
        return {"error": "SingMOS skipped: no active chunks"}
    scores: list[float] = []
    batch_size = 4
    with torch.no_grad():
        for start in range(0, len(chunks), batch_size):
            batch = chunks[start : start + batch_size]
            lengths = torch.tensor([item.numel() for item in batch], dtype=torch.long)
            padded = torch.zeros(len(batch), 1, int(lengths.max()))
            for index, item in enumerate(batch):
                padded[index, 0, : item.numel()] = item
            output = predictor(padded, lengths)
            if isinstance(output, (tuple, list)):
                output = output[0]
            scores.extend(float(value) for value in output.reshape(-1).tolist())

    scores.sort()
    return {
        "singmos_mean": sum(scores) / len(scores),
        "singmos_p10": scores[max(0, len(scores) // 10)],
        "singmos_min": scores[0],
        "singmos_chunks": len(scores),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Objective quality report for lessampler renders")
    parser.add_argument("--ust", type=Path, help="UST used to derive target F0")
    parser.add_argument("--lessampler", type=Path, default=Path("build/max-gcc/bin/lessampler"))
    parser.add_argument("--singmos", action="store_true", help="also run SingMOS-Pro (needs torch, s3prl)")
    parser.add_argument("--json", type=Path, help="write the full report as JSON")
    parser.add_argument("--margin-ms", type=float, default=30.0, help="skip this much at each note edge")
    parser.add_argument("wav", nargs="+", type=Path)
    args = parser.parse_args()

    targets = note_targets(args.ust) if args.ust else []
    report = {}

    with tempfile.TemporaryDirectory(prefix="lessampler-quality-") as temporary:
        work = Path(temporary)
        for wav in args.wav:
            fs, samples = read_wav(wav)
            metrics = {
                "duration_s": len(samples) / fs,
                "sample_rate": fs,
                **level_metrics(samples),
            }
            if args.ust:
                metrics.update(seam_metrics(wav, args.ust, samples, fs))
                metrics.update(f0_metrics(args.lessampler, wav, targets, work, args.margin_ms))
            if args.singmos:
                metrics.update(singmos_scores(wav, samples, fs))
            report[str(wav)] = metrics

    columns = [
        ("duration_s", "dur", "{:.1f}s"),
        ("peak_dbfs", "peak", "{:.1f}"),
        ("rms_dbfs", "rms", "{:.1f}"),
        ("seam_dips", "dips", "{:.0f}"),
        ("longest_silence_ms", "silence", "{:.0f}ms"),
        ("mae_cents", "F0 MAE", "{:.1f}c"),
        ("gpe", "GPE", "{:.1%}"),
        ("vuv", "VUV", "{:.1%}"),
        ("singmos_mean", "SingMOS", "{:.2f}"),
    ]

    header = ["file"] + [label for _, label, _ in columns]
    print(" | ".join(header))
    print(" | ".join("---" for _ in header))
    for name, metrics in report.items():
        row = [Path(name).name]
        for key, _, template in columns:
            value = metrics.get(key)
            if isinstance(value, (int, float)):
                row.append(template.format(value))
            else:
                row.append("-")
        print(" | ".join(row))

    for name, metrics in report.items():
        if "error" in metrics:
            print(f"{Path(name).name}: {metrics['error']}", file=sys.stderr)

    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8")
        print(f"report: {args.json}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
