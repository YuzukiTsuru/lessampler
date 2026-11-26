/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "RmvpeDecode.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include "Config/Config.h"

namespace lessampler {

    namespace {

        constexpr double rmvpe_frame_period_ms = 10.0;
        constexpr double rmvpe_cents_offset = 1997.3794084376191;

    }// namespace

    F0Track DecodeRmvpeOutput(const RmvpeOutput &output, const Config &configure) {
        F0Track raw;
        raw.frame_period = rmvpe_frame_period_ms;
        raw.f0.assign(output.frames, 0.0);
        raw.time_axis.resize(output.frames);

        if (!output.direct_f0 && (output.bins == 0 || output.salience.size() < output.frames * output.bins)) {
            return raw;
        }

        const double threshold = configure.f0_neural_threshold;
        const double floor = configure.f0_neural_floor;
        const double ceil = configure.f0_neural_ceil;

        for (std::size_t frame = 0; frame < output.frames; ++frame) {
            raw.time_axis[frame] = static_cast<double>(frame) * rmvpe_frame_period_ms / 1000.0;

            if (output.direct_f0) {
                if (frame >= output.f0.size() || frame >= output.uv.size()) {
                    continue;
                }
                const double f0 = static_cast<double>(output.f0[frame]);
                const bool unvoiced = output.uv[frame] != 0;
                if (!unvoiced && std::isfinite(f0) && f0 >= floor && f0 <= ceil) {
                    raw.f0[frame] = f0;
                }
                continue;
            }

            if (output.bins != 360) {
                continue;
            }
            const auto *row = output.salience.data() + frame * output.bins;
            const auto *peak_it = std::max_element(row, row + output.bins);
            const auto peak = static_cast<std::size_t>(peak_it - row);
            if (static_cast<double>(*peak_it) <= threshold) {
                continue;
            }

            const auto first = peak > 4 ? peak - 4 : 0;
            const auto last = std::min(output.bins - 1, peak + 4);
            double weighted_cents = 0.0;
            double weight_sum = 0.0;
            for (std::size_t bin = first; bin <= last; ++bin) {
                const double weight = std::max(0.0, static_cast<double>(row[bin]));
                weighted_cents += weight * (20.0 * static_cast<double>(bin) + rmvpe_cents_offset);
                weight_sum += weight;
            }
            if (weight_sum <= 0.0) {
                continue;
            }

            const double cents = weighted_cents / weight_sum;
            const double f0 = 10.0 * std::pow(2.0, cents / 1200.0);
            if (std::isfinite(f0) && f0 >= floor && f0 <= ceil) {
                raw.f0[frame] = f0;
            }
        }
        return raw;
    }

}// namespace lessampler
