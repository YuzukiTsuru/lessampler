/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_NEURALFEATURES_H
#define LESSAMPLER_NEURALFEATURES_H

#include <cstddef>
#include <span>
#include <vector>

#include "Vocoder/Common/F0Estimator.h"

namespace lessampler {

    inline constexpr double neural_f0_min = 40.0;
    inline constexpr double neural_f0_max = 1200.0;

    /*
     * Resample an F0 track onto mel-frame times and fill unvoiced stretches
     * with interpolated F0. uv is 0 for voiced frames and 1 for unvoiced.
     */
    void BuildMelF0Track(const F0Track &track, std::size_t frames, std::vector<float> &f0,
                         std::vector<float> &uv);

    /*
     * Fill leading/trailing unvoiced F0 and interpolate internal gaps in place.
     * uv keeps its original values (0 voiced, non-zero unvoiced).
     */
    void FillUnvoicedF0(std::vector<float> &f0, std::span<const float> uv, std::size_t frames);

}// namespace lessampler

#endif//LESSAMPLER_NEURALFEATURES_H
