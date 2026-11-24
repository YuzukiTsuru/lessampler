/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_WORLDMODELBUILDER_H
#define LESSAMPLER_WORLDMODELBUILDER_H

#include <span>

#include "Config/Config.h"
#include "Vocoder/World/WorldModel.h"

namespace lessampler {

    [[nodiscard]] WorldModel BuildWorldModel(std::span<const double> samples, int fs, const Config &configure,
                                             int max_workers = 0);

}// namespace lessampler

#endif//LESSAMPLER_WORLDMODELBUILDER_H
