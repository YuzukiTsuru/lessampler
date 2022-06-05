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
// Created by gloom on 2022/5/20.
//

#ifndef LESSAMPLER_SYNTHESIS_H
#define LESSAMPLER_SYNTHESIS_H

#include <iostream>

#include "AudioModel/lessAudioModel.h"

class Synthesis {
public:
    explicit Synthesis(lessAudioModel audioModel, int x_length);

    ~Synthesis();

    double *GetWavData();

private:
    double *x = nullptr;
    int x_length = 0;

    lessAudioModel audioModel{};

private:
    void AllocateMemory();

    void SynthesisWav() const;
};


#endif //LESSAMPLER_SYNTHESIS_H
