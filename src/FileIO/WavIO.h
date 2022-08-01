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
// Created by gloom on 2022/8/1.
//

#ifndef LESSAMPLER_WAVIO_H
#define LESSAMPLER_WAVIO_H

#include <filesystem>
#include <iostream>

class WavIO {
public:
    WavIO() = default;

    /*
     * Get Audio Length
     */
    static int GetAudioLength(const char *filename);

    /*
     * Read Audio
     */
    static int WavRead(const char *FilePath, double *output);

    static void WriteWav(const std::filesystem::path &path, double *x, long long x_length, int fs);

};


#endif //LESSAMPLER_WAVIO_H
