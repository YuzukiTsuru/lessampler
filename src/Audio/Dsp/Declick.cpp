/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Audio/Dsp/Declick.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>

namespace lessampler {

    void Declick::Apply(std::span<double> samples, int sample_rate, double fade_ms) {
        if (samples.empty() || sample_rate <= 0 || fade_ms <= 0.0) {
            return;
        }

        const auto requested = static_cast<std::size_t>(
                std::llround(fade_ms * static_cast<double>(sample_rate) / 1000.0));
        const std::size_t fade = std::min({requested, samples.size() / 2, std::size_t{1} << 20});
        if (fade < 2) {
            return;
        }

        for (std::size_t i = 0; i < fade; ++i) {
            const double phase = static_cast<double>(i) / static_cast<double>(fade - 1);
            const double gain = 0.5 * (1.0 - std::cos(std::numbers::pi * phase));
            samples[i] *= gain;
            samples[samples.size() - 1 - i] *= gain;
        }
    }

}// namespace lessampler
