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
// Created by gloom on 2022/7/14.
//

#include <iostream>
#include <string>
#include "SHA1.h"
#include "lessConfigure.h"

std::string lessConfigure::get_version() {
    make_ver();
    return version_data;
}

void lessConfigure::make_ver() {
    // Generate Checksum code
    version_data = std::to_string(audio_model_frame_period)
                   + std::to_string(fft_size)
                   + std::to_string(model_amp)
                   + std::to_string(f0_speed)
                   + std::to_string(f0_dio_floor)
                   + std::to_string(f0_harvest_floor)
                   + std::to_string(f0_cheap_trick_floor)
                   + std::to_string(f0_allow_range)
                   + std::to_string(ap_threshold)
                   + get_f0_mode_str(f0_mode);

    // Using SHA-1 for checksum feature
    SHA1 checksum;
    checksum.update(version_data);
    version_data = checksum.final();
}

std::string lessConfigure::get_f0_mode_str(lessConfigure::F0_MODE f0_mode) {
    return [&]() -> std::string {
        if (f0_mode == lessConfigure::F0_MODE::F0_MODE_DIO)
            return "DIO";
        if (f0_mode == lessConfigure::F0_MODE::F0_MODE_HARVEST)
            return "HARVEST";
        else
            return "UNKNOWN";
    }();
}

