#!/usr/bin/env python3
"""Unit tests for the lessampler.less configurator."""
from __future__ import annotations

import argparse
import io
import tempfile
import unittest
from contextlib import redirect_stdout
from pathlib import Path

import lessampler_config as configurator

try:
    import tomllib
except ModuleNotFoundError:  # Python < 3.11
    tomllib = None


class ConfiguratorTest(unittest.TestCase):
    def test_all_scenarios_render_valid_toml(self) -> None:
        if tomllib is None:
            self.skipTest("tomllib requires Python 3.11+")
        with tempfile.TemporaryDirectory() as temporary:
            models = Path(temporary) / "models"
            for name in configurator.SCENARIOS:
                args = argparse.Namespace(
                    scenario=name,
                    output=Path(temporary) / f"{name}.less",
                    models_dir=models,
                    nhv_model=None,
                    pcnsf_model=None,
                    rmvpe_model=None,
                    dry_run=False,
                    force=True,
                    list=False,
                )
                text = configurator.build_config(name, args)
                parsed = tomllib.loads(text)
                spec = configurator.SCENARIOS[name]
                self.assertEqual(parsed["audio_model"]["backend"], spec["backend"])
                self.assertEqual(parsed["f0"]["estimator"], spec["f0"])
                self.assertEqual(parsed["master"]["preset"], spec["master"])
                if spec.get("neural"):
                    self.assertIn("neural", parsed)
                    self.assertEqual(parsed["neural"]["source_hop"], 128)
                if spec.get("rmvpe"):
                    self.assertIn("neural_model_path", parsed["f0"])
                if spec.get("frontend"):
                    self.assertEqual(parsed["neural"]["frontend"], spec["frontend"])

    def test_models_dir_makes_absolute_paths(self) -> None:
        args = argparse.Namespace(
            scenario="nhv-note",
            output=Path("/tmp/out.less"),
            models_dir=Path("relative-models"),
            nhv_model=None,
            pcnsf_model=None,
            rmvpe_model=None,
            dry_run=True,
            force=False,
            list=False,
        )
        text = configurator.build_config("nhv-note", args)
        self.assertIn("nhv_v3_3.onnx", text)
        self.assertIn(str(Path("relative-models").resolve()), text)

    def test_explicit_model_path_wins(self) -> None:
        args = argparse.Namespace(
            scenario="pcnsf-note",
            output=Path("/tmp/out.less"),
            models_dir=Path("ignored"),
            nhv_model=None,
            pcnsf_model=Path("/tmp/custom.onnx"),
            rmvpe_model=None,
            dry_run=True,
            force=False,
            list=False,
        )
        text = configurator.build_config("pcnsf-note", args)
        self.assertIn("/tmp/custom.onnx", text)

    def test_list_output(self) -> None:
        buffer = io.StringIO()
        with redirect_stdout(buffer):
            configurator.print_scenarios()
        output = buffer.getvalue()
        self.assertIn("nhv-note", output)
        self.assertIn("pcnsf-note", output)
        self.assertIn("--polish", output)
        self.assertIn("--feature", output)

    def test_output_is_not_overwritten_without_force(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / "lessampler.less"
            output.write_text("keep", encoding="utf-8")
            with self.assertRaises(SystemExit):
                configurator.main([
                    "--scenario", "world-baseline",
                    "--output", str(output),
                ])
            self.assertEqual(output.read_text(encoding="utf-8"), "keep")


if __name__ == "__main__":
    unittest.main()
