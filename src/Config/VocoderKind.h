/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_VOCODERKIND_H
#define LESSAMPLER_VOCODERKIND_H

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <string_view>

namespace lessampler {

    enum class VocoderKind {
        world,
        llsm,
        neural,
    };

    inline std::string_view VocoderKindName(VocoderKind kind) {
        switch (kind) {
            case VocoderKind::llsm:
                return "llsm";
            case VocoderKind::neural:
                return "neural";
            case VocoderKind::world:
            default:
                return "world";
        }
    }

    inline std::optional<VocoderKind> ParseVocoderKind(std::string_view name) {
        std::string lower(name);
        std::ranges::transform(lower, lower.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });

        if (lower == "world") {
            return VocoderKind::world;
        }
        if (lower == "llsm") {
            return VocoderKind::llsm;
        }
        if (lower == "neural") {
            return VocoderKind::neural;
        }
        return std::nullopt;
    }

}// namespace lessampler

#endif//LESSAMPLER_VOCODERKIND_H
