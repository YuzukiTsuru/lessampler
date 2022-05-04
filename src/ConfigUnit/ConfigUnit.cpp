/*
 * Copyright (c)  2022, YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

//
// Created by gloom on 2022/5/4.
//

#include <fstream>
#include <filesystem>
#include "config.h"
#include "ConfigUnit.h"

ConfigUnit::ConfigUnit(const std::string &config_file_path) {
    this->config_file_path = config_file_path;
    if (std::filesystem::exists(config_file_path)) {
        read_config();
    }
}

void ConfigUnit::read_config() {
    // read config file
    std::ifstream file(this->config_file_path);
    if (!file.is_open()) {
        throw std::runtime_error("ConfigUnit file not found");
    }
    file >> this->config_file;
    file.close();
}

void ConfigUnit::create_default_config() {
    // create project info section
    this->config.add_section("config");
    inicpp::section &new_config = this->config["config"];
    new_config["version"] = PROJECT_GIT_HASH;

    // create audio model info
    this->config.add_section("audio_model");
    new_config = this->config["audio_model"];
    new_config["frame_period"] = "0.2";
    new_config["fft_size"] = "auto";

    // create info for f0
    this->config.add_section("f0");
    new_config = this->config["f0"];
    new_config["f0_mode"] = "DIO";
    new_config["f0_speed"] = "1";
    new_config["f0_dio_floor"] = "40.0";
    new_config["f0_harvest_floor"] = "40.0";
    new_config["f0_cheaptrick_floor"] = "71.0";
    new_config["f0_allow_range"] = "0.1";

    // create info for ap
    this->config.add_section("ap");
    new_config = this->config["ap"];
    new_config["ap_threshold"] = "0.85";
#if DEBUG_MODE
    new_config["debug"] = "1";
#else
    new_config["debug"] = "0";
#endif
}

void ConfigUnit::parse_config() {
    this->config = inicpp::parser::load(this->config_file);
}

void ConfigUnit::print_config() {
    std::cout << this->config_file << std::endl;
}
