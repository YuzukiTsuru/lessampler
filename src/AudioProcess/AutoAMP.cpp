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
// Created by gloom on 2022/5/21.
//

#include <cmath>
#include <utility>

#include "AutoAMP.h"
#include "Utils/LOG.h"

AutoAMP::AutoAMP(ShinePara shine, double *x) : shine(std::move(shine)) {
    this->x_length = this->shine.output_samples;
    this->x = x;
    this->x_out = new double[x_length];

    YALL_DEBUG_ << "The X_LENGTH is: " + std::to_string(x_length);

    GetMaxAMP();
    YALL_DEBUG_ << "Get Max AMP is: " + std::to_string(MaxAMP);

    SetDefaultValue();
    YALL_DEBUG_ << "The Default PCM is: " + std::to_string(sample_value);

    YALL_DEBUG_ << "Diminished Consonant Fricative...";
    DiminishedConsonantFricative();

    YALL_DEBUG_ << "Limit maximum amplitude";
    LimitMaximumAmplitude();
}

double *AutoAMP::GetAMP() {
    return x_out;
}

void AutoAMP::GetMaxAMP() {
    for (int i = 0; i < x_length; ++i) {
        if (!std::isnan(x[i])) {
            if (MaxAMP < std::abs(x[i])) {
                MaxAMP = std::abs(x[i]);
            }
        }
    }
    if (MaxAMP == 0.0){
        YALL_WARN_ << "Max AMP is Zero.";
    }
}

void AutoAMP::SetDefaultValue() {
    // TODO: This is a UTAU flag
    sample_value = default_sample_value;
}

void AutoAMP::DiminishedConsonantFricative() {
    for (int i = 0; i < x_length; ++i) {
        // remove blank fricatives
        if (std::isnan(x[i])) {
            x_out[i] = 0.0;
        } else {
            x_out[i] = x[i] * 0.5 * shine.volumes / MaxAMP;
        }
    }
}

void AutoAMP::LimitMaximumAmplitude() {
    for (int i = 0; i < x_length; ++i) {
        if (x_out[i] > MaxValue) {
            x_out[i] = MaxValue;
        } else if (x_out[i] < MinValue) {
            x_out[i] = MinValue;
        }
    }
}
