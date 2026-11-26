/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "LlsmF0Estimator.h"

#include <algorithm>
#include <cmath>
#include <type_traits>
#include <vector>

extern "C" {
#include <dsputils.h>
#include <llsm.h>
}

#include "Config/Config.h"
#include "Vocoder/Common/WorldF0Estimator.h"

namespace lessampler {

    std::string_view LlsmF0Estimator::name() const noexcept {
        return "llsm";
    }

    F0Track LlsmF0Estimator::Estimate(std::span<const double> x, int fs, const Config &configure) const {
        static_assert(std::is_same_v<FP_TYPE, float>);

        F0Track track = WorldF0Estimator(F0EstimatorKind::harvest).Estimate(x, fs, configure);
        if (track.f0.empty()) {
            return track;
        }

        std::vector<FP_TYPE> samples(x.size());
        std::ranges::transform(x, samples.begin(), [](double value) {
            return static_cast<FP_TYPE>(value);
        });
        std::vector<FP_TYPE> f0(track.f0.size());
        std::ranges::transform(track.f0, f0.begin(), [](double value) {
            return static_cast<FP_TYPE>(value);
        });

        llsm_refine_f0(samples.data(), static_cast<int>(samples.size()), static_cast<FP_TYPE>(fs),
                       f0.data(), static_cast<int>(f0.size()),
                       static_cast<FP_TYPE>(track.frame_period / 1000.0));

        for (std::size_t i = 0; i < track.f0.size(); ++i) {
            const double value = static_cast<double>(f0[i]);
            if (std::isfinite(value) && value > 0.0) {
                track.f0[i] = value;
            }
        }
        return track;
    }

}// namespace lessampler
