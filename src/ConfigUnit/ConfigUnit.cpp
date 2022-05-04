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
        read_config_file();
        parse_config();
    } else {
        create_default_config();
        save_config_file();
    }
}

void ConfigUnit::read_config_file() {
    // read config file
    std::ifstream file(this->config_file_path);
    if (!file.is_open()) {
        throw std::runtime_error("ConfigUnit file not found");
    }
    file >> this->config_file;
    file.close();
}

void ConfigUnit::save_config_file() {
    // read config file
    std::ofstream file(this->config_file_path);
    if (!file.is_open()) {
        throw std::runtime_error("ConfigUnit file not found");
    }
    file << this->config_file;
    file.close();
}

void ConfigUnit::create_default_config() {
    // create project info section
    this->config.add_section("config");
    inicpp::section &new_config = this->config["config"];
    new_config["version"] = PROJECT_GIT_HASH;
#if DEBUG_MODE
    new_config["debug"] = "1";
#else
    new_config["debug"] = "0";
#endif

    // create audio model info
    this->config.add_section("audio_model");
    new_config = this->config["audio_model"];
    new_config["frame_period"] = "5.0";
    new_config["fft_size"] = "auto";

    // create info for f0
    this->config.add_section("f0");
    new_config = this->config["f0"];
    new_config["f0_mode"] = "DIO";
    new_config["f0_speed"] = "1";
    new_config["f0_dio_floor"] = "40.0";
    new_config["f0_harvest_floor"] = "40.0";
    new_config["f0_cheap_trick_floor"] = "71.0";
    new_config["f0_allow_range"] = "0.1";

    // create info for ap
    this->config.add_section("ap");
    new_config = this->config["ap"];
    new_config["ap_threshold"] = "0.85";
}

void ConfigUnit::parse_config() {
    this->config = inicpp::parser::load(this->config_file);
    // parse config file config section
    auto config_section = this->config["config"];
    this->configure.version = config_section["version"].get<inicpp::string_ini_t>();
    this->configure.debug_mode = config_section["debug"].get<inicpp::boolean_ini_t>();

    // parse config file audio model section
    auto audio_model_section = this->config["audio_model"];
    this->configure.audio_model_frame_period = audio_model_section["frame_period"].get<inicpp::float_ini_t>();
    // check if fft_size is auto or not
    if (audio_model_section["fft_size"].get<inicpp::string_ini_t>() == "auto") {
        this->configure.fft_size = 0;
    } else {
        std::stringstream ss;
        ss << audio_model_section["fft_size"].get<inicpp::string_ini_t>();
        ss >> this->configure.fft_size;
    }

    // parse config file f0 section
    auto f0_section = this->config["f0"];
    this->configure.f0_mode = f0_section["f0_mode"].get<inicpp::string_ini_t>();
    this->configure.f0_speed = static_cast<int>(f0_section["f0_speed"].get<inicpp::signed_ini_t>());
    this->configure.f0_dio_floor = f0_section["f0_dio_floor"].get<inicpp::float_ini_t>();
    this->configure.f0_harvest_floor = f0_section["f0_harvest_floor"].get<inicpp::float_ini_t>();
    this->configure.f0_cheap_trick_floor = f0_section["f0_cheap_trick_floor"].get<inicpp::float_ini_t>();
    this->configure.f0_allow_range = f0_section["f0_allow_range"].get<inicpp::float_ini_t>();

    // parse config file ap section
    auto ap_section = this->config["ap"];
    this->configure.ap_threshold = ap_section["ap_threshold"].get<inicpp::float_ini_t>();
}

void ConfigUnit::print_config() {
    std::cout << this->config_file << std::endl;
}

lessConfigure ConfigUnit::get_config() {
    return this->configure;
}
