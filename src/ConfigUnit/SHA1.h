/*
    sha1.hpp - source code of

    ============
    SHA-1 in C++
    ============

    100% Public Domain.

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
