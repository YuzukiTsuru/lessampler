/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * Application is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with Application. If not, see <http://www.gnu.org/licenses/>.
 */

//
// Created by gloom on 2022/5/2.
//

#ifndef LESSAMPLER_LESSAMPLER_H
#define LESSAMPLER_LESSAMPLER_H

#include <filesystem>
#include <iostream>

#include "Config/ConfigLoader.h"
#include "Core/Log.h"

class Application {
public:
    Application(int argc, char **argv);

    void run();

private:
    int argc;
    char **argv;
    std::filesystem::path exec_path;
    std::filesystem::path in_file_path;
    Config configure;

private:
    const int default_fs = 44100;

private:
    [[nodiscard]] bool ParseArgs();

    static void ShowUsage();

private:
    static void show_logo();
};


#endif//LESSAMPLER_LESSAMPLER_H
