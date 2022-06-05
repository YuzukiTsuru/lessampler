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

#include "FileWriteUnit.h"

#include <Utils/LOG.h>

#if (defined (__WIN32__) || defined (_WIN32)) && !defined (__MINGW32__)
#pragma warning(disable : 4996)
#endif

void FileWriteUnit::WriteWav(const std::filesystem::path &path, double *x, long long x_length, int fs) {
    SNDFILE *sf;
    SF_INFO info;
    info.channels = 1;
    info.samplerate = fs;
    info.frames = x_length;
    info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    sf = sf_open(path.string().c_str(), SFM_WRITE, &info);
    if (sf == nullptr) {
        YALL_ERROR_ << "Failed to open the file.";
        exit(-1);
    }

    sf_write_double(sf, x, x_length);

    sf_close(sf);
}
