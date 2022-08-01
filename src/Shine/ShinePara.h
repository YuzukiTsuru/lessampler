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
// Created by gloom on 2022/5/30.
//

#ifndef LESSAMPLER_SHINEPARA_H
#define LESSAMPLER_SHINEPARA_H

#include <iostream>
#include <vector>

class ShinePara {
public:
    // Basic Transformation Parameters
    std::string input_file_name = {};
    std::string output_file_name = {};
    int time_percent = 0;
    double velocity = 0.0;
    double offset = 0.0;
    double required_length = 0.0;
    int required_frame = 0;
    double first_half_fixed_part = 0.0;
    double last_unused_part = 0.0;
    double volumes = 0;
    int modulation = 0;
    double wave_length = 0.0;
    double pre_cross_length = 0.0;
    double base_length = 0.0;
    double cross_length = 0.0;
    double stretch_length = 0.0;
    int output_samples = 0;
    double scale_num = 0.0;
    int tempo_num = 0;

public:
    // Pitch sections
    std::vector<int> pitch_bend = {};
    int pitch_length = 0;
    int pitch_step = 256;

public:
    // Options
    bool is_custom_pitch = false;
    bool is_gender = false;
    bool is_breath = false;
    bool is_opening = false;
public:
    // option data
    double gender_value = 0.0;
    double breath_value = 0.0;
    double opening = 0.0;
};

#endif //LESSAMPLER_SHINEPARA_H
