/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "AudioModel.h"

#include <utility>

#include "WorldModule/WorldModule.h"

AudioModel::AudioModel(std::span<const double> x, int fs, const lessConfigure &configure) {
    _lessAudioModel.x_length = x.size();
    _lessAudioModel.fs = fs;

    WorldModule module(x, fs, configure);
    WorldPara world = module.TakeModule();

    _lessAudioModel.fft_size = world.fft_size;
    _lessAudioModel.frame_period = world.frame_period;
    _lessAudioModel.w_length = world.fft_size / 2 + 1;
    _lessAudioModel.f0 = std::move(world.f0);
    _lessAudioModel.spectrogram = std::move(world.spectrogram);
    _lessAudioModel.aperiodicity = std::move(world.aperiodicity);
}

const lessAudioModel &AudioModel::GetAudioModel() const {
    return _lessAudioModel;
}
