/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "VoicebankConfig.h"

#include <format>
#include <utility>

#include <toml++/toml.hpp>

#include "ConfigParser.h"
#include "Core/Constants.h"
#include "Core/Exception.h"
#include "Core/Log.h"

namespace lessampler {

    namespace {

        std::filesystem::path directory_of(const std::filesystem::path &path) {
            if (std::filesystem::is_directory(path)) {
                return path;
            }
            const auto parent = path.parent_path();
            return parent.empty() ? std::filesystem::path(".") : parent;
        }

    }// namespace

    VoicebankConfig::VoicebankConfig(const std::filesystem::path &voice_file_path, Config _configure)
        : configure(std::move(_configure)), voice_path(directory_of(voice_file_path)) {
        SetVoiceConfig();
    }

    VoicebankConfig::VoicebankConfig(Config _configure) : configure(std::move(_configure)) {}

    void VoicebankConfig::SetVoiceConfig() {
        if (voice_path.empty()) {
            return;
        }

        voice_config_path = voice_path / std::string(lessampler::constants::voicebank_config_file_name);
        if (std::filesystem::exists(voice_config_path)) {
            parse_config();
        }
    }

    void VoicebankConfig::parse_config() {
        toml::table table;
        try {
            table = toml::parse_file(voice_config_path.string());
        } catch (const toml::parse_error &error) {
            throw parameter_error(std::format("Cannot parse voicebank config {}: {}",
                                              voice_config_path.string(), error.description()));
        }

        apply_config(table, configure);
        YALL_DEBUG_ << std::format("Loaded voicebank config: {}", voice_config_path.string());
    }

    Config VoicebankConfig::GetConfig() const {
        return configure;
    }

}// namespace lessampler
