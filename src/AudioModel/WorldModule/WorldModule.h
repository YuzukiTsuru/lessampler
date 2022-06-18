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
// Created by gloom on 2022/5/2.
//

#ifndef LESSAMPLER_WORLDMODULE_H
#define LESSAMPLER_WORLDMODULE_H

#include <iostream>
#include <vector>

#include "WorldPara.h"
#include "ConfigUnit/lessConfigure.h"


class WorldModule {
public:
    WorldModule(std::vector<double> x_vector, int fs, const lessConfigure &configure);

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

    lessConfigure configure;
};

#endif //LESSAMPLER_WORLDMODULE_H
