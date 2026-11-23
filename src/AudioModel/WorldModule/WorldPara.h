/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_WORLDPARA_H
#define LESSAMPLER_WORLDPARA_H

#include <vector>

#include "Utils/Matrix.h"

struct WorldPara {
    double frame_period = 5.0;
    int fs = 0;
    int fft_size = 0;

    std::vector<double> f0;
    std::vector<double> time_axis;
    Matrix spectrogram;
    Matrix aperiodicity;
};

#endif//LESSAMPLER_WORLDPARA_H
