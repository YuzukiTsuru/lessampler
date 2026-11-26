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

        const auto raw = DecodeRmvpeOutput(output, configure);
        return ResampleF0Track(raw, x.size(), fs, configure.audio_model_frame_period);
    }

}// namespace lessampler
