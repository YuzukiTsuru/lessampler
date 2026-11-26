/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_DSP_GROWLSHAPER_H
#define LESSAMPLER_DSP_GROWLSHAPER_H

#include <span>

#include "Core/VoiceStyle.h"

namespace lessampler {

    /*
     * Backend-independent growl voice color: a slow amplitude modulation plus
     * a bounded tanh saturation. The phase restarts at every note, and the
     * dry/wet mix follows the note's growl weight, so a note that morphs out
     * of growl returns to the untouched backend output.
     */
    class GrowlShaper {
    public:
        static void Apply(std::span<double> samples, int sample_rate, const VoiceStyle &style);

        static void Apply(std::span<double> samples, int sample_rate, const StyleWeights &weights);
    };

}// namespace lessampler

#endif//LESSAMPLER_DSP_GROWLSHAPER_H
