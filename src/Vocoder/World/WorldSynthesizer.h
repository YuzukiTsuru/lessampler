/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_WORLDSYNTHESIZER_H
#define LESSAMPLER_WORLDSYNTHESIZER_H

#include <vector>

#include "Vocoder/World/WorldModel.h"

namespace lessampler {

    /*
 * Render a WORLD model to exactly output_samples mono samples.
 */
    [[nodiscard]] std::vector<double> SynthesizeWorld(const WorldModel &model, int output_samples);

}// namespace lessampler

#endif//LESSAMPLER_WORLDSYNTHESIZER_H
