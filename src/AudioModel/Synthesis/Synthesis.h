/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_SYNTHESIS_H
#define LESSAMPLER_SYNTHESIS_H

#include <vector>

#include "AudioModel/lessAudioModel.h"

class Synthesis {
public:
    Synthesis(const lessAudioModel &audioModel, int x_length);

    [[nodiscard]] std::vector<double> &GetWavData() noexcept;

    [[nodiscard]] const std::vector<double> &GetWavData() const noexcept;

private:
    std::vector<double> x;

private:
    void SynthesisWav(const lessAudioModel &audioModel);
};

#endif//LESSAMPLER_SYNTHESIS_H
