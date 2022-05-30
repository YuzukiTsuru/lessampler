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

void libUTAU::CheckPara(lessAudioModel audioModel) {
    utauPara.wave_length = static_cast<double>(audioModel.x_length) / static_cast<double>(audioModel.fs) * 1000;
    // Check last Unused Part of audio less 0
    if (utauPara.lastUnusedPart < 0) {
        utauPara.lastUnusedPart = utauPara.wave_length - utauPara.offset + utauPara.lastUnusedPart;
        // Check again: last Unused Part of audio less 0
        if (utauPara.lastUnusedPart < 0)
            utauPara.lastUnusedPart = 0;
    }

    // Check the required time
    if (utauPara.offset + utauPara.lastUnusedPart >= utauPara.wave_length)
        throw parameter_error("The audio offset and whitespace are greater than the required audio length");

    // Check the fixed length para
    if (utauPara.offset + utauPara.lastUnusedPart + utauPara.firstHalfFixedPart >= utauPara.wave_length)
        utauPara.firstHalfFixedPart = utauPara.wave_length - utauPara.offset + utauPara.lastUnusedPart;

    utauPara.pre_cross_length = utauPara.wave_length - utauPara.offset - utauPara.firstHalfFixedPart - utauPara.lastUnusedPart;
    utauPara.base_length = utauPara.firstHalfFixedPart / utauPara.velocity;
    utauPara.cross_length = utauPara.requiredLength - utauPara.base_length;

    if (utauPara.pre_cross_length <= 0 && utauPara.cross_length > 0)
        throw parameter_error("The input audio length does not match the required cross-transformation length, and its reference value is less than 0");

    utauPara.stretch_length = utauPara.pre_cross_length / utauPara.cross_length;
    // Prevent telescopic cutting beyond the required length
    if (utauPara.stretch_length > 1.0)
        utauPara.stretch_length = 1.0;

    utauPara.output_samples = static_cast<int>(utauPara.requiredLength * 0.001 * audioModel.fs) + 1;
}
