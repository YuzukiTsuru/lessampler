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
#include "libUTAU/FlagsDecoder.h"
#include "AudioModel/lessAudioModel.h"

class UTAUFlags {
public:
    bool isLoopStratch = false;
};

// AudioProcess Concept
//  offset    fixed   pre_cross  blank
//|--------|--------|---------|---------| Original Signal
//         |        |          |
//         |   l1   |    l2     |
//         |--------|------------|        Output Signal
// l1  = fixed / velocity                        -> base_length
// l2  = pre_cross / stretch                     -> cross_length
// l1 + l2  = required length ＝ argv[7]         -> required length

class UTAUPara {
public:
    // Basic Transformation Parameters
    std::string localName = {};
    std::string inputFileName = {};
    std::string outputFileName = {};
    std::string scaleName = {};
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
    std::string pitch = {};

public:
    // Extended transformation parameters
    double wave_length = 0.0;
    double pre_cross_length = 0.0;
    double base_length = 0.0;
    double cross_length = 0.0;
    double stretch_length = 0.0;
    int output_samples = 0;
    int *pitch_bend = nullptr;
    double scaleNum = 0.0;
    int tempoNum = 0;
    bool isCustomPitch = false;
};

class libUTAU {
public:
    libUTAU(int argc, char *argv[]);

    void printUTAUPara() const;

    void CheckPara(lessAudioModel audioModel);

    [[maybe_unused]] [[nodiscard]] UTAUPara getUTAUPara() const;

private:
    UTAUPara utauPara;
};

#endif //LESSAMPLER_LIBUTAU_H
