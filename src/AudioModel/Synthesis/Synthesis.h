// Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

//
// Created by gloom on 2022/5/20.
//

#ifndef LESSAMPLER_SYNTHESIS_H
#define LESSAMPLER_SYNTHESIS_H

#include <iostream>

#include "AudioModel/lessAudioModel.h"

class SynthesisPara {
public:
    int fs;
    int f0_length;
    double *f0;
    double **spectrogram;
    double **aperiodicity;
};

class Synthesis {
public:
    explicit Synthesis(lessAudioModel audioModel, int x_length);

private:
    double *x = nullptr;
    int x_length = 0;

    lessAudioModel audioModel{};

private:
    void AllocateMemory();

    void SynthesisWav() const;
};


#endif //LESSAMPLER_SYNTHESIS_H
