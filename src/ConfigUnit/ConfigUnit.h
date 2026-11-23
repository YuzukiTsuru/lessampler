/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_CONFIGUNIT_H
#define LESSAMPLER_CONFIGUNIT_H

#include <filesystem>

#include "lessConfigure.h"

class ConfigUnit {
public:
    explicit ConfigUnit(const std::filesystem::path &exec_path);

    void SetConfig(const std::filesystem::path &exec_path);

    ~ConfigUnit();

    [[nodiscard]] lessConfigure GetConfig() const;

private:
    std::filesystem::path config_file_path;
    lessConfigure configure;

private:
    void load_or_create();

    void migrate_legacy_ini();
};

#endif//LESSAMPLER_CONFIGUNIT_H
