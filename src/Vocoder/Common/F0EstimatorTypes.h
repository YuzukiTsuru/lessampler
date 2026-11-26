/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_F0ESTIMATORTYPES_H
#define LESSAMPLER_F0ESTIMATORTYPES_H

#include <span>
#include <string_view>
#include <vector>

namespace lessampler {

    class Config;

    struct F0Track {
        double frame_period = 5.0;
        std::vector<double> f0;
        std::vector<double> time_axis;
    };

    /*
     * Backend-neutral F0 estimator. Implementations are created by the
     * factory in F0Estimator.h; WORLD, LLSM and neural backends never depend
     * on a concrete estimator type.
     */
    class F0Estimator {
    public:
        virtual ~F0Estimator() = default;

        [[nodiscard]] virtual std::string_view name() const noexcept = 0;

        [[nodiscard]] virtual F0Track Estimate(std::span<const double> x, int fs,
                                               const Config &configure) const = 0;
    };

}// namespace lessampler

#endif//LESSAMPLER_F0ESTIMATORTYPES_H
