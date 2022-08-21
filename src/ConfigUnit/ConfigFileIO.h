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
// Created by gloom on 2022/8/20.
//

#ifndef LESSAMPLER_CONFIGFILEIO_H
#define LESSAMPLER_CONFIGFILEIO_H

#include <fstream>
#include <filesystem>

#include "Utils/exception.h"

class ConfigFileIO {
public:
    inline static std::string read_config_file(const std::filesystem::path &config_file_path){
        // read config file
        std::ifstream file(config_file_path);
        if (!file.is_open()) {
            throw file_open_error(config_file_path.string());
        }
        std::string config_file = std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        file.close();
        return config_file;
    };

    inline static void save_config_file(const std::filesystem::path &config_file_path, const std::string &config_file){
        // read config file
        std::ofstream file(config_file_path);
        if (!file.is_open()) {
            throw file_open_error(config_file_path.string());
        }
        file << config_file;
        file.close();
    };
};


#endif //LESSAMPLER_CONFIGFILEIO_H
