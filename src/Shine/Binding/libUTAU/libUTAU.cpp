/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
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
    UTAUParameterParser paser(argc, argv);
    utauPara = paser.get_para();
}

void libUTAU::PrintUTAUPara() const {
    std::stringstream ss;
    ss << "\nProgram Name:           " << utauPara.local_name;
    ss << "\nIn File Name:           " << utauPara.input_file_name;
    ss << "\nOut File Name:          " << utauPara.output_file_name;
    ss << "\nScale Name:             " << utauPara.scale_name;
    ss << "\nScale Number:           " << utauPara.scale_num;
    ss << "\nTime Percent:           " << utauPara.time_percent;
    ss << "\nVelocity:               " << utauPara.velocity;
    ss << "\nFlags:                  " << utauPara.flags;
    ss << "\nOffset:                 " << utauPara.offset;
    ss << "\nHost Required Length:   " << utauPara.required_length;
    ss << "\nFirst Half Fixed Part:  " << utauPara.first_half_fixed_part;
    ss << "\nLast Unused Part:       " << utauPara.last_unused_part;
    ss << "\nVolumes:                " << utauPara.volumes;
    ss << "\nTempo:                  " << utauPara.tempo;
    ss << "\nTempo Number:           " << utauPara.tempo_num;
    ss << "\nPitch String:           " << utauPara.pitch;
    YALL_DEBUG_ << ss.str();
}

[[maybe_unused]] UTAUPara libUTAU::GetUTAUPara() const {
    return utauPara;
}

UTAUFlags libUTAU::GetUTAUFlags() const {
    return utauFlags;
}

void libUTAU::CheckPara(const lessAudioModel& audioModel) {
    utauPara.wave_length = static_cast<double>(audioModel.x.size()) / static_cast<double>(audioModel.fs) * 1000;
    // Check last Unused Part of audio less 0
    if (utauPara.last_unused_part < 0) {
        utauPara.last_unused_part = utauPara.wave_length - utauPara.offset + utauPara.last_unused_part;
        // Check again: last Unused Part of audio less 0
        if (utauPara.last_unused_part < 0)
            utauPara.last_unused_part = 0;
    }

    // Check the required time
    if (utauPara.offset + utauPara.last_unused_part >= utauPara.wave_length)
        throw parameter_error("The audio offset and whitespace are greater than the required audio length");

    // Check the fixed length para
    if (utauPara.offset + utauPara.last_unused_part + utauPara.first_half_fixed_part >= utauPara.wave_length)
        utauPara.first_half_fixed_part = utauPara.wave_length - utauPara.offset + utauPara.last_unused_part;

    utauPara.pre_cross_length = utauPara.wave_length - utauPara.offset - utauPara.first_half_fixed_part - utauPara.last_unused_part;
    utauPara.base_length = utauPara.first_half_fixed_part / utauPara.velocity;
    utauPara.cross_length = utauPara.required_length - utauPara.base_length;

    if (utauPara.pre_cross_length <= 0 && utauPara.cross_length > 0)
        throw parameter_error("The input audio length does not match the required cross-transformation length, and its reference value is less than 0");

    // get stretched length
    utauPara.stretch_length = utauPara.pre_cross_length / utauPara.cross_length;

    // Prevent telescopic cutting beyond the required length
    if (utauPara.stretch_length > 1.0)
        utauPara.stretch_length = 1.0;

    utauPara.output_samples = static_cast<int>(utauPara.required_length * 0.001 * audioModel.fs) + 1;
}

