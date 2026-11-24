/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_AUTOAMP_H
#define LESSAMPLER_AUTOAMP_H

#include <span>

namespace lessampler {

    class AutoAMP {
    public:
        /*
     * Normalize and clamp the samples in place. No temporary buffer is allocated.
     */
        static void ApplyInPlace(std::span<double> samples, double volume);

        [[nodiscard]] static double MaxAmplitude(std::span<const double> samples);

    private:
        static constexpr double MaxValue = 1.0;
        static constexpr double MinValue = -1.0;
    };

}// namespace lessampler

#endif//LESSAMPLER_AUTOAMP_H
