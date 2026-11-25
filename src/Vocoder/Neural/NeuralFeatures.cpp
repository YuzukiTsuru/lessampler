/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Vocoder/Neural/NeuralFeatures.h"

#include <algorithm>
#include <cmath>

#include "Vocoder/Neural/MelSpectrogram.h"

namespace lessampler {

    namespace {

        double sample_f0(const F0Track &track, double time_ms, double fallback) {
            if (track.f0.empty()) {
                return fallback;
            }
            if (track.frame_period <= 0.0) {
                return track.f0.front();
            }
            const double position = time_ms / track.frame_period;
            if (position <= 0.0) {
                return track.f0.front();
            }
            const auto index = static_cast<std::size_t>(position);
            if (index + 1 >= track.f0.size()) {
                return track.f0.back();
            }
            const double fraction = position - static_cast<double>(index);
            return track.f0[index] * (1.0 - fraction) + track.f0[index + 1] * fraction;
        }

    }// namespace

    void FillUnvoicedF0(std::vector<float> &f0, std::span<const float> uv, std::size_t frames) {
        if (frames == 0 || f0.size() < frames || uv.size() < frames) {
            return;
        }

        std::size_t first_voiced = frames;
        std::size_t last_voiced = 0;
        for (std::size_t i = 0; i < frames; ++i) {
            if (uv[i] == 0.0f) {
                first_voiced = std::min(first_voiced, i);
                last_voiced = i;
            }
        }
        if (first_voiced == frames) {
            return;
        }

        for (std::size_t i = 0; i < first_voiced; ++i) {
            f0[i] = f0[first_voiced];
        }
        for (std::size_t i = last_voiced + 1; i < frames; ++i) {
            f0[i] = f0[last_voiced];
        }

        std::size_t previous = first_voiced;
        for (std::size_t i = first_voiced + 1; i <= last_voiced; ++i) {
            if (uv[i] == 0.0f) {
                if (i > previous + 1) {
                    const auto steps = static_cast<float>(i - previous);
                    for (std::size_t j = previous + 1; j < i; ++j) {
                        const float fraction = static_cast<float>(j - previous) / steps;
                        f0[j] = f0[previous] * (1.0f - fraction) + f0[i] * fraction;
                    }
                }
                previous = i;
            }
        }
    }

    void BuildMelF0Track(const F0Track &track, std::size_t frames, std::vector<float> &f0,
                         std::vector<float> &uv) {
        f0.assign(frames, 0.0f);
        uv.assign(frames, 1.0f);
        if (track.f0.empty() || frames == 0) {
            return;
        }

        const double frame_ms = static_cast<double>(MelSpectrogram::hop_size) * 1000.0 /
                                static_cast<double>(MelSpectrogram::sample_rate);
        for (std::size_t i = 0; i < frames; ++i) {
            const double value = sample_f0(track, static_cast<double>(i) * frame_ms, 0.0);
            if (std::isfinite(value) && value > 0.0) {
                f0[i] = static_cast<float>(std::clamp(value, neural_f0_min, neural_f0_max));
                uv[i] = 0.0f;
            }
        }

        FillUnvoicedF0(f0, uv, frames);
    }

}// namespace lessampler
