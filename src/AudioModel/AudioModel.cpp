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
#include <memory>

#include "FileIO/AudioModelIO.h"
#include "FileIO/JSONFileIO.h"

#include "AudioModel.h"
#include "WorldModule/WorldModule.h"

AudioModel::AudioModel(double *x, int x_length, int fs, const lessConfigure &configure) : configure(configure) {
    _lessAudioModel.x = x;
    _lessAudioModel.x_length = x_length;
    _lessAudioModel.fs = fs;

    // initialize the audio model from x, x_length, fs using World Vocoder
    WorldModule model(x, x_length, fs, configure);
    worldPara = model.GetModule();
    InitAudioModel();
}

lessAudioModel AudioModel::GetAudioModel() {
    return _lessAudioModel;
}

void AudioModel::InitAudioModel() {
    _lessAudioModel.fft_size = worldPara.fft_size;
    _lessAudioModel.frame_period = worldPara.frame_period;
    _lessAudioModel.f0_length = worldPara.f0_length;

    _lessAudioModel.f0 = new double[worldPara.f0_length];
    std::memcpy(_lessAudioModel.f0, worldPara.f0, sizeof(double) * worldPara.f0_length);

    _lessAudioModel.time_axis = new double[worldPara.f0_length];
    std::memcpy(_lessAudioModel.time_axis, worldPara.time_axis, sizeof(double) * worldPara.f0_length);

    _lessAudioModel.w_length = worldPara.fft_size / 2 + 1;

    _lessAudioModel.spectrogram = new double *[worldPara.f0_length];
    std::memcpy(_lessAudioModel.spectrogram, worldPara.spectrogram, sizeof(double) * worldPara.f0_length);
    for (int i = 0; i < worldPara.f0_length; ++i) {
        _lessAudioModel.spectrogram[i] = new double[_lessAudioModel.w_length];
        std::memcpy(_lessAudioModel.spectrogram[i], worldPara.spectrogram[i], sizeof(double) * _lessAudioModel.w_length);
    }

    _lessAudioModel.aperiodicity = new double *[worldPara.f0_length];
    std::memcpy(_lessAudioModel.aperiodicity, worldPara.aperiodicity, sizeof(double) * worldPara.f0_length);
    for (int i = 0; i < worldPara.f0_length; ++i) {
        _lessAudioModel.aperiodicity[i] = new double[_lessAudioModel.w_length];
        std::memcpy(_lessAudioModel.aperiodicity[i], worldPara.aperiodicity[i], sizeof(double) * _lessAudioModel.w_length);
    }
}

void AudioModel::SaveAudioModelToJsonFile(const std::string &Path) {
    JSONFileIO jsonFileIo(_lessAudioModel);
    jsonFileIo.SaveFile(Path);
}

