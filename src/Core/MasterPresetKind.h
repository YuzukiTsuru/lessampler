/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_MASTERPRESETKIND_H
#define LESSAMPLER_MASTERPRESETKIND_H

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <string_view>

namespace lessampler {

    enum class MasterPresetKind {
        none,
        natural,
        warm,
        power,
    };

    inline std::string_view MasterPresetKindName(MasterPresetKind kind) noexcept {
        switch (kind) {
            case MasterPresetKind::none:
                return "none";
            case MasterPresetKind::natural:
                return "natural";
            case MasterPresetKind::power:
                return "power";
            case MasterPresetKind::warm:
            default:
                return "warm";
        }
    }

    inline std::optional<MasterPresetKind> ParseMasterPresetKind(std::string_view name) {
        std::string lower(name);
        std::ranges::transform(lower, lower.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });

        if (lower == "none") {
            return MasterPresetKind::none;
        }
        if (lower == "natural") {
            return MasterPresetKind::natural;
        }
        if (lower == "warm") {
            return MasterPresetKind::warm;
        }
        if (lower == "power") {
            return MasterPresetKind::power;
        }
        return std::nullopt;
    }

}// namespace lessampler

#endif//LESSAMPLER_MASTERPRESETKIND_H
