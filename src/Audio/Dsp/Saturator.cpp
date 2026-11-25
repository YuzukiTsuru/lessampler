/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Audio/Dsp/Saturator.h"

#include <algorithm>
#include <cmath>

namespace lessampler {

    void Saturator::Process(std::span<double> samples, double drive, double mix) {
        if (samples.empty() || drive <= 0.0 || mix <= 0.0) {
            return;
        }
        const double dry = 1.0 - std::clamp(mix, 0.0, 1.0);
        const double wet = std::clamp(mix, 0.0, 1.0);
        const double normalization = std::tanh(drive);
        if (normalization <= 1e-9) {
            return;
        }
        for (double &value: samples) {
            const double saturated = std::tanh(drive * value) / normalization;
            value = dry * value + wet * saturated;
        }
    }

}// namespace lessampler
