/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_RESAMPLE_H
#define LESSAMPLER_RESAMPLE_H

#include <cstddef>
#include <span>
#include <vector>

#include "Vocoder/Common/F0EstimatorTypes.h"

namespace lessampler {

    /*
     * Windowed-sinc resampler with a low-pass cutoff below the smaller
     * Nyquist frequency. Shared by the neural front end and RMVPE.
     */
    [[nodiscard]] std::vector<double> Resample(std::span<const double> samples, int from_rate, int to_rate);

    /*
     * Number of frames WORLD uses for the same duration and frame period.
     */
    [[nodiscard]] std::size_t F0FrameCount(std::size_t sample_count, int sample_rate, double frame_period_ms);

    /*
     * Resample an F0 track onto a new frame period. Both neighbours voiced
     * uses linear interpolation; both unvoiced stays zero; a mixed pair uses
     * the nearest integer frame so voiced frames are not smeared into
     * unvoiced ones.
     */
    [[nodiscard]] F0Track ResampleF0Track(const F0Track &source, std::size_t sample_count, int sample_rate,
                                          double frame_period_ms);

}// namespace lessampler

#endif//LESSAMPLER_RESAMPLE_H
