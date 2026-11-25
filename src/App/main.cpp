/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include <exception>

#include "App/Application.h"
#include "Core/Log.h"

using namespace lessampler;

int main(int argc, char *argv[]) {
    try {
        Application application(argc, argv);
        return application.run();
    } catch (const std::exception &error) {
        LOG_ERROR << error.what();
        return -1;
    }
    return 0;
}
