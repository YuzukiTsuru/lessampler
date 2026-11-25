/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_FEATUREASSEMBLER_H
#define LESSAMPLER_FEATUREASSEMBLER_H

#include <cstddef>
#include <span>

#include "Config/Config.h"
#include "Vocoder/FeatureTypes.h"

namespace lessampler {

    /*
 * Assemble note-level mel/F0 features on one timeline and run the vocoder
 * once. The note buffers are WORLD/LLSM renders; each is normalized to the
 * model's target RMS, weighted by its UST envelope, and summed in linear mel
 * space. F0 is blended in log space by the same weights.
 */
    [[nodiscard]] FeatureAssembleResult AssembleFeatureTimeline(std::span<const FeatureSegment> segments,
                                                                int sample_rate, double peak_limit,
                                                                const Config &configure);

}// namespace lessampler

#endif//LESSAMPLER_FEATUREASSEMBLER_H
