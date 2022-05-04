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

#ifndef LESSAMPLER_LOG_H
#define LESSAMPLER_LOG_H

#include <iostream>
#include "config.h"
#include "exception.h"
#include "ColorCout.hpp"

class LOG {
public:
    template<typename T>
    [[maybe_unused]] static void INFO(T msg) {
        std::cout << cc::cyan << "[" << PROJECT_NAME << " INFO] " << msg << cc::reset << std::endl;
    };

    template<typename T>
    [[maybe_unused]] static void DEBUG(T msg) {
#if DEBUG_MODE
        std::cout << cc::white << "[" << PROJECT_NAME << " DEBUG] " << cc::reset << msg << std::endl;
#endif
    };

    template<typename T>
    [[maybe_unused]] static void WARNING(T msg) {
        std::cout << cc::yellow << "[" << PROJECT_NAME << " WARNING] " << msg << cc::reset << std::endl;
    };

    template<typename T>
    [[maybe_unused]] static void FATAL(T msg) {
        std::cout << cc::red << "[" << PROJECT_NAME << " ERROR] " << msg << cc::reset << std::endl;
        throw std::runtime_error(msg);
    };
};

#endif //LESSAMPLER_LOG_H
