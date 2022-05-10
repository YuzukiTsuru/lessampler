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

#ifndef LESSAMPLER_AUDIOMODEL_H
#define LESSAMPLER_AUDIOMODEL_H

#include "lessAudioModel.h"

#include "ConfigUnit/lessConfigure.h"
#include "WorldModule/WorldPara.h"

class AudioModel {
public:
    AudioModel(double *x, int x_length, int fs, const lessConfigure &configure);

    lessAudioModel GetAudioModel();

    void SaveAudioModelToJsonFile();

private:
    lessConfigure configure{};
    WorldPara worldPara{};
    lessAudioModel lessAudioModel{};

private:
    void InitAudioModel();
};


#endif //LESSAMPLER_AUDIOMODEL_H
