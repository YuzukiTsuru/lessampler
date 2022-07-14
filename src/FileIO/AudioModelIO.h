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
// Created by Ghost Gloomy on 2020/8/27.
//

#ifndef LESSAMPLER_AUDIOMODELIO_H
#define LESSAMPLER_AUDIOMODELIO_H

#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstring>

#include "AudioModel/lessAudioModel.h"
#include "ConfigUnit/lessConfigure.h"

class AudioModelIO {
public:
    AudioModelIO(std::filesystem::path Path, lessAudioModel audioModel, lessConfigure configure);

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
    lessConfigure _configure{};
    std::filesystem::path root_file_path{};
    std::filesystem::path in_file_path{};
    std::filesystem::path audio_model_file_path{};

protected:
    const std::string audio_model_file_ext = "lessaudio"; // lessaudio model file extition
    const char lessaudio_header[6] = {'s', 'h', 'i', 'n', 'e', '\0'};  // lessaudio model file header
    const char lessaudio_ending[5] = {'5', '4', '0', '2', '\0'};  // lessaudio model file end

private:
    std::ofstream WriteAudioContent();

    void ReadAudioContent();

    void GenerateFilePath();
};

#endif //LESSAMPLER_AUDIOMODELIO_H
