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
// Created by Ghost Gloomy on 2020/8/27.
//

#ifndef LESSAMPLER_AUDIOMODELIO_H
#define LESSAMPLER_AUDIOMODELIO_H

#include <iostream>
#include <fstream>
#include <filesystem>
#include <parameterio.h>
#include <cstring>
#include "AudioModel/lessAudioModel.h"

class AudioModelIO {
public:
    AudioModelIO(std::filesystem::path Path, lessAudioModel audioModel);

    explicit AudioModelIO(std::filesystem::path Path);

    ~AudioModelIO();

    [[maybe_unused]] void SetFilePath(const std::filesystem::path &Path);

    [[maybe_unused]] void SetAudioModel(lessAudioModel audioModel);

    [[maybe_unused]] std::filesystem::path GetFilePath();

    [[maybe_unused]] lessAudioModel GetAudioModel();

    bool CheckAudioModel();

    void SaveAudioModel();

    lessAudioModel ReadAudioModel();

public:
    static bool CheckAudioModelFile(const std::filesystem::path &path);

private:
    lessAudioModel _audioModel{};
    std::filesystem::path root_file_path{};
    std::filesystem::path in_file_path{};
    std::filesystem::path audio_model_file_path{};

protected:
    const std::string audio_model_file_ext = "lessaudio";
    const char f0_length_header[5] = {'F', '0', 'L', ' ', '\0'};            // number of samples (int)
    const char frame_period_header[5] = {'F', 'P', ' ', ' ', '\0'};         // frame perido (double)
    const char fft_size_header[5] = {'F', 'F', 'T', ' ', '\0'};             // FFT size (int)
    const char x_length_header[5] = {'X', 'L', ' ', ' ', '\0'};             // x_length (int)
    const char fs_header[5] = {'F', 'S', ' ', ' ', '\0'};                   // frame per sample (int)
    const char f0_header[5] = {'5', '4', '0', '0', '\0'};                   // F0
    const char sp_header[5] = {'5', '4', '0', '1', '\0'};                   // SP
    const char ap_header[5] = {'5', '4', '0', '2', '\0'};                   // AP

private:
    void WriteAudioContent();

    void ReadAudioContent();

    void GenerateFilePath();
};


#endif //LESSAMPLER_AUDIOMODELIO_H
