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

#ifndef LESSAMPLER_UTAUEXCEPTION_H
#define LESSAMPLER_UTAUEXCEPTION_H

#include <stdexcept>
#include <string>

class parameter_error : public std::runtime_error {
public:
    explicit parameter_error(const std::string &what) : std::runtime_error("Parameter error: " + what + ".") {};
};

class type_error : public std::runtime_error {
public:
    explicit type_error(const std::string &what) : std::runtime_error("Type error: " + what + ".") {};
};

#endif //LESSAMPLER_UTAUEXCEPTION_H
