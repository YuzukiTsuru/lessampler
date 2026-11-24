/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_F0ESTIMATOR_H
#define LESSAMPLER_F0ESTIMATOR_H

#include <span>
#include <vector>

#include "Config/Config.h"

struct F0Track {
    double frame_period = 5.0;
    std::vector<double> f0;
    std::vector<double> time_axis;
};

/*
 * Backend-neutral F0 estimation. Both the WORLD and LLSM backends use the same
 * DIO/Harvest front end.
 */
[[nodiscard]] F0Track EstimateF0(std::span<const double> x, int fs, const Config &configure);

#endif//LESSAMPLER_F0ESTIMATOR_H
