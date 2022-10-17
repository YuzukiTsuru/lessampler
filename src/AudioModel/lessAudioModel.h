/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

//
// Created by gloom on 2022/5/2.
//

#ifndef LESSAMPLER_LESSAUDIOMODEL_H
#define LESSAMPLER_LESSAUDIOMODEL_H

#include <vector>

typedef struct lessAudioModel_ {
    double frame_period = 0.0;
    int fs = 0;
    int w_length = 0;
    int fft_size = 0;

    std::vector<double> x;
    std::vector<double> f0;
    std::vector<double> time_axis;
    std::vector<std::vector<double>> spectrogram;
    std::vector<std::vector<double>> aperiodicity;
} lessAudioModel;

#endif//LESSAMPLER_LESSAUDIOMODEL_H
