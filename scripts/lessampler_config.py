#!/usr/bin/env python3
"""Generate a lessampler.less preset for a synthesis scenario.

The script writes a complete TOML configuration; it does not download models.
Model paths are resolved to absolute paths and checked for existence.
"""
from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

FRAME_PERIOD = 5.804988662131519


def _config_version() -> str:
    try:
        return subprocess.check_output(
            ["git", "rev-parse", "--short", "HEAD"],
            cwd=Path(__file__).resolve().parent.parent,
            stderr=subprocess.DEVNULL,
            text=True,
        ).strip() or "configurator"
    except (OSError, subprocess.SubprocessError):
        return "configurator"
DEFAULT_NHV = "nhv_v3_3.onnx"
DEFAULT_PCNSF = "pc_nsf_hifigan_44.1k_hop512_128bin_2025.02.onnx"
DEFAULT_RMVPE = "rmvpe.onnx"

MODEL_INFO = {
    "nhv": {
        "file": DEFAULT_NHV,
        "url": "https://raw.githubusercontent.com/wavtechyukky/NHVSing/main/exported_models/v3_3/nhv_v3_3.onnx",
        "sha256": "37d22b928d041e42a017e07f652e59ce27c962f9c5128ef8ebaf4bf982305973",
        "license": "NHVSing code MIT; published weights non-commercial",
    },
    "pcnsf": {
        "file": DEFAULT_PCNSF,
        "url": "https://github.com/openvpi/vocoders/releases/download/pc-nsf-hifigan-44.1k-hop512-128bin-2025.02/pc_nsf_hifigan_44.1k_hop512_128bin_2025.02.zip",
        "sha256": "b297c0c790dbcfe6b2123d87cc20bf8718969f91ffa3170a198c8fb53686155d",
        "license": "CC BY-NC-SA 4.0; weights are not bundled",
    },
    "rmvpe": {
        "file": DEFAULT_RMVPE,
        "url": "https://github.com/yqzhishen/RMVPE/releases/download/230917/rmvpe.oudep",
        "sha256": "bb00595b185fe7cf557a349356f6583e84aed18eb85edb69be1dfa513250f30f",
        "license": "OpenUtau RMVPE package; check its model terms",
    },
}

SCENARIOS = {
    "world-baseline": {
        "title": "WORLD, fastest and most predictable",
        "backend": "world",
        "f0": "harvest",
        "master": "natural",
        "neural": None,
        "extra": "",
    },
    "world-edit": {
        "title": "WORLD + warm master, per-note UTAU editing",
        "backend": "world",
        "f0": "harvest",
        "master": "warm",
        "neural": None,
        "extra": "",
    },
    "llsm-edit": {
        "title": "LLSM + warm master, per-note UTAU editing",
        "backend": "llsm",
        "f0": "llsm",
        "master": "warm",
        "neural": None,
        "extra": "",
    },
    "llsm-rmvpe": {
        "title": "LLSM with RMVPE F0",
        "backend": "llsm",
        "f0": "neural",
        "master": "warm",
        "neural": None,
        "rmvpe": True,
        "extra": "",
    },
    "nhv-note": {
        "title": "NHVSing per note, UTAU controls preserved",
        "backend": "neural",
        "f0": "harvest",
        "master": "warm",
        "neural": "nhv",
        "extra": "",
    },
    "pcnsf-note": {
        "title": "PC-NSF per note, UTAU controls preserved",
        "backend": "neural",
        "f0": "harvest",
        "master": "warm",
        "neural": "pcnsf",
        "extra": "",
    },
    "nhv-note-rmvpe": {
        "title": "NHVSing per note with RMVPE F0",
        "backend": "neural",
        "f0": "neural",
        "master": "warm",
        "neural": "nhv",
        "rmvpe": True,
        "extra": "",
    },
    "pcnsf-note-rmvpe": {
        "title": "PC-NSF per note with RMVPE F0",
        "backend": "neural",
        "f0": "neural",
        "master": "warm",
        "neural": "pcnsf",
        "rmvpe": True,
        "extra": "",
    },
    "nhv-polish": {
        "title": "WORLD mix + NHVSing whole-song polish",
        "backend": "world",
        "f0": "harvest",
        "master": "warm",
        "neural": "nhv",
        "extra": "--polish",
    },
    "pcnsf-polish": {
        "title": "WORLD mix + PC-NSF whole-song polish",
        "backend": "world",
        "f0": "harvest",
        "master": "warm",
        "neural": "pcnsf",
        "extra": "--polish",
    },
    "nhv-feature": {
        "title": "WORLD notes + one NHVSing feature pass",
        "backend": "world",
        "f0": "harvest",
        "master": "warm",
        "neural": "nhv",
        "extra": "--feature",
    },
    "llsm-feature": {
        "title": "LLSM notes + one NHVSing feature pass",
        "backend": "llsm",
        "f0": "llsm",
        "master": "warm",
        "neural": "nhv",
        "extra": "--feature",
    },
    "pcnsf-feature": {
        "title": "WORLD notes + one PC-NSF feature pass",
        "backend": "world",
        "f0": "harvest",
        "master": "warm",
        "neural": "pcnsf",
        "extra": "--feature",
    },
    "nhv-source-mel": {
        "title": "Experimental NHVSing source-mel frontend",
        "backend": "neural",
        "f0": "harvest",
        "master": "none",
        "neural": "nhv",
        "frontend": "source-mel",
        "extra": "",
    },
    "pcnsf-source-mel": {
        "title": "Experimental PC-NSF source-mel frontend",
        "backend": "neural",
        "f0": "harvest",
        "master": "none",
        "neural": "pcnsf",
        "frontend": "source-mel",
        "extra": "",
    },
}


