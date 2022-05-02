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

#include <iostream>

#include "UTAUException.h"
#include "UTAUParameterPaser.h"

#include "libUTAU.h"

libUTAU::libUTAU(int argc, char **argv) {
    if (argc < 3) {
        throw parameter_error("too sort");
    }
    UTAUParameterPaser paser(argc, argv);
    utauPara = paser.get_para();
}

void libUTAU::printUTAUPara() const {
    std::cout << "Program Name:           " << utauPara.localName << std::endl;
    std::cout << "In File Name:           " << utauPara.inputFileName << std::endl;
    std::cout << "Out File Name:          " << utauPara.outputFileName << std::endl;
    std::cout << "Scale Name:             " << utauPara.scaleName << std::endl;
    std::cout << "Scale Number:           " << utauPara.scaleNum << std::endl;
    std::cout << "Time Percent:           " << utauPara.timePercent << std::endl;
    std::cout << "Velocity:               " << utauPara.velocity << std::endl;
    std::cout << "Flags:                  " << utauPara.flags << std::endl;
    std::cout << "Offset:                 " << utauPara.offset << std::endl;
    std::cout << "Host Required Length:   " << utauPara.requiredLength << std::endl;
    std::cout << "First Half Fixed Part:  " << utauPara.firstHalfFixedPart << std::endl;
    std::cout << "Last Unused Part:       " << utauPara.lastUnusedPart << std::endl;
    std::cout << "Volumes:                " << utauPara.volumes << std::endl;
    std::cout << "Tempo:                  " << utauPara.tempo << std::endl;
    std::cout << "Tempo Number:           " << utauPara.tempoNum << std::endl;
    std::cout << "Pitch String:           " << utauPara.pitch << std::endl;
}

[[maybe_unused]] UTAUPara libUTAU::getUTAUPara() const {
    return utauPara;
}
