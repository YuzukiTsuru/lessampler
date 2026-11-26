#!/usr/bin/env python3
"""Unit tests for the host-side automatic expression helpers."""
from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

import render_ust as ru


class EffectiveVbrTest(unittest.TestCase):
    def test_explicit_vbr_wins(self) -> None:
        note = {"VBR": "20,100,30,10,10,0"}
        self.assertEqual(ru.effective_vbr(note, 1000.0), ru.parse_vbr(note))

    def test_zero_vbr_blocks_auto_vibrato(self) -> None:
        self.assertIsNone(ru.effective_vbr({"VBR": "0,100,0"}, 1000.0))

    def test_long_plain_note_gets_vibrato(self) -> None:
        vbr = ru.effective_vbr({}, 1000.0, ru.default_expression())
        self.assertIsNotNone(vbr)
        length, period, depth, fade_in, fade_out, phase = vbr
        self.assertAlmostEqual(length, 82.0)
        self.assertAlmostEqual(period, 1000.0 / 5.4)
        self.assertAlmostEqual(depth, 28.0)
        self.assertAlmostEqual(fade_in, 100.0 * 200.0 / 820.0)
        self.assertEqual(fade_out, 0.0)
        self.assertEqual(phase, 0.0)

    def test_short_note_stays_plain(self) -> None:
        self.assertIsNone(ru.effective_vbr({}, 500.0))

    def test_explicit_pitch_curve_blocks_auto_vibrato(self) -> None:
        self.assertIsNone(ru.effective_vbr({"PBS": "0,0"}, 1000.0))
        self.assertIsNone(ru.effective_vbr({"PBY": "10"}, 1000.0))

    def test_disabled_vibrato(self) -> None:
        expression = ru.default_expression()
        expression["vibrato"]["enabled"] = False
        self.assertIsNone(ru.effective_vbr({}, 1000.0, expression))


class PortamentoTest(unittest.TestCase):
    def test_adjacent_plain_note_gets_glide(self) -> None:
        glide = ru.auto_portamento_glide({}, 60, 64, 50.0, 1000.0, ru.default_expression()["portamento"])
        self.assertEqual(glide, 70.0)

    def test_large_gap_or_interval_is_ignored(self) -> None:
        options = ru.default_expression()["portamento"]
        self.assertIsNone(ru.auto_portamento_glide({}, 60, 64, 500.0, 1000.0, options))
        self.assertIsNone(ru.auto_portamento_glide({}, 60, 73, 50.0, 1000.0, options))

    def test_explicit_curve_or_short_note_is_ignored(self) -> None:
        options = ru.default_expression()["portamento"]
        self.assertIsNone(ru.auto_portamento_glide({"PBS": "0,0"}, 60, 64, 50.0, 1000.0, options))
        self.assertIsNone(ru.auto_portamento_glide({}, 60, 64, 50.0, 50.0, options))

    def test_disabled_portamento(self) -> None:
        expression = ru.default_expression()
        expression["portamento"]["enabled"] = False
        self.assertIsNone(
            ru.auto_portamento_glide({}, 60, 64, 50.0, 1000.0, expression["portamento"])
        )

    def test_fallback_points_hold_then_glide(self) -> None:
        self.assertEqual(
            ru.fallback_pitch_points(-200.0, 40.0, 70.0),
            [(-200.0, 40.0), (-70.0, 40.0), (0.0, 0.0)],
        )
        self.assertEqual(
            ru.fallback_pitch_points(-200.0, 40.0, None),
            [(-200.0, 40.0), (0.0, 0.0)],
        )
        self.assertEqual(
            ru.fallback_pitch_points(-200.0, 0.0, 70.0),
            [(-200.0, 0.0), (0.0, 0.0)],
        )


class ExpressionFileTest(unittest.TestCase):
    def test_toml_overrides_defaults(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "expression.toml"
            path.write_text(
                "[vibrato]\nrate_hz = 6.0\ndepth_cents = 20\nenabled = false\n"
                "[breath]\nlevel = 0.45\n",
                encoding="utf-8",
            )
            expression = ru.load_expression(path)
        self.assertFalse(expression["vibrato"]["enabled"])
        self.assertEqual(expression["vibrato"]["rate_hz"], 6.0)
        self.assertEqual(expression["vibrato"]["depth_cents"], 20.0)
        self.assertEqual(expression["breath"]["level"], 0.45)
        self.assertEqual(expression["portamento"]["glide_ms"], 70.0)

    def test_invalid_file_is_reported(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "expression.toml"
            path.write_text("[vibrato\n", encoding="utf-8")
            with self.assertRaises(SystemExit):
                ru.load_expression(path)


if __name__ == "__main__":
    unittest.main()
