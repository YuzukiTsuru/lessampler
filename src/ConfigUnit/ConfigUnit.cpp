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
// Created by gloom on 2022/5/4.
//

#include <fstream>
#include <filesystem>

#include "Utils/exception.h"
#include "lessconfig.h"
#include "ConfigUnit.h"

ConfigUnit::ConfigUnit(const std::filesystem::path &exec_path) {
    SetConfig(exec_path);
}

ConfigUnit::~ConfigUnit() = default;

void ConfigUnit::SetConfig(const std::filesystem::path &exec_path) {
    path_helper.SetExecPath(exec_path);
    this->config_file_path = path_helper.GetConfigPath();
    init_config();
}

void ConfigUnit::init_config() {
    make_schema();
    if (path_helper.is_exec) {
        YALL_DEBUG_ << "Config file exists, loading...";
        read_config_file();
        parse_config();
    } else {
        YALL_DEBUG_ << "Config file not exists, creating...";
        create_default_config();
        save_config_file();
        parse_config();
    }
}

void ConfigUnit::make_schema() {
    // create project info section
    inicpp::section_schema_params section_config_params{};
    section_config_params.name = "config";
    section_config_params.comment = "\n============ Gobal Settings ===========\n";
    section_config_params.requirement = inicpp::item_requirement::mandatory;
    config_schema.add_section(section_config_params);

    inicpp::option_schema_params<inicpp::string_ini_t> version{};
    version.name = "version";
    version.default_value = PROJECT_GIT_HASH;
    version.type = inicpp::option_item::single;
    config_schema.add_option("config", version);

    inicpp::option_schema_params<inicpp::boolean_ini_t> debug{};
    debug.name = "debug";
#if DEBUG_MODE
    debug.default_value = "1";
#else
    debug.default_value = "0";
#endif
    debug.type = inicpp::option_item::single;
    config_schema.add_option("config", debug);

    // create project audio model section
    inicpp::section_schema_params section_audio_model_params{};
    section_audio_model_params.name = "audio_model";
    section_audio_model_params.comment = "\n========= Audio Model Settings ========\n"
                                         "Note: modifying any of the parameters here will require remodeling the voice db\n";
    section_audio_model_params.requirement = inicpp::item_requirement::mandatory;
    config_schema.add_section(section_audio_model_params);

    inicpp::option_schema_params<inicpp::float_ini_t> frame_period{};
    frame_period.name = "frame_period";
    frame_period.default_value = std::to_string(configure.audio_model_frame_period);
    frame_period.type = inicpp::option_item::single;
    config_schema.add_option("audio_model", frame_period);

    inicpp::option_schema_params<inicpp::float_ini_t> fft_size{};
    fft_size.name = "fft_size";
    fft_size.default_value = "auto";
    fft_size.type = inicpp::option_item::single;
    config_schema.add_option("audio_model", fft_size);

    inicpp::option_schema_params<inicpp::float_ini_t> model_amp{};
    model_amp.name = "model_amp";
    model_amp.default_value = std::to_string(configure.model_amp);
    model_amp.type = inicpp::option_item::single;
    model_amp.comment = "Apply AutoAMP before Modeling, Suitable for sound bank with uneven volume\n"
                        "Set '0' as disable";
    config_schema.add_option("audio_model", model_amp);

    // create project f0 section
    inicpp::section_schema_params section_f0_params{};
    section_f0_params.name = "f0";
    section_f0_params.comment = "\n============= F0 Settings =============\n"
                                "Note: modifying any of the parameters here will require remodeling the voice db\n";
    section_f0_params.requirement = inicpp::item_requirement::mandatory;
    config_schema.add_section(section_f0_params);

    inicpp::option_schema_params<inicpp::string_ini_t> f0_mode{};
    f0_mode.name = "f0_mode";
    f0_mode.default_value = lessConfigure::get_f0_mode_str(configure.f0_mode);
    f0_mode.comment = "You Can Choose 'DIO' 'Harvest' for f0";
    f0_mode.type = inicpp::option_item::single;
    config_schema.add_option("f0", f0_mode);

    inicpp::option_schema_params<inicpp::signed_ini_t> f0_speed{};
    f0_speed.name = "f0_speed";
    f0_speed.default_value = std::to_string(configure.f0_speed);
    f0_speed.type = inicpp::option_item::single;
    config_schema.add_option("f0", f0_speed);

    inicpp::option_schema_params<inicpp::float_ini_t> f0_dio_floor{};
    f0_dio_floor.name = "f0_dio_floor";
    f0_dio_floor.default_value = std::to_string(configure.f0_dio_floor);
    f0_dio_floor.type = inicpp::option_item::single;
    config_schema.add_option("f0", f0_dio_floor);

    inicpp::option_schema_params<inicpp::float_ini_t> f0_harvest_floor{};
    f0_harvest_floor.name = "f0_harvest_floor";
    f0_harvest_floor.default_value = std::to_string(configure.f0_harvest_floor);
    f0_harvest_floor.type = inicpp::option_item::single;
    config_schema.add_option("f0", f0_harvest_floor);

    inicpp::option_schema_params<inicpp::float_ini_t> f0_cheap_trick_floor{};
    f0_cheap_trick_floor.name = "f0_cheap_trick_floor";
    f0_cheap_trick_floor.default_value = std::to_string(configure.f0_cheap_trick_floor);
    f0_cheap_trick_floor.comment =
            "When fft_size changes from default value,\n"
            "a replaced f0_cheap_trick_floor will be used in CheapTrick\n"
            "for Spectral envelope estimation.\n"
            "The lowest F0 that WORLD can work as expected is determined\n"
            "by the following : f0_cheap_trick_floor = 3.0 * fs / fft_size.\n\n"
            "You can also set your own fft_size";
    f0_cheap_trick_floor.type = inicpp::option_item::single;
    config_schema.add_option("f0", f0_cheap_trick_floor);

    inicpp::option_schema_params<inicpp::float_ini_t> f0_allow_range{};
    f0_allow_range.name = "f0_allow_range";
    f0_allow_range.default_value = std::to_string(configure.f0_allow_range);
    f0_allow_range.type = inicpp::option_item::single;
    config_schema.add_option("f0", f0_allow_range);

    // create project ap section
    inicpp::section_schema_params section_ap_params{};
    section_ap_params.name = "ap";
    section_ap_params.comment = "\n============= AP Settings =============\n";
    section_ap_params.requirement = inicpp::item_requirement::mandatory;
    config_schema.add_section(section_ap_params);

    inicpp::option_schema_params<inicpp::float_ini_t> ap_threshold{};
    ap_threshold.name = "ap_threshold";
    ap_threshold.default_value = std::to_string(configure.ap_threshold);
    ap_threshold.type = inicpp::option_item::single;
    config_schema.add_option("ap", ap_threshold);
}

void ConfigUnit::read_config_file() {
    // read config file
    std::ifstream file(config_file_path);
    if (!file.is_open()) {
        throw file_open_error(config_file_path.string());
    }
    config_file = std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    file.close();
}

void ConfigUnit::save_config_file() {
    // read config file
    std::ofstream file(config_file_path);
    if (!file.is_open()) {
        throw file_open_error(config_file_path.string());
    }
    file << config_file;
    file.close();
}

void ConfigUnit::create_default_config() {
    std::stringstream str;
    str << config_schema;
    config_file = str.str();
}

void ConfigUnit::parse_config() {
    config = inicpp::parser::load(config_file);
    // parse config file config section
    auto config_section = config["config"];
    configure.version = config_section["version"].get<inicpp::string_ini_t>();
    if (configure.version != PROJECT_GIT_HASH) {
        YALL_WARN_ << "Configure file version does not match software version.";
    }
    configure.debug_mode = config_section["debug"].get<inicpp::boolean_ini_t>();

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

lessConfigure ConfigUnit::GetConfig() const {
    return configure;
}

