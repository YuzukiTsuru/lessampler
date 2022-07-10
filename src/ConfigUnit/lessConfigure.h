/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

//
// Created by gloom on 2022/5/4.
//

#ifndef LESSAMPLER_LESSCONFIGURE_H
#define LESSAMPLER_LESSCONFIGURE_H

#include <iostream>

class lessConfigure {
public:
    enum class F0_MODE {
        F0_MODE_DIO,
        F0_MODE_HARVEST,
        F0_MODE_UNKNOWN,
    };

    std::string version;
    bool debug_mode = false;
    double audio_model_frame_period = (1000.0 * 256 / 44100);
    bool custom_fft_size = false;
    int fft_size = 1024;
    F0_MODE f0_mode = F0_MODE::F0_MODE_DIO;
    int f0_speed = 1;
    double f0_dio_floor = 40.0;
    double f0_harvest_floor = 40.0;
    double f0_cheap_trick_floor = 71.0;
    double f0_allow_range = 0.1;
    double ap_threshold = 0.85;
};

#endif //LESSAMPLER_LESSCONFIGURE_H
