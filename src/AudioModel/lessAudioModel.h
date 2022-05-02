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

class AudioModel {
public:
    double w_length;
    int x_length;
    double frame_period;
    int fs;

    double *x;
    double *f0;
    double *time_axis;
    int f0_length;

    double **spectrogram;
    int *speclength;
    double **aperiodicity;
    int *aperiodlength;
    int fft_size;
};

class TransAudioModel {
public:
    int t_f0_length;

    double *t_f0;
    double **t_spectrogram;
    int *t_speclength;
    double **t_aperiodicity;
    int *t_aperiodlength;
};

#endif //LESSAMPLER_LESSAUDIOMODEL_H
