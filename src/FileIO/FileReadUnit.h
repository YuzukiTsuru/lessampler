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
// @Author Ghost Gloomy on 2020/1/26.
//
#ifndef LESSAMPLER_FILEREADUNIT_H
#define LESSAMPLER_FILEREADUNIT_H

#include <iostream>
#include <filesystem>


extern "C" {
#include <sndfile.h>
}

#define ENABLE_SNDFILE_WINDOWS_PROTOTYPES 1

class FileReadUnit {
public:
    /*
     * Get Audio Length
     */
    static int GetAudioLength(const char *filename);

    /*
     * Read Audio
     */
    static int WavRead(const char *FilePath, double *output);
};

#endif //LESSAMPLER_FILEREADUNIT_H
