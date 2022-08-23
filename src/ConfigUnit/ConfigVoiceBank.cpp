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
// Created by gloom on 2022/8/21.
//

#include <utility>

#include "StaticCast.h"
#include "ConfigVoiceBank.h"
#include "Utils/exception.h"

ConfigVoiceBank::ConfigVoiceBank(std::filesystem::path _voice_path, lessConfigure _configure) : voice_path(std::move(_voice_path)),
                                                                                                configure(std::move(_configure)) {
    SetVoiceConfig();
}

ConfigVoiceBank::ConfigVoiceBank(lessConfigure _configure) : configure(std::move(_configure)) {
    SetVoiceConfig();
}

void ConfigVoiceBank::SetVoiceConfig() {
    voice_config_path = voice_path / VOIICEBANKCONFIGFILENAME;
    if (voice_config_path.empty()) {
        throw file_open_error("Configure file: " + voice_config_path.string());
    }
}

void ConfigVoiceBank::parse_config() {
    config = inicpp::parser::load(config_file_data_string);
    // parse config file audio model section
    auto audio_model_section = config["audio_model"];
    configure.audio_model_frame_period = audio_model_section["frame_period"].get<inicpp::float_ini_t>();
    configure.model_amp = audio_model_section["model_amp"].get<inicpp::float_ini_t>();
    // check if fft_size is auto or not
    if (audio_model_section["fft_size"].get<inicpp::string_ini_t>() == "auto") {
        configure.fft_size = 0;
        configure.custom_fft_size = false;
    } else {
        std::stringstream ss;
        ss << audio_model_section["fft_size"].get<inicpp::string_ini_t>();
        ss >> configure.fft_size;
        configure.custom_fft_size = true;
    }

    // parse config file f0 section
    auto f0_section = config["f0"];
    configure.f0_mode = [&]() -> lessConfigure::F0_MODE {
        auto f0_mode = f0_section["f0_mode"].get<inicpp::string_ini_t>();
        // transform to upper cap
        std::transform(f0_mode.begin(), f0_mode.end(), f0_mode.begin(), ::toupper);
        if (f0_mode == "DIO")
            return lessConfigure::F0_MODE::F0_MODE_DIO;
        else if (f0_mode == "HARVEST")
            return lessConfigure::F0_MODE::F0_MODE_HARVEST;
        else
            return lessConfigure::F0_MODE::F0_MODE_UNKNOWN;
    }();

    configure.f0_speed = static_cast<int>(f0_section["f0_speed"].get<inicpp::signed_ini_t>());
    configure.f0_dio_floor = f0_section["f0_dio_floor"].get<inicpp::float_ini_t>();
    configure.f0_harvest_floor = f0_section["f0_harvest_floor"].get<inicpp::float_ini_t>();
    configure.f0_cheap_trick_floor = f0_section["f0_cheap_trick_floor"].get<inicpp::float_ini_t>();
    configure.f0_allow_range = f0_section["f0_allow_range"].get<inicpp::float_ini_t>();

    // parse config file ap section
    auto ap_section = config["ap"];
    configure.ap_threshold = ap_section["ap_threshold"].get<inicpp::float_ini_t>();
}
