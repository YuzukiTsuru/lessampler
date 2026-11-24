/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_CONFIGPARSER_H
#define LESSAMPLER_CONFIGPARSER_H

#include <filesystem>
#include <string>

#include <toml++/toml.hpp>

#include "Config.h"

/*
 * Apply a parsed TOML table to an existing configuration. Missing keys keep
 * their current value; invalid values raise parameter_error.
 */
void apply_config(const toml::table &table, Config &configure);

/*
 * Default lessampler.fex content, including comments.
 */
[[nodiscard]] std::string default_config_toml();

/*
 * Serialize the current configuration as TOML. Used when migrating the old
 * INI format.
 */
void write_config_toml(const std::filesystem::path &path, const Config &configure);

/*
 * Parse the pre-TOML lessampler.fex format. Returns false when the file cannot
 * be read or contains a value that cannot be converted.
 */
bool parse_legacy_ini_file(const std::filesystem::path &path, Config &configure);

#endif//LESSAMPLER_CONFIGPARSER_H
