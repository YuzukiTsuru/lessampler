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

#ifndef LESSAMPLER_LIBUTAU_H
#define LESSAMPLER_LIBUTAU_H

#include <iostream>

class UTAUPara {
public:
    std::string localName = {};
    std::string inputFileName = {};
    std::string outputFileName = {};
    std::string scaleName = {};
    double scaleNum = 0.0;
    int timePercent = 0;
    double velocity = 0.0;
    std::string flags = {};
    double offset = 0.0;
    double requiredLength = 0.0;
    double firstHalfFixedPart = 0.0;
    double lastUnusedPart = 0.0;
    double volumes = 0;
    int modulation = 0;
    std::string tempo = {};
    int tempoNum = 0;
    std::string pitch = {};
};

class libUTAU {
public:
    libUTAU(int argc, char *argv[]);

    void printUTAUPara() const;

    [[maybe_unused]] [[nodiscard]] UTAUPara getUTAUPara() const;

private:
    UTAUPara utauPara;
};

#endif //LESSAMPLER_LIBUTAU_H
