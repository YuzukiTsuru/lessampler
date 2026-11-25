/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_DSP_LIMITER_H
#define LESSAMPLER_DSP_LIMITER_H

#include <span>

namespace lessampler {

    /*
     * Offline look-ahead peak limiter. It is intended for a complete mix held
     * in memory, not for the incremental classic wavtool path.
     */
    class Limiter {
    public:
        static void Process(std::span<double> interleaved, int sample_rate, int channels,
                            double ceiling_db, double lookahead_ms = 5.0, double release_ms = 50.0);
    };

}// namespace lessampler

#endif//LESSAMPLER_DSP_LIMITER_H
