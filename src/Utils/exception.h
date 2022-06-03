/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

//
// Created by gloom on 2022/5/2.
//

#ifndef LESSAMPLER_EXCEPTION_H
#define LESSAMPLER_EXCEPTION_H

#include <stdexcept>
#include <string>

class file_open_error : public std::runtime_error {
public:
    explicit file_open_error(const std::string &what) : std::runtime_error("Fail to open file: " + what + ".") {};
};

class header_check_error : public std::runtime_error {
public:
    header_check_error(const std::string &what, const std::string &expect) : std::runtime_error(
            "Header: " + what + " is not same as " + expect + ".") {};
};

class parameter_error : public std::runtime_error {
public:
    explicit parameter_error(const std::string &what) : std::runtime_error("Parameter Error: " + what + ".") {};
};

class type_error : public std::runtime_error {
public:
    explicit type_error(const std::string &what) : std::runtime_error("Type Error: " + what + ".") {};
};

class audio_file_error : public std::runtime_error {
public:
    explicit audio_file_error(const std::string &what) : std::runtime_error("Audio File Error: " + what + ".") {};
};

#endif //LESSAMPLER_EXCEPTION_H
