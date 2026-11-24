/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Config.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <format>
#include <string>

namespace lessampler {

    namespace {

        std::uint64_t fnv1a(std::string_view data) {
            std::uint64_t hash = 1469598103934665603ULL;
            for (unsigned char c: data) {
                hash ^= c;
                hash *= 1099511628211ULL;
            }
            return hash;
        }

    }// namespace

    std::string Config::fingerprint() const {
        // The vocoder backend is deliberately not part of the fingerprint: the
        // cached .lessaudio holds WORLD analysis only, so switching between WORLD
        // and LLSM must not invalidate an existing WORLD cache.
        const std::string data = std::format(
                "{}|{}|{}|{}|{}|{}|{}|{}|{}|{}",
                audio_model_frame_period,
                fft_size,
                model_amp,
                f0_speed,
                f0_dio_floor,
                f0_harvest_floor,
                f0_cheap_trick_floor,
                f0_allow_range,
                ap_threshold,
                f0_mode_name(f0_mode));
        return std::format("{:016x}", fnv1a(data));
    }

    std::string_view Config::f0_mode_name(F0_MODE mode) {
        switch (mode) {
            case F0_MODE::F0_MODE_DIO:
                return "DIO";
            case F0_MODE::F0_MODE_HARVEST:
                return "HARVEST";
            default:
                return "UNKNOWN";
        }
    }

    Config::F0_MODE Config::parse_f0_mode(std::string_view name) {
        std::string upper(name);
        std::ranges::transform(upper, upper.begin(), [](unsigned char c) {
            return static_cast<char>(std::toupper(c));
        });

        if (upper == "DIO") {
            return F0_MODE::F0_MODE_DIO;
        }
        if (upper == "HARVEST") {
            return F0_MODE::F0_MODE_HARVEST;
        }
        return F0_MODE::F0_MODE_UNKNOWN;
    }

}// namespace lessampler
