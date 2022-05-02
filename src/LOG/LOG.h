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

#if (defined(_WIN16) || defined(_WIN32) || defined(_WIN64)) && !defined(__WINDOWS__)
# define __WINDOWS__
#endif

class LOG {
public:
    [[maybe_unused]] static void INFO(const std::string &msg);

    [[maybe_unused]] static void DEBUG(const std::string &msg);

    [[maybe_unused]] static void WARNING(const std::string &msg);

    [[maybe_unused]] static void ERROR(const std::string &msg);
};

#endif //LESSAMPLER_LOG_H
