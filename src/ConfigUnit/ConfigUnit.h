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

#ifndef LESSAMPLER_CONFIGUNIT_H
#define LESSAMPLER_CONFIGUNIT_H

#include <iostream>
#include <inicpp/inicpp.h>
#include "lessConfigure.h"
#include "LOG.h"

class ConfigUnit {
public:
    explicit ConfigUnit(const std::string &config_file_path);

    lessConfigure get_config();

private:
    std::filesystem::path config_file_path;
    std::string config_file;
    inicpp::config config;
    lessConfigure configure;

    void read_config_file();

    void save_config_file();

    void create_default_config();

    void parse_config();

    void print_config();
};


#endif //LESSAMPLER_CONFIGUNIT_H
