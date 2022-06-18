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
#include <memory>

#include "FileIO/AudioModelIO.h"
#include "FileIO/JSONFileIO.h"

#include "Utils/VectorWrapper.h"

#include "AudioModel.h"
#include "WorldModule/WorldModule.h"

AudioModel::AudioModel(double *x, size_t x_length, int fs, const lessConfigure &configure) : configure(configure) {
    _lessAudioModel.x.reserve(x_length);
    _lessAudioModel.x.insert(_lessAudioModel.x.end(), x, x + x_length);

    _lessAudioModel.x_length = x_length;
    _lessAudioModel.fs = fs;

    // initialize the audio model from x, x_length, fs using World Vocoder
    WorldModule model(_lessAudioModel.x, _lessAudioModel.fs, configure);
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

    _lessAudioModel.f0.reserve(worldPara.f0_length);
    _lessAudioModel.f0.insert(_lessAudioModel.f0.end(), worldPara.f0, worldPara.f0 + worldPara.f0_length);

    _lessAudioModel.time_axis.reserve(worldPara.f0_length);
    _lessAudioModel.time_axis.insert(_lessAudioModel.time_axis.end(), worldPara.time_axis, worldPara.time_axis + worldPara.f0_length);

    _lessAudioModel.w_length = worldPara.fft_size / 2 + 1;

    _lessAudioModel.spectrogram.reserve(worldPara.f0_length);
    for (int i = 0; i < worldPara.f0_length; ++i) {
        _lessAudioModel.spectrogram[i].reserve(_lessAudioModel.w_length);
        _lessAudioModel.spectrogram[i].insert(_lessAudioModel.spectrogram[i].end(), worldPara.spectrogram[i],
                                              worldPara.spectrogram[i] + _lessAudioModel.w_length);
    }

    _lessAudioModel.aperiodicity.reserve(worldPara.f0_length);
    for (int i = 0; i < worldPara.f0_length; ++i) {
        _lessAudioModel.aperiodicity[i].reserve(_lessAudioModel.w_length);
        _lessAudioModel.aperiodicity[i].insert(_lessAudioModel.aperiodicity[i].end(), worldPara.aperiodicity[i],
                                               worldPara.aperiodicity[i] + _lessAudioModel.w_length);
    }
}