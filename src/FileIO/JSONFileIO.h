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
// Created by Ghost Gloomy on 2020/8/27.
//

#ifndef LESSAMPLER_JSONFILEIO_H
#define LESSAMPLER_JSONFILEIO_H

#include <iostream>
#include "AudioModel/lessAudioModel.h"

class JSONFileIO {
public:
    JSONFileIO(lessAudioModel audioModel, std::string path);

private:
    lessAudioModel _audioModel;
    std::string Path;

private:
    void SaveJsonModel();
};


#endif //LESSAMPLER_JSONFILEIO_H
