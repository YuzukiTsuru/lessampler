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

#include "FileIO/AudioModelIO.h"

#include "AudioModel.h"

AudioModel::AudioModel(double *x, int x_length, int fs, const lessConfigure &configure) : configure(configure) {
    this->_lessAudioModel.x = x;
    this->_lessAudioModel.x_length = x_length;
    this->_lessAudioModel.fs = fs;

    // initialize the audio model from x, x_length, fs using World Vocoder
    worldModule.AnalysisModel(x, x_length, fs, configure);
    worldPara = worldModule.GetModule();
    InitAudioModel();
}

lessAudioModel AudioModel::GetAudioModel() {
    return this->_lessAudioModel;
}

void AudioModel::InitAudioModel() {
    this->_lessAudioModel.fft_size = worldPara.fft_size;
    this->_lessAudioModel.frame_period = worldPara.frame_period;
    this->_lessAudioModel.f0 = worldPara.f0;
    this->_lessAudioModel.f0_length = worldPara.f0_length;
    this->_lessAudioModel.time_axis = worldPara.time_axis;
    this->_lessAudioModel.spectrogram = worldPara.spectrogram;
    this->_lessAudioModel.aperiodicity = worldPara.aperiodicity;
}

void AudioModel::SaveAudioModelToJsonFile() {
    // TODO AT
}

