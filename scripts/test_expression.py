#!/usr/bin/env python3
"""Unit tests for the host-side automatic expression helpers."""
from __future__ import annotations

import tempfile
import unittest
import wave
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


class BreathTest(unittest.TestCase):
    def test_breath_lyric_detection(self) -> None:
        for lyric in ("x", "X", "息", "吸", "吐", "・", "breath", "?x", "a R吐", "e R息F4"):
            self.assertTrue(ru.is_breath_lyric(lyric), lyric)
        for lyric in ("R", "あ", "a", ""):
            self.assertFalse(ru.is_breath_lyric(lyric), lyric)

    def test_breath_alias_from_lyric(self) -> None:
        self.assertEqual(ru.breath_alias("a R吐"), "a R吐")
        self.assertEqual(ru.breath_alias("e R息F4"), "e R息")
        self.assertEqual(ru.breath_alias("?i R吐A3"), "i R吐")
        self.assertIsNone(ru.breath_alias("x"))
        self.assertIsNone(ru.breath_alias("あ"))

    def test_breath_vowel(self) -> None:
        self.assertEqual(ru.breath_vowel("あ"), "a")
        self.assertEqual(ru.breath_vowel("か"), "a")
        self.assertEqual(ru.breath_vowel("し"), "i")
        self.assertEqual(ru.breath_vowel("ん"), "n")
        self.assertEqual(ru.breath_vowel("ア"), "a")
        self.assertEqual(ru.breath_vowel("ka"), "a")
        self.assertEqual(ru.breath_vowel("a あ"), "a")
        self.assertIsNone(ru.breath_vowel(""))

    def test_surrounding_lyric_skips_breath_and_rest(self) -> None:
        notes = [
            {"Lyric": "あ"},
            {"Lyric": "x"},
            {"Lyric": "R"},
            {"Lyric": "a R吐"},
            {"Lyric": "い"},
        ]
        self.assertEqual(ru.surrounding_lyric(notes, 1), "い")
        self.assertEqual(ru.surrounding_lyric([{"Lyric": "あ"}, {"Lyric": "x"}], 1), "あ")

    def test_breath_entry_prefers_matching_vowel(self) -> None:
        def entry(base: str, pitch: int | None) -> dict[str, object]:
            return {
                "base": base,
                "alias": f"{base}A3",
                "pitch": pitch,
                "folder": "A3",
                "pitch_tag": "A3",
            }

        entries = [
            entry("a R息", 57),
            entry("i R息", 57),
            entry("a R吐", 57),
        ]
        selected = ru.find_breath_entry(entries, "a", 60, {})
        self.assertIsNotNone(selected)
        self.assertEqual(selected["base"], "a R息")
        self.assertEqual(ru.find_breath_entry(entries, None, 60, {})["base"], "a R息")
        requested = ru.find_breath_entry(entries, None, 60, {}, "a R吐")
        self.assertIsNotNone(requested)
        self.assertEqual(requested["base"], "a R吐")

    def test_synth_breath_is_deterministic_and_bounded(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            first = Path(temporary) / "first.wav"
            second = Path(temporary) / "second.wav"
            ru.synthesize_breath(first, 44100, 300.0, 7, 0.15)
            ru.synthesize_breath(second, 44100, 300.0, 7, 0.15)
            self.assertEqual(first.read_bytes(), second.read_bytes())
            with wave.open(str(first), "rb") as handle:
                self.assertEqual(handle.getframerate(), 44100)
                self.assertEqual(handle.getnchannels(), 1)
                self.assertEqual(handle.getsampwidth(), 2)
                self.assertEqual(handle.getnframes(), 13230)

    def test_synth_breath_segment_fallback(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            work = Path(temporary)
            segment = ru.make_breath_segment(
                Path("lessampler"),
                work,
                "breath-test",
                None,
                44100,
                100.0,
                250.0,
                60,
                120.0,
                30.0,
                3,
                ru.DEFAULT_ENVELOPE,
            )
            self.assertIsNotNone(segment)
            self.assertTrue(Path(str(segment["input"])).is_file())
            self.assertEqual(segment["position_ms"], 100.0)
            self.assertEqual(segment["length_ms"], 250.0)

    def test_mismatched_sample_rate_falls_back_to_synth(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            work = Path(temporary)
            sample = work / "breath.wav"
            with wave.open(str(sample), "wb") as handle:
                handle.setnchannels(1)
                handle.setsampwidth(2)
                handle.setframerate(22050)
                handle.writeframes(b"\x00\x00" * 2205)
            entry = {
                "wav": sample,
                "offset": 0.0,
                "consonant": 0.0,
                "cutoff": 0.0,
            }
            segment = ru.make_breath_segment(
                Path("lessampler"),
                work,
                "breath-mismatch",
                entry,
                44100,
                0.0,
                200.0,
                60,
                120.0,
                30.0,
                4,
                ru.DEFAULT_ENVELOPE,
            )
            self.assertIsNotNone(segment)
            self.assertTrue(str(segment["input"]).endswith("-synth.wav"))


class AutoBreathSlotTest(unittest.TestCase):
    @staticmethod
    def note(start: float, duration: float, p_eff: float = 100.0) -> dict[str, str]:
        return {
            "_start": f"{start:.6f}",
            "_duration": f"{duration:.6f}",
            "_p_eff": f"{p_eff:.6f}",
            "_midi": "60",
            "_lyric": "あ",
        }

    def test_phrase_gap_gets_one_breath(self) -> None:
        notes = [self.note(0.0, 1000.0), self.note(1500.0, 1000.0)]
        slots = ru.auto_breath_slots(notes, [], ru.default_expression()["breath"])
        self.assertEqual(len(slots), 1)
        self.assertEqual(slots[0]["index"], 1)
        self.assertAlmostEqual(float(slots[0]["duration"]), 300.0)
        self.assertAlmostEqual(float(slots[0]["start"]), 1080.0)

    def test_small_gap_is_ignored(self) -> None:
        notes = [self.note(0.0, 1000.0), self.note(1100.0, 1000.0)]
        self.assertEqual(ru.auto_breath_slots(notes, [], ru.default_expression()["breath"]), [])

    def test_explicit_breath_suppresses_auto(self) -> None:
        notes = [self.note(0.0, 1000.0), self.note(1500.0, 1000.0)]
        events = [{"start": 1200.0, "duration": 200.0}]
        self.assertEqual(ru.auto_breath_slots(notes, events, ru.default_expression()["breath"]), [])

    def test_disabled_breath(self) -> None:
        notes = [self.note(0.0, 1000.0), self.note(1500.0, 1000.0)]
        options = ru.default_expression()["breath"]
        options["enabled"] = False
        self.assertEqual(ru.auto_breath_slots(notes, [], options), [])


class ExpressionFileTest(unittest.TestCase):
    def test_toml_overrides_defaults(self) -> None:
        if ru.tomllib is None:
            self.skipTest("tomllib requires Python 3.11+")
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
        if ru.tomllib is None:
            self.skipTest("tomllib requires Python 3.11+")
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "expression.toml"
            path.write_text("[vibrato\n", encoding="utf-8")
            with self.assertRaises(SystemExit):
                ru.load_expression(path)


if __name__ == "__main__":
    unittest.main()
