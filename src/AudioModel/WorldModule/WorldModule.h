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
 *   along with libllsm. If not, see <http://www.gnu.org/licenses/>.
 */

//
// Created by gloom on 2022/5/2.
//

#ifndef LESSAMPLER_WORLDMODULE_H
#define LESSAMPLER_WORLDMODULE_H

#include "WorldPara.h"


class WorldModule {
public:
    enum F0_MDOE {
        F0_MDOE_UNKNOWN = 0,
        F0_MDOE_DIO = 1,
        F0_MDOE_HARVEST = 2
    };

    WorldModule(double *x, int x_length, int fs, double frame_period, F0_MDOE f0_mode = F0_MDOE_DIO);

    ~WorldModule();

    WorldPara GetModule();

private:
    void F0EstimationDio();

    void F0EstimationHarvest();

    void SpectralEnvelopeEstimation();

    void AperiodicityEstimation();

protected:
    double *x = nullptr;
    int x_length = 0;

    WorldPara worldPara;
};

#endif //LESSAMPLER_WORLDMODULE_H
