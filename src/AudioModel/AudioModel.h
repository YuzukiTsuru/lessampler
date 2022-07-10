/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

//
// Created by gloom on 2022/5/2.
//

#ifndef LESSAMPLER_AUDIOMODEL_H
#define LESSAMPLER_AUDIOMODEL_H

#include "lessAudioModel.h"

#include "ConfigUnit/lessConfigure.h"
#include "WorldModule/WorldPara.h"

class AudioModel {
public:
    AudioModel(double *x, int x_length, int fs, const lessConfigure &configure);

    lessAudioModel GetAudioModel();

private:
    lessConfigure configure{};
    WorldPara worldPara{};
    lessAudioModel _lessAudioModel{};

private:
    void InitAudioModel();
};


#endif //LESSAMPLER_AUDIOMODEL_H
