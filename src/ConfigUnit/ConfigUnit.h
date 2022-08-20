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

#ifndef LESSAMPLER_CONFIGUNIT_H
#define LESSAMPLER_CONFIGUNIT_H

#include <iostream>
#include <filesystem>

#include <inicpp/inicpp.h>

#include "lessConfigure.h"
#include "ConfigPathHelper.h"
#include "Utils/LOG.h"

class ConfigUnit {
public:
    explicit ConfigUnit(const std::filesystem::path &exec_path);

    void SetConfig(const std::filesystem::path &exec_path);

    ~ConfigUnit();

    [[nodiscard]] lessConfigure GetConfig() const;

private:
    ConfigPathHelper path_helper;
    std::filesystem::path config_file_path;
    std::string config_file;
    inicpp::config config;
    inicpp::schema config_schema;
    lessConfigure configure;

private:
    // init config
    void init_config();

    // make configure schema
    void make_schema();

    // read configure file
    void read_config_file();

    // set default configure file
    void save_config_file();

    // create defalut config from lessConfigure class default value
    void create_default_config();

    // configure parser
    void parse_config();
};


#endif //LESSAMPLER_CONFIGUNIT_H
