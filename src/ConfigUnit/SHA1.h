/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
    ============
    SHA-1 in C++
    ============
    Original C Code
        -- Steve Reid <steve@edmweb.com>
    Small changes to fit into bglibs
        -- Bruce Guenter <bruce@untroubled.org>
    Translation to simpler C++ Code
        -- Volker Diels-Grabsch <v@njh.eu>
    Safety fixes
        -- Eugene Hopkinson <slowriot at voxelstorm dot com>
    Header-only library
        -- Zlatko Michailov <zlatko@michailov.org>
    lessampler inline
        -- YuzukiTsuru <gloomyghost@gloomyghost.com>
*/

//
// Created by gloom on 2022/7/15.
//

#ifndef LESSAMPLER_SHA1_H
#define LESSAMPLER_SHA1_H

#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

class SHA1 {
public:
    SHA1();

    void update(const std::string &s);

    void update(std::istream &is);

    std::string final();

    [[maybe_unused]] static std::string from_file(const std::string &filename);

private:
    uint32_t digest[5]{};
    std::string buffer;
    uint64_t transforms{};
};

#endif //LESSAMPLER_SHA1_H
