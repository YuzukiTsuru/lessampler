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

#include "Utils/LOG.h"
#include "Utils/exception.h"
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
    std::stringstream ss;
    ss << "\nProgram Name:           " << utauPara.localName;
    ss << "\nIn File Name:           " << utauPara.inputFileName;
    ss << "\nOut File Name:          " << utauPara.outputFileName;
    ss << "\nScale Name:             " << utauPara.scaleName;
    ss << "\nScale Number:           " << utauPara.scaleNum;
    ss << "\nTime Percent:           " << utauPara.timePercent;
    ss << "\nVelocity:               " << utauPara.velocity;
    ss << "\nFlags:                  " << utauPara.flags;
    ss << "\nOffset:                 " << utauPara.offset;
    ss << "\nHost Required Length:   " << utauPara.requiredLength;
    ss << "\nFirst Half Fixed Part:  " << utauPara.firstHalfFixedPart;
    ss << "\nLast Unused Part:       " << utauPara.lastUnusedPart;
    ss << "\nVolumes:                " << utauPara.volumes;
    ss << "\nTempo:                  " << utauPara.tempo;
    ss << "\nTempo Number:           " << utauPara.tempoNum;
    ss << "\nPitch String:           " << utauPara.pitch;
    YALL_DEBUG_ << ss.str();
}

[[maybe_unused]] UTAUPara libUTAU::getUTAUPara() const {
    return utauPara;
}
