/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_AUDIOMODELIO_H
#define LESSAMPLER_AUDIOMODELIO_H

#include <filesystem>
#include <string_view>

#include "Analysis/World/WorldModel.h"
#include "Config/Config.h"

class WorldModelCache {
public:
    WorldModelCache(std::filesystem::path Path, const WorldModel &audioModel, Config configure);

    explicit WorldModelCache(std::filesystem::path Path);

    ~WorldModelCache();

    [[nodiscard]] const WorldModel &GetAudioModel() const;

    [[nodiscard]] bool CheckAudioModel(const Config &configure) const;

    void SaveAudioModel() const;

    [[nodiscard]] const WorldModel &ReadAudioModel(const Config &configure);

private:
    const WorldModel *_audioModel = nullptr;
    WorldModel _ownedModel{};
    Config _configure{};
    std::filesystem::path in_file_path{};
    std::filesystem::path audio_model_file_path{};

    static constexpr std::string_view audio_model_file_ext = "lessaudio";

private:
    void GenerateFilePath();
};

#endif//LESSAMPLER_AUDIOMODELIO_H
