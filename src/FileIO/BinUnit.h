//
// Created by Ghost Gloomy on 2020/8/27.
//

#ifndef LESSAMPLER_BINUNIT_H
#define LESSAMPLER_BINUNIT_H

#include <iostream>
#include <fstream>
#include <filesystem>
#include "AudioModel/lessAudioModel.h"

class BinUnit {
public:
    BinUnit(const std::string &Path, lessAudioModel audioModel);

    [[maybe_unused]] void SetFilePath(const std::string &Path);

    [[maybe_unused]] void SetAudioModel(lessAudioModel audioModel);

    [[maybe_unused]] std::string GetFilePath();

    [[maybe_unused]] lessAudioModel GetAudioModel();

    void SaveAudioModel();

    void ReadAudioModel();

private:
    lessAudioModel _audioModel{.f0_length = 0};
    std::filesystem::path FilePath{};

private:
    void WriteAudioContent();

    void ReadAudioContent();
};


#endif //LESSAMPLER_BINUNIT_H
