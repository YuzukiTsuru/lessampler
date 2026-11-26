/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_LLSMF0ESTIMATOR_H
#define LESSAMPLER_LLSMF0ESTIMATOR_H

#include "Vocoder/Common/F0EstimatorTypes.h"

namespace lessampler {

    /*
     * LLSM F0: a Harvest initial track followed by libllsm2's harmonic
     * instantaneous-frequency refinement. libllsm2 has no detector of its own.
     */
    class LlsmF0Estimator final : public F0Estimator {
    public:
        [[nodiscard]] std::string_view name() const noexcept override;

        [[nodiscard]] F0Track Estimate(std::span<const double> x, int fs, const Config &configure) const override;
    };

}// namespace lessampler

#endif//LESSAMPLER_LLSMF0ESTIMATOR_H
