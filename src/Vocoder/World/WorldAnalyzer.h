/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_WORLDMODULE_H
#define LESSAMPLER_WORLDMODULE_H

#include <span>

#include "Config/Config.h"
#include "WorldPara.h"

namespace lessampler {

    class WorldAnalyzer {
    public:
        WorldAnalyzer(std::span<const double> x, int fs, const Config &config, int max_workers = 0);

        [[nodiscard]] WorldPara TakeModule();

    private:
        void SpectralEnvelopeEstimation();

        void AperiodicityEstimation();

    protected:
        const double *x = nullptr;
        int x_length = 0;

        WorldPara worldPara;

        Config configure;
        int max_workers_ = 0;
    };

}// namespace lessampler

#endif//LESSAMPLER_WORLDMODULE_H
