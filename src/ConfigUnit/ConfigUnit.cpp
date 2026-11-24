/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ConfigUnit.h"

#include <filesystem>
#include <format>
#include <fstream>
#include <system_error>

#include <toml++/toml.hpp>

#include "ConfigParser.h"
#include "Constants.h"
#include "Utils/LOG.h"
#include "Utils/exception.h"
#include "lessconfig.h"

ConfigUnit::ConfigUnit(const std::filesystem::path &exec_path) {
    SetConfig(exec_path);
}

ConfigUnit::~ConfigUnit() = default;

void ConfigUnit::SetConfig(const std::filesystem::path &exec_path) {
    config_file_path = exec_path / std::string(lessampler_constants::config_file_name);

    if (!std::filesystem::exists(config_file_path)) {
        YALL_DEBUG_ << std::format("Config file not found, creating defaults: {}", config_file_path.string());
        std::ofstream out(config_file_path);
        if (!out) {
            throw file_open_error(config_file_path.string());
        }
        out << default_config_toml();
        if (!out) {
            throw file_open_error(config_file_path.string());
        }
    }

    load_or_create();
}

void ConfigUnit::load_or_create() {
    toml::table table;
    try {
        table = toml::parse_file(config_file_path.string());
    } catch (const toml::parse_error &error) {
        YALL_WARN_ << std::format("Cannot parse {} as TOML: {}. Trying the legacy INI format.",
                                  config_file_path.string(), error.description());
        migrate_legacy_ini();
        try {
            table = toml::parse_file(config_file_path.string());
        } catch (const toml::parse_error &second_error) {
            throw parameter_error(std::format("Cannot parse config file {}: {}",
                                              config_file_path.string(), second_error.description()));
        }
    }

    apply_config(table, configure);

    if (!configure.version.empty() && configure.version != PROJECT_GIT_HASH) {
        YALL_WARN_ << "Configure file version does not match software version.";
    }
}

void ConfigUnit::migrate_legacy_ini() {
    lessConfigure migrated;
    migrated.version = PROJECT_GIT_HASH;

    if (!parse_legacy_ini_file(config_file_path, migrated)) {
        throw parameter_error(std::format("Cannot migrate legacy config file: {}", config_file_path.string()));
    }

    const std::filesystem::path backup = config_file_path.string() + ".bak";
    std::error_code error;
    std::filesystem::copy_file(config_file_path, backup,
                               std::filesystem::copy_options::overwrite_existing, error);
    if (error) {
        throw file_open_error(backup.string());
    }

    write_config_toml(config_file_path, migrated);
    YALL_WARN_ << std::format("Migrated legacy INI config to TOML: {}", config_file_path.string());
}

lessConfigure ConfigUnit::GetConfig() const {
    return configure;
}
