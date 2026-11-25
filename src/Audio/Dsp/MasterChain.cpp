/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Audio/Dsp/MasterChain.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

#include "Audio/Dsp/Biquad.h"
#include "Audio/Dsp/Compressor.h"
#include "Audio/Dsp/Limiter.h"
#include "Audio/Dsp/Saturator.h"

namespace lessampler {

    MasterOptions CreateMasterOptions(MasterPresetKind kind) {
        MasterOptions options;
        switch (kind) {
            case MasterPresetKind::none:
                options.enabled = false;
                return options;
            case MasterPresetKind::natural:
                options.enabled = true;
                options.ceiling_db = -1.0;
                return options;
            case MasterPresetKind::warm:
                options.highpass_hz = 40.0;
                options.low_shelf_hz = 180.0;
                options.low_shelf_db = 1.5;
                options.presence_hz = 3000.0;
                options.presence_db = 1.0;
                options.compressor_threshold_db = -18.0;
                options.compressor_ratio = 2.0;
                options.compressor_attack_ms = 10.0;
                options.compressor_release_ms = 120.0;
                options.compressor_makeup_db = 3.0;
                options.saturation = 0.10;
                options.target_rms_db = -18.0;
                options.ceiling_db = -1.0;
                return options;
            case MasterPresetKind::power:
                options.highpass_hz = 40.0;
                options.low_shelf_hz = 160.0;
                options.low_shelf_db = 2.0;
                options.presence_hz = 3200.0;
                options.presence_db = 1.5;
                options.compressor_threshold_db = -20.0;
                options.compressor_ratio = 3.0;
                options.compressor_attack_ms = 8.0;
                options.compressor_release_ms = 100.0;
                options.compressor_makeup_db = 5.0;
                options.saturation = 0.18;
                options.target_rms_db = -17.0;
                options.ceiling_db = -1.0;
                return options;
        }
        throw std::invalid_argument("unknown master preset kind");
    }

    MasterOptions CreateMasterOptions(std::string_view name) {
        if (name.empty()) {
            return CreateMasterOptions(MasterPresetKind::warm);
        }
        const auto kind = ParseMasterPresetKind(name);
        if (!kind) {
            throw std::invalid_argument(
                    "unknown master preset: " + std::string(name) + " (expected " + AvailableMasterPresets() + ")");
        }
        return CreateMasterOptions(*kind);
    }

    MasterOptions MasterOptions::Preset(std::string_view name) {
        return CreateMasterOptions(name);
    }

    std::string AvailableMasterPresets() {
        return "none natural warm power";
    }

    void MasterChain::Process(std::span<double> interleaved, int sample_rate, int channels,
                              const MasterOptions &options) {
        if (interleaved.empty() || sample_rate <= 0 || channels <= 0 || !options.enabled) {
            return;
        }
        const auto channel_count = static_cast<std::size_t>(channels);
        const auto frames = interleaved.size() / channel_count;
        if (frames == 0) {
            return;
        }

        const bool has_tone = options.highpass_hz > 0.0 || std::abs(options.low_shelf_db) > 1e-9 ||
                              std::abs(options.presence_db) > 1e-9;
        if (has_tone) {
            std::vector<Biquad> highpass(channel_count);
            std::vector<Biquad> low_shelf(channel_count);
            std::vector<Biquad> presence(channel_count);
            for (std::size_t channel = 0; channel < channel_count; ++channel) {
                if (options.highpass_hz > 0.0) {
                    highpass[channel].SetHighPass(sample_rate, options.highpass_hz);
                }
                if (std::abs(options.low_shelf_db) > 1e-9) {
                    low_shelf[channel].SetLowShelf(sample_rate, options.low_shelf_hz, options.low_shelf_db);
                }
                if (std::abs(options.presence_db) > 1e-9) {
                    presence[channel].SetPeaking(sample_rate, options.presence_hz, options.presence_db, 0.9);
                }
            }
            for (std::size_t frame = 0; frame < frames; ++frame) {
                for (std::size_t channel = 0; channel < channel_count; ++channel) {
                    const auto index = frame * channel_count + channel;
                    double value = interleaved[index];
                    if (options.highpass_hz > 0.0) {
                        value = highpass[channel].Process(value);
                    }
                    if (std::abs(options.low_shelf_db) > 1e-9) {
                        value = low_shelf[channel].Process(value);
                    }
                    if (std::abs(options.presence_db) > 1e-9) {
                        value = presence[channel].Process(value);
                    }
                    interleaved[index] = value;
                }
            }
        }

        if (options.compressor_ratio > 1.0) {
            Compressor compressor;
            compressor.Configure(sample_rate, options.compressor_threshold_db, options.compressor_ratio,
                                 options.compressor_attack_ms, options.compressor_release_ms,
                                 options.compressor_makeup_db);
            compressor.Process(interleaved, channels);
        }

        if (options.saturation > 0.0) {
            Saturator::Process(interleaved, 1.5, options.saturation);
        }

        if (options.target_rms_db < 0.0) {
            double sum = 0.0;
            std::size_t active = 0;
            for (double value: interleaved) {
                if (std::isfinite(value) && std::abs(value) > 1e-4) {
                    sum += value * value;
                    ++active;
                }
            }
            if (active > 0) {
                const double rms = std::sqrt(sum / static_cast<double>(active));
                const double target = std::pow(10.0, options.target_rms_db / 20.0);
                if (rms > 1e-9) {
                    const double gain = std::clamp(target / rms, 0.1, 10.0);
                    for (double &value: interleaved) {
                        value *= gain;
                    }
                }
            }
        }

        Limiter::Process(interleaved, sample_rate, channels, options.ceiling_db,
                         options.limiter_lookahead_ms, options.limiter_release_ms);
    }

}// namespace lessampler
