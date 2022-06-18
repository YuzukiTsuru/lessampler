/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

//
// Created by gloom on 2022/5/2.
//

#ifndef LESSAMPLER_LESSAUDIOMODEL_H
#define LESSAMPLER_LESSAUDIOMODEL_H

#include <vector>

class lessAudioModel {
public:
    size_t x_length = 0;
    double frame_period = 0.0;
    int fs = 0;

    std::vector<double> x;
    //double *x = nullptr;
    std::vector<double> f0;
    //double *f0 = nullptr;
    std::vector<double> time_axis;
    //double *time_axis = nullptr;
    int f0_length = 0;
    int w_length = 0;


    std::vector<std::vector<double>> spectrogram;
    // double **spectrogram = nullptr;
    std::vector<std::vector<double>> aperiodicity;
    // double **aperiodicity = nullptr;
    int fft_size = 0;
};

#endif //LESSAMPLER_LESSAUDIOMODEL_H
