/*
 * Copyright (c)  2022, YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

//
// Created by gloom on 2022/5/2.
//

#ifndef LESSAMPLER_LESSAUDIOMODEL_H
#define LESSAMPLER_LESSAUDIOMODEL_H

class lessAudioModel {
public:
    int x_length = 0;
    double frame_period = 0.0;
    int fs = 0;

    double *x = nullptr;
    double *f0 = nullptr;
    double *time_axis = nullptr;
    int f0_length = 0;
    int w_length = 0;

    double **spectrogram = nullptr;
    double **aperiodicity = nullptr;
    int fft_size = 0;
};

#endif //LESSAMPLER_LESSAUDIOMODEL_H
