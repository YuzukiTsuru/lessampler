/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_WORLDMODELSTORE_H
#define LESSAMPLER_WORLDMODELSTORE_H

#include "Audio/SampleSource.h"
#include "Config/Config.h"
#include "Vocoder/World/WorldModel.h"

namespace lessampler {

    /*
 * Return the cached WORLD model for a sample, or build and save one. The PCM
 * is loaded lazily only on a cache miss. Cache-write failures are logged and do
 * not prevent rendering or batch modeling.
 */
    [[nodiscard]] WorldModel LoadOrBuildWorldModel(const SampleSource &source, const Config &configure,
                                                   int max_workers = 0);

}// namespace lessampler

#endif//LESSAMPLER_WORLDMODELSTORE_H
