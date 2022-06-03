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

#ifndef LESSAMPLER_LESSAMPLER_H
#define LESSAMPLER_LESSAMPLER_H

#include <iostream>
#include <filesystem>

#include "ConfigUnit/ConfigUnit.h"
#include "AudioModel/AudioModel.h"
#include "FileIO/AudioModelIO.h"

class lessampler {
public:
    lessampler(int argc, char **argv);

    void run() const;

private:
    int argc;
    char **argv;
    std::filesystem::path exec_path;
    lessConfigure configure;

private:
    void read_audio_file();

private:
    static void show_logo();
};


#endif //LESSAMPLER_LESSAMPLER_H
