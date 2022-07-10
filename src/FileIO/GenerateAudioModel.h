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
// Created by gloom on 2022/5/23.
//

#ifndef LESSAMPLER_GENERATEAUDIOMODEL_H
#define LESSAMPLER_GENERATEAUDIOMODEL_H

#include <filesystem>
#include <vector>

#include "ConfigUnit/ConfigUnit.h"

class GenerateAudioModel {
public:
    GenerateAudioModel(std::filesystem::path path, lessConfigure configure);
    GenerateAudioModel(char *path, lessConfigure configure);

    void PrintWavFiles();

private:
    std::filesystem::path target_voice_path;
    std::vector<std::filesystem::path> wav_files;
    lessConfigure configure;

private:
    void GetWavFileLists();

    void GenerateModelFromFile();

    void WavFileModel(const std::filesystem::path &wav_path);

private:
    // Baisc Impl of Parallel foreach
    template<class I, class F>
    void for_each(size_t thread_count, I begin, I end, F f);

    // Compatible Impl of Parallel foreach
    template<class I, class F>
    void for_each(I begin, I end, F f);
};


#endif //LESSAMPLER_GENERATEAUDIOMODEL_H
