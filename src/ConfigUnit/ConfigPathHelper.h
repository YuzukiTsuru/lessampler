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
// Created by gloom on 2022/8/7.
//

#ifndef LESSAMPLER_CONFIGPATHHELPER_H
#define LESSAMPLER_CONFIGPATHHELPER_H

#include <filesystem>

class ConfigPathHelper {
public:
    explicit ConfigPathHelper() = default;

    void SetExecPath(const std::filesystem::path &exec_path);

    std::filesystem::path GetConfigPath();

    bool config_is_exec();

private:
    std::filesystem::path exec_path = {};
    std::filesystem::path exec_config = {};

private:
    bool is_exec = false;
    bool is_voicebank = false;
};


#endif //LESSAMPLER_CONFIGPATHHELPER_H
