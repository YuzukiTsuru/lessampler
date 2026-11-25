/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_DSP_SATURATOR_H
#define LESSAMPLER_DSP_SATURATOR_H

#include <span>

namespace lessampler {

    class Saturator {
    public:
        /*
         * Parallel tanh saturation. mix is 0..1 and drive is the tanh input
         * gain; the dry signal is always retained.
         */
        static void Process(std::span<double> samples, double drive, double mix);
    };

}// namespace lessampler

#endif//LESSAMPLER_DSP_SATURATOR_H
