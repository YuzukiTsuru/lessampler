/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_GENERATEAUDIOMODEL_H
#define LESSAMPLER_GENERATEAUDIOMODEL_H

#include <filesystem>
#include <vector>

#include "Config/Config.h"

class VoicebankModeler {
public:
    VoicebankModeler(const std::filesystem::path &path, Config configure);

    /*
     * Analyze one WAV file and write its .lessaudio sidecar.
     */
    static void GenerateModelForFile(const std::filesystem::path &wav_path, const Config &configure);

    void PrintWavFiles() const;

private:
    std::filesystem::path target_voice_path;
    std::vector<std::filesystem::path> wav_files;
    Config configure;

private:
    void GetWavFileLists();

    void GenerateModelFromFile();
};

#endif//LESSAMPLER_GENERATEAUDIOMODEL_H
