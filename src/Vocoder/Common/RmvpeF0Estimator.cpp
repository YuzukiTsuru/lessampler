/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "RmvpeF0Estimator.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

#include "Config/Config.h"
#include "Core/Exception.h"
#include "Vocoder/Common/Resample.h"

namespace lessampler {

    namespace {

        constexpr int rmvpe_sample_rate = 16000;
        constexpr std::size_t rmvpe_hop = 160;
        constexpr double rmvpe_frame_period_ms = 10.0;
        constexpr double rmvpe_cents_offset = 1997.3794084376191;

    }// namespace

    RmvpeF0Estimator::RmvpeF0Estimator(const Config &configure)
        : model_(LoadRmvpeModel(configure.f0_neural_model_path, configure.f0_neural_threads)) {}

    std::string_view RmvpeF0Estimator::name() const noexcept {
        return "neural";
    }

    F0Track RmvpeF0Estimator::Estimate(std::span<const double> x, int fs, const Config &configure) const {
        if (x.empty() || fs <= 0) {
            throw parameter_error("Empty audio input");
        }

        auto wave = Resample(x, fs, rmvpe_sample_rate);
        if (wave.empty()) {
            throw parameter_error("RMVPE input is empty after resampling");
        }
        const auto padded = ((wave.size() + rmvpe_hop - 1) / rmvpe_hop) * rmvpe_hop;
        wave.resize(padded, 0.0);

        std::vector<float> input(wave.size());
        std::ranges::transform(wave, input.begin(), [](double value) {
            return static_cast<float>(value);
        });

        const auto output = model_->Infer(input, static_cast<float>(configure.f0_neural_threshold));
        if (output.Empty()) {
            throw parameter_error("RMVPE produced no frames");
        }

        F0Track raw;
        raw.frame_period = rmvpe_frame_period_ms;
        raw.f0.assign(output.frames, 0.0);
        raw.time_axis.resize(output.frames);

        const double threshold = configure.f0_neural_threshold;
        const double floor = configure.f0_neural_floor;
        const double ceil = configure.f0_neural_ceil;

        for (std::size_t frame = 0; frame < output.frames; ++frame) {
            raw.time_axis[frame] = static_cast<double>(frame) * rmvpe_frame_period_ms / 1000.0;

            if (output.direct_f0) {
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

        return ResampleF0Track(raw, x.size(), fs, configure.audio_model_frame_period);
    }

}// namespace lessampler
