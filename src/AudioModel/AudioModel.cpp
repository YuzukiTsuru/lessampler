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

#include "FileIO/BinUnit.h"

#include "AudioModel.h"
#include "WorldModule/WorldModule.h"

AudioModel::AudioModel(double *x, int x_length, int fs, const lessConfigure &configure) : configure(configure) {
    lessAudioModel.x = x;
    lessAudioModel.x_length = x_length;
    lessAudioModel.fs = fs;

    // initialize the audio model from x, x_length, fs using World Vocoder
    WorldModule model(x, x_length, fs, configure);
    worldPara = model.GetModule();
    InitAudioModel();
}

void AudioModel::InitAudioModel() {
    lessAudioModel.fft_size = worldPara.fft_size;
    lessAudioModel.frame_period = worldPara.frame_period;
    lessAudioModel.f0 = worldPara.f0;
    lessAudioModel.f0_length = worldPara.f0_length;
    lessAudioModel.time_axis = worldPara.time_axis;
    lessAudioModel.spectrogram = worldPara.spectrogram;
    lessAudioModel.speclength = [&]() -> int * {
        auto _temp = new int[lessAudioModel.f0_length];
        for (int i = 0; i < lessAudioModel.f0_length; ++i) {
            _temp[i] = lessAudioModel.fft_size / 2 + 1;
        }
        return _temp;
    }();
    lessAudioModel.aperiodicity = worldPara.aperiodicity;
    lessAudioModel.aperiodlength = [&]() -> int * {
        auto _temp = new int[lessAudioModel.f0_length];
        for (int i = 0; i < lessAudioModel.f0_length; ++i) {
            _temp[i] = lessAudioModel.fft_size / 2 + 1;
        }
        return _temp;
    }();
}

void AudioModel::SaveAudioModelToJsonFile() {

}
