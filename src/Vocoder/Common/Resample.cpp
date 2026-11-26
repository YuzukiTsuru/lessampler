/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Resample.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <vector>

#include "Core/Exception.h"

namespace lessampler {

    std::vector<double> Resample(std::span<const double> samples, int from_rate, int to_rate) {
        if (from_rate <= 0 || to_rate <= 0) {
            throw parameter_error("Invalid sample rate for resampling");
        }
        if (samples.empty() || from_rate == to_rate) {
            return std::vector<double>(samples.begin(), samples.end());
        }

        const double ratio = static_cast<double>(to_rate) / static_cast<double>(from_rate);
        const auto output_length = static_cast<std::size_t>(std::llround(static_cast<double>(samples.size()) * ratio));
        std::vector<double> output(output_length, 0.0);

        constexpr int half_taps = 16;
        const double cutoff = std::min(1.0, ratio) * 0.97;

        for (std::size_t n = 0; n < output_length; ++n) {
            const double position = static_cast<double>(n) / ratio;
            const auto center = static_cast<std::ptrdiff_t>(std::floor(position));
            double sum = 0.0;
            double weight_sum = 0.0;
            for (int tap = -half_taps + 1; tap <= half_taps; ++tap) {
                const auto index = center + tap;
                if (index < 0 || static_cast<std::size_t>(index) >= samples.size()) {
                    continue;
                }
                const double distance = position - static_cast<double>(index);
                if (std::abs(distance) >= half_taps) {
                    continue;
                }
                const double window = 0.5 + 0.5 * std::cos(std::numbers::pi * distance / half_taps);
                const double sinc = std::abs(distance) < 1e-12
                                            ? 1.0
                                            : std::sin(std::numbers::pi * cutoff * distance) /
                                                      (std::numbers::pi * cutoff * distance);
                const double weight = cutoff * sinc * window;
                sum += samples[static_cast<std::size_t>(index)] * weight;
                weight_sum += weight;
            }
            output[n] = weight_sum == 0.0 ? 0.0 : sum / weight_sum;
        }
        return output;
    }

    std::size_t F0FrameCount(std::size_t sample_count, int sample_rate, double frame_period_ms) {
        if (sample_rate <= 0 || !(frame_period_ms > 0.0)) {
            throw parameter_error("Invalid F0 frame period");
        }
        const double frames = 1000.0 * static_cast<double>(sample_count) /
                              static_cast<double>(sample_rate) / frame_period_ms;
        return static_cast<std::size_t>(frames) + 1;
    }

    F0Track ResampleF0Track(const F0Track &source, std::size_t sample_count, int sample_rate,
                            double frame_period_ms) {
        F0Track output;
        output.frame_period = frame_period_ms;
        const auto frames = F0FrameCount(sample_count, sample_rate, frame_period_ms);
        output.f0.assign(frames, 0.0);
        output.time_axis.resize(frames);

        if (source.f0.empty() || !(source.frame_period > 0.0)) {
            for (std::size_t i = 0; i < frames; ++i) {
                output.time_axis[i] = static_cast<double>(i) * frame_period_ms / 1000.0;
            }
            return output;
        }

        const auto last = source.f0.size() - 1;
        for (std::size_t i = 0; i < frames; ++i) {
            const double time_ms = static_cast<double>(i) * frame_period_ms;
            output.time_axis[i] = time_ms / 1000.0;

            const double position = time_ms / source.frame_period;
            if (position <= 0.0) {
                output.f0[i] = source.f0.front();
                continue;
            }
            if (position >= static_cast<double>(last)) {
                output.f0[i] = source.f0.back();
                continue;
            }

            const auto left = static_cast<std::size_t>(position);
            const auto right = left + 1;
            const double fraction = position - static_cast<double>(left);
            const double a = source.f0[left];
            const double b = source.f0[right];
            if (a > 0.0 && b > 0.0) {
                output.f0[i] = a * (1.0 - fraction) + b * fraction;
            } else if (a <= 0.0 && b <= 0.0) {
                output.f0[i] = 0.0;
            } else {
                output.f0[i] = fraction < 0.5 ? a : b;
            }
        }
        return output;
    }

}// namespace lessampler
