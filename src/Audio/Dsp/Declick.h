/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_DSP_DECLICK_H
#define LESSAMPLER_DSP_DECLICK_H

#include <span>

namespace lessampler {

    /*
     * Raised-cosine fade at both ends of a note buffer. WORLD starts at an
     * arbitrary source phase and leaves a zero tail after the last rendered
     * frame; a short fade keeps those edges from becoming clicks.
     */
    class Declick {
    public:
        static void Apply(std::span<double> samples, int sample_rate, double fade_ms = 1.5);
    };

}// namespace lessampler

#endif//LESSAMPLER_DSP_DECLICK_H
