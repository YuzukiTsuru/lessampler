/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_LIBUTAU_H
#define LESSAMPLER_LIBUTAU_H

#include <iostream>
#include "FlagsDecoder.h"
#include "AudioModel/lessAudioModel.h"

class UTAUFlags {
public:
    bool isLoopStratch = false;
};

// AudioProcess Concept
//  offset    fixed  pre_cross   blank
//|--------|--------|---------|---------| Original Signal
//         |        |          |
//         |   l1   |    l2     |
//         |--------|------------|        Output Signal
// l1  = fixed / velocity                        -> base_length
// l2  = pre_cross / stretch                     -> cross_length
// l1 + l2 = required_length = argv[7]         -> required_length

class UTAUPara {
public:
    // Basic Transformation Parameters
    std::string local_name = {};
    std::string input_file_name = {};
    std::string output_file_name = {};
    std::string scale_name = {};
    int time_percent = 0;
    double velocity = 0.0;
    std::string flags = {};
    double offset = 0.0;
    double required_length = 0.0;
    double first_half_fixed_part = 0.0;
    double last_unused_part = 0.0;
    double volumes = 0;
    int modulation = 0;
    std::string tempo = {};
    std::string pitch = {};

public:
    // Extended transformation parameters
    double wave_length = 0.0;
    double pre_cross_length = 0.0;
    double base_length = 0.0;
    double cross_length = 0.0;
    double stretch_length = 0.0;
    int output_samples = 0;
    double scale_num = 0.0;
    int tempo_num = 0;
    bool is_custom_pitch = false;
};

class libUTAU {
public:
    libUTAU(int argc, char *argv[]);

    void PrintUTAUPara() const;

    void CheckPara(lessAudioModel audioModel);

    [[maybe_unused]] [[nodiscard]] UTAUPara GetUTAUPara() const;

    [[nodiscard]] UTAUFlags GetUTAUFlags() const;

private:
    UTAUPara utauPara;
    UTAUFlags utauFlags;
};

#endif //LESSAMPLER_LIBUTAU_H
