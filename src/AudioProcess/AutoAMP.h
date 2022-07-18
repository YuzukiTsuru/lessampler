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
// Created by gloom on 2022/5/21.
//

#ifndef LESSAMPLER_AUTOAMP_H
#define LESSAMPLER_AUTOAMP_H

#include "AudioModel/lessAudioModel.h"
#include "Shine/ShinePara.h"

class AutoAMP {
public:
    // Auto AMP with Shine para
    AutoAMP(ShinePara shine, double *x);

    // Auto AMP only for WAV
    AutoAMP(double *x);

    double *GetAMP();

private:
    ShinePara shine;

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
