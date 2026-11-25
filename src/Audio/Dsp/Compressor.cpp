/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Audio/Dsp/Compressor.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace lessampler {

    namespace {

        double time_coefficient(int sample_rate, double milliseconds) {
            if (sample_rate <= 0 || milliseconds <= 0.0) {
                return 0.0;
            }
            return std::exp(-1.0 / (static_cast<double>(sample_rate) * milliseconds / 1000.0));
        }

    }// namespace

    void Compressor::Configure(int sample_rate, double threshold_db, double ratio,
                               double attack_ms, double release_ms, double makeup_db) {
        threshold_db_ = threshold_db;
        ratio_ = std::max(1.0, ratio);
        makeup_db_ = makeup_db;
        attack_coefficient_ = time_coefficient(sample_rate, attack_ms);
        release_coefficient_ = time_coefficient(sample_rate, release_ms);
        envelope_ = 0.0;
    }

    void Compressor::Process(std::span<double> interleaved, int channels) {
        if (interleaved.empty() || channels <= 0 || ratio_ <= 1.0) {
            return;
        }
        const auto channel_count = static_cast<std::size_t>(channels);
        const auto frames = interleaved.size() / channel_count;
        const double makeup = std::pow(10.0, makeup_db_ / 20.0);

        for (std::size_t frame = 0; frame < frames; ++frame) {
            const auto base = frame * channel_count;
            double peak = 0.0;
            for (std::size_t channel = 0; channel < channel_count; ++channel) {
                peak = std::max(peak, std::abs(interleaved[base + channel]));
            }

            const double coefficient = peak > envelope_ ? attack_coefficient_ : release_coefficient_;
            envelope_ = coefficient * envelope_ + (1.0 - coefficient) * peak;

            double gain = 1.0;
            if (envelope_ > 1e-9) {
                const double input_db = 20.0 * std::log10(envelope_);
                if (input_db > threshold_db_) {
                    const double reduction_db = (threshold_db_ - input_db) * (1.0 - 1.0 / ratio_);
                    gain = std::pow(10.0, (reduction_db + makeup_db_) / 20.0);
                } else {
                    gain = makeup;
                }
            }
            for (std::size_t channel = 0; channel < channel_count; ++channel) {
                interleaved[base + channel] *= gain;
            }
        }
    }

}// namespace lessampler
