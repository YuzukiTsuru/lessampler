/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Audio/Dsp/GrowlShaper.h"

#include <cmath>
#include <cstddef>
#include <numbers>

namespace lessampler {

    namespace {

        constexpr double kModulationHz = 27.0;
        constexpr double kModulationDepth = 0.35;
        constexpr double kDrive = 3.0;

    }// namespace

    void GrowlShaper::Apply(std::span<double> samples, int sample_rate, const StyleWeights &weights) {
        VoiceStyle style;
        style.start = weights;
        style.end = weights;
        Apply(samples, sample_rate, style);
    }

    void GrowlShaper::Apply(std::span<double> samples, int sample_rate, const VoiceStyle &style) {
        if (samples.empty() || sample_rate <= 0 || !style.active()) {
            return;
        }
        if (style.start.growl == 0.0 && style.end.growl == 0.0) {
            return;
        }

        const double duration_ms = 1000.0 * static_cast<double>(samples.size()) / static_cast<double>(sample_rate);
        const double phase_increment = 2.0 * std::numbers::pi * kModulationHz / static_cast<double>(sample_rate);
        double phase = 0.0;
        for (std::size_t i = 0; i < samples.size(); ++i) {
            const double elapsed_ms = 1000.0 * static_cast<double>(i) / static_cast<double>(sample_rate);
            const double weight = StyleAt(style, elapsed_ms, duration_ms).growl;
            if (weight > 0.0) {
                const double drive = 1.0 + kDrive * weight;
                const double wet = std::tanh(samples[i] * drive) / std::tanh(drive);
                const double modulation = 1.0 - kModulationDepth * weight * (0.5 + 0.5 * std::sin(phase));
                samples[i] = samples[i] * (1.0 - weight) + wet * modulation * weight;
            }
            phase += phase_increment;
            if (phase >= 2.0 * std::numbers::pi) {
                phase -= 2.0 * std::numbers::pi;
            }
        }
    }

}// namespace lessampler
