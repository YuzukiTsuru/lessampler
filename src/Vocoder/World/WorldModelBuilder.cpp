/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Vocoder/World/WorldModelBuilder.h"

#include <utility>

#include "Vocoder/World/WorldAnalyzer.h"

namespace lessampler {

    WorldModel BuildWorldModel(std::span<const double> samples, int fs, const Config &configure,
                               int max_workers) {
        WorldModel model;
        model.x_length = samples.size();
        model.fs = fs;

        WorldAnalyzer analyzer(samples, fs, configure, max_workers);
        WorldPara world = analyzer.TakeModule();

        model.fft_size = world.fft_size;
        model.frame_period = world.frame_period;
        model.w_length = world.fft_size / 2 + 1;
        model.f0 = std::move(world.f0);
        model.spectrogram = std::move(world.spectrogram);
        model.aperiodicity = std::move(world.aperiodicity);
        return model;
    }

}// namespace lessampler
