/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_WAVIO_H
#define LESSAMPLER_WAVIO_H

#include <filesystem>
#include <span>
#include <vector>

struct WavData {
    std::vector<double> samples;
    int sample_rate = 0;
    int channels = 0;
};

class WavIO {
public:
    /*
     * Read a WAV file as mono. Multi-channel input is averaged per frame.
     */
    static WavData ReadWav(const std::filesystem::path &path);

    /*
     * Write mono 16-bit PCM.
     */
    static void WriteWav(const std::filesystem::path &path, std::span<const double> x, int fs);
};

#endif//LESSAMPLER_WAVIO_H
