/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "AutoAMP.h"

#include <algorithm>
#include <cmath>

#include "Core/Log.h"

namespace lessampler {

    double AutoAMP::MaxAmplitude(std::span<const double> samples) {
        double maximum = 0.0;
        for (double value: samples) {
            if (!std::isnan(value)) {
                maximum = std::max(maximum, std::abs(value));
            }
        }
        return maximum;
    }

    void AutoAMP::ApplyInPlace(std::span<double> samples, double volume) {
        const double maximum = MaxAmplitude(samples);
        if (maximum == 0.0) {
            LOG_WARN << "Max AMP is Zero.";
            std::fill(samples.begin(), samples.end(), 0.0);
            return;
        }

        const auto scale = 0.5 * volume / maximum;
        for (double &value: samples) {
            value = std::isnan(value) ? 0.0 : std::clamp(value * scale, MinValue, MaxValue);
        }
    }

}// namespace lessampler
