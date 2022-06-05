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
// Created by gloom on 2022/6/5.
//

#ifndef LESSAMPLER_FILEWRITEUNIT_H
#define LESSAMPLER_FILEWRITEUNIT_H

#include <iostream>
#include <filesystem>

extern "C" {
#include <sndfile.h>
}

class FileWriteUnit {
public:
    static void WriteWav(const std::filesystem::path& path, double *x, long long x_length, int fs);
};


#endif //LESSAMPLER_FILEWRITEUNIT_H
