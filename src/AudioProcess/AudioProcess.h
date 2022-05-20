
// Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

//
// Created by gloom on 2022/5/7.
//

#ifndef LESSAMPLER_AUDIOPROCESS_H
#define LESSAMPLER_AUDIOPROCESS_H

#include "AudioModel/lessAudioModel.h"
#include "libUTAU/libUTAU.h"

class AduioProcess {
public:
    AduioProcess(lessAudioModel audioModel, UTAUPara utauPara, UTAUFlags flags);

    TransAudioModel GetTransAudioModel();

private:
    lessAudioModel audioModel{};
    TransAudioModel transAudioModel{};
    UTAUPara utauPara{};
    UTAUFlags flags;

    double freq_avg = 0;
    int pitch_length = 0;
    int pitch_step = 256;

private:
    void AllocateMemory();

    void DecodePitchBend();

    void GetAvgFreq();

    void PitchShift();
};


#endif //LESSAMPLER_AUDIOPROCESS_H
