/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Audio/Dsp/Limiter.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <deque>
#include <vector>

namespace lessampler {

    void Limiter::Process(std::span<double> interleaved, int sample_rate, int channels,
                          double ceiling_db, double lookahead_ms, double release_ms) {
        if (interleaved.empty() || sample_rate <= 0 || channels <= 0) {
            return;
        }
        const auto channel_count = static_cast<std::size_t>(channels);
        const auto frames = interleaved.size() / channel_count;
        if (frames == 0) {
            return;
        }

        const double ceiling = std::clamp(std::pow(10.0, ceiling_db / 20.0), 1e-6, 1.0);
        const auto lookahead = static_cast<std::size_t>(
                std::max<long long>(0, std::llround(lookahead_ms * static_cast<double>(sample_rate) / 1000.0)));
        const double release_coefficient = release_ms > 0.0
                                                   ? std::exp(-1.0 / (static_cast<double>(sample_rate) * release_ms / 1000.0))
                                                   : 0.0;

        std::vector<double> required(frames, 1.0);
        for (std::size_t frame = 0; frame < frames; ++frame) {
            double peak = 0.0;
            for (std::size_t channel = 0; channel < channel_count; ++channel) {
                peak = std::max(peak, std::abs(interleaved[frame * channel_count + channel]));
            }
            if (peak > ceiling) {
                required[frame] = ceiling / peak;
            }
        }

        // Sliding minimum over [frame, frame + lookahead]. The gain reduction
        // therefore starts before the peak and never lets a sample through
        // above the ceiling.
        std::vector<double> minimum(frames, 1.0);
        std::deque<std::size_t> window;
        for (std::size_t index = 0; index < std::min(lookahead + 1, frames); ++index) {
            while (!window.empty() && required[window.back()] >= required[index]) {
                window.pop_back();
            }
            window.push_back(index);
        }
        for (std::size_t frame = 0; frame < frames; ++frame) {
            while (!window.empty() && window.front() < frame) {
                window.pop_front();
            }
            minimum[frame] = window.empty() ? 1.0 : required[window.front()];

            const std::size_t incoming = frame + lookahead + 1;
            if (incoming < frames) {
                while (!window.empty() && required[window.back()] >= required[incoming]) {
                    window.pop_back();
                }
                window.push_back(incoming);
            }
        }

        double gain = 1.0;
        for (std::size_t frame = 0; frame < frames; ++frame) {
            const double target = minimum[frame];
            if (target < gain) {
                gain = target;
            } else {
                gain = 1.0 - (1.0 - gain) * release_coefficient;
                gain = std::min(gain, target);
            }
            for (std::size_t channel = 0; channel < channel_count; ++channel) {
                const auto index = frame * channel_count + channel;
                interleaved[index] = std::clamp(interleaved[index] * gain, -ceiling, ceiling);
            }
        }
    }

}// namespace lessampler
