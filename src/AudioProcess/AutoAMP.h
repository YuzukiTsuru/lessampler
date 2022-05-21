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
// Created by gloom on 2022/5/21.
//

#ifndef LESSAMPLER_AUTOAMP_H
#define LESSAMPLER_AUTOAMP_H

#include "AudioModel/lessAudioModel.h"
#include "libUTAU/libUTAU.h"

class AutoAMP {
public:
    AutoAMP(UTAUPara utauPara, UTAUFlags utauFlags, double *x);

    double *GetAMP();

private:
    UTAUPara utauPara{};
    UTAUFlags utauFlags{};

    int x_length = 0;
    double *x = nullptr;
    double *x_out = nullptr;

    const double default_sample_value = 0.86;
    const double MaxValue = 1.0;
    const double MinValue = -1.0;

    double sample_value = 0.0;
    double MaxAMP = 0.0;

private:
    void GetMaxAMP();

    void SetDefaultValue();

    void DiminishedConsonantFricative();

    void LimitMaximumAmplitude();
};


#endif //LESSAMPLER_AUTOAMP_H
