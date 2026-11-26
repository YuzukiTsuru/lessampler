/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_WORLDF0ESTIMATOR_H
#define LESSAMPLER_WORLDF0ESTIMATOR_H

#include "Config/F0EstimatorKind.h"
#include "Vocoder/Common/F0EstimatorTypes.h"

namespace lessampler {

    /*
     * WORLD front end: Harvest, or DIO followed by StoneMask. The output is
     * byte-compatible with the historical shared EstimateF0 function.
     */
    class WorldF0Estimator final : public F0Estimator {
    public:
        explicit WorldF0Estimator(F0EstimatorKind kind);

        [[nodiscard]] std::string_view name() const noexcept override;

        [[nodiscard]] F0Track Estimate(std::span<const double> x, int fs, const Config &configure) const override;

    private:
        F0EstimatorKind kind_;
    };

}// namespace lessampler

#endif//LESSAMPLER_WORLDF0ESTIMATOR_H