def _model_path(args: argparse.Namespace, key: str) -> Path | None:
    explicit = {
        "nhv": args.nhv_model,
        "pcnsf": args.pcnsf_model,
        "rmvpe": args.rmvpe_model,
    }[key]
    if explicit is not None:
        return explicit.expanduser().resolve()
    if args.models_dir is not None:
        return (args.models_dir / MODEL_INFO[key]["file"]).expanduser().resolve()
    return None


def _path_comment(path: Path | None, key: str) -> list[str]:
    info = MODEL_INFO[key]
    sha_label = "sha256 (extracted onnx)" if key in ("pcnsf", "rmvpe") else "sha256"
    lines = [f"# {info['license']}", f"# download: {info['url']}",
             f"# {sha_label}: {info['sha256']}"]
    if path is None:
        lines.append("# Model path is a placeholder; pass --models-dir or the matching --*-model option.")
    return lines


def _toml_path(path: Path | None, key: str) -> str:
    if path is None:
        return f'"/absolute/path/to/{MODEL_INFO[key]["file"]}"'
    return '"' + str(path).replace("\\", "\\\\").replace('"', '\\"') + '"'


def build_config(scenario: str, args: argparse.Namespace) -> str:
    spec = SCENARIOS[scenario]
    neural_key = spec.get("neural")
    rmvpe = bool(spec.get("rmvpe"))
    frontend = spec.get("frontend", "world")
    model_path = _model_path(args, neural_key) if neural_key else None
    rmvpe_path = _model_path(args, "rmvpe") if rmvpe else None

    lines = [
        "# Generated by scripts/lessampler_config.py",
        f"# scenario: {scenario}",
        f"# {spec['title']}",
        "# Model weights are not distributed by lessampler.",
        "",
        "[config]",
        f'version = "{_config_version()}"',
        "debug = false",
        "",
        "[audio_model]",
        f'backend = "{spec["backend"]}"',
        f"frame_period = {FRAME_PERIOD}",
        'fft_size = "auto"',
        "model_amp = 0.85",
        "",
        "[f0]",
        f'estimator = "{spec["f0"]}"',
        "f0_speed = 1",
        "f0_dio_floor = 40.0",
        "f0_harvest_floor = 40.0",
        "f0_ceil = 800.0",
        "f0_cheap_trick_floor = 71.0",
        "f0_allow_range = 0.1",
    ]
    if rmvpe:
        lines.extend(_path_comment(rmvpe_path, "rmvpe"))
        lines.extend([
            f"neural_model_path = {_toml_path(rmvpe_path, 'rmvpe')}",
            "neural_threads = 0",
            "neural_threshold = 0.03",
            "neural_floor = 40.0",
            "neural_ceil = 1200.0",
        ])
    lines.extend([
        "",
        "[ap]",
        "ap_threshold = 0.1",
        "",
    ])
    if neural_key:
        lines.extend([
            "[neural]",
            f'frontend = "{frontend}"',
            "source_hop = 128",
            f'preset = "{"pc-nsf-hifigan" if neural_key == "pcnsf" else "nhv"}"',
        ])
        lines.extend(_path_comment(model_path, neural_key))
        lines.extend([
            f"model_path = {_toml_path(model_path, neural_key)}",
            "threads = 0",
            "target_rms = 0.111",
            "",
        ])
    lines.extend([
        "[master]",
        f'preset = "{spec["master"]}"',
        "",
        "[style]",
        'default = "natural"',
        "amount = 1.0",
        "morph_ms = 0.0",
        "",
    ])
    return "\n".join(lines)


def validate_models(scenario: str, args: argparse.Namespace) -> list[str]:
    spec = SCENARIOS[scenario]
    warnings = []
    keys = []
    if spec.get("neural"):
        keys.append(spec["neural"])
    if spec.get("rmvpe"):
        keys.append("rmvpe")
    for key in keys:
        path = _model_path(args, key)
        if path is None:
            warnings.append(f"{key}: no path supplied; the config contains a placeholder")
        elif not path.is_file():
            warnings.append(f"{key}: file not found: {path}")
    return warnings


def print_scenarios() -> None:
    print("scenario                 backend  f0       model        command")
    print("------------------------ -------- -------- ------------ ------------------------")
    for name, spec in SCENARIOS.items():
        model = spec.get("neural") or "-"
        if spec.get("rmvpe"):
            model = f"{model}+rmvpe" if model != "-" else "rmvpe"
        extra = spec.get("extra") or ""
        print(f"{name:24s} {spec['backend']:8s} {spec['f0']:8s} {model:12s} {extra}")


def choose_scenario() -> str:
    print("Available scenarios:")
    for index, (name, spec) in enumerate(SCENARIOS.items(), start=1):
        print(f"  {index:2d}. {name:24s} {spec['title']}")
    try:
        answer = input("Scenario number or name: ").strip()
    except EOFError as error:
        raise SystemExit("no scenario selected") from error
    if answer.isdigit():
        index = int(answer)
        if 1 <= index <= len(SCENARIOS):
            return list(SCENARIOS)[index - 1]
    if answer in SCENARIOS:
        return answer
    raise SystemExit(f"unknown scenario: {answer}")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Generate a lessampler.less preset")
    parser.add_argument("--scenario", help="scenario id; omitted means an interactive choice")
    parser.add_argument("--list", action="store_true", help="list scenarios and exit")
    parser.add_argument("--output", type=Path, help="output file (default: ./lessampler.less)")
    parser.add_argument("--models-dir", type=Path, help="directory containing model files")
    parser.add_argument("--nhv-model", type=Path, help="NHVSing ONNX file")
    parser.add_argument("--pcnsf-model", type=Path, help="PC-NSF-HiFiGAN ONNX file")
    parser.add_argument("--rmvpe-model", type=Path, help="RMVPE ONNX file")
    parser.add_argument("--dry-run", action="store_true", help="print TOML instead of writing")
    parser.add_argument("--force", action="store_true", help="overwrite an existing output file")
    args = parser.parse_args(argv)

    if args.list:
        print_scenarios()
        return 0

    scenario = args.scenario or choose_scenario()
    if scenario not in SCENARIOS:
        parser.error(f"unknown scenario: {scenario}")

    if args.output is None:
        args.output = Path("lessampler.less")
    args.output = args.output.expanduser()

    text = build_config(scenario, args)
    for warning in validate_models(scenario, args):
        print(f"[warn] {warning}", file=sys.stderr)

    if args.dry_run:
        print(text, end="")
        return 0

    if args.output.exists() and not args.force:
        raise SystemExit(f"output exists: {args.output} (use --force to overwrite)")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(text, encoding="utf-8")
    spec = SCENARIOS[scenario]
    extra = f" (render with {spec['extra']})" if spec.get("extra") else ""
    print(f"wrote {args.output} for scenario {scenario}{extra}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
