/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_F0ESTIMATORKIND_H
#define LESSAMPLER_F0ESTIMATORKIND_H

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <string_view>

namespace lessampler {

    enum class F0EstimatorKind {
        auto_select,
        harvest,
        dio,
        llsm,
        neural,
    };

    inline std::string_view F0EstimatorKindName(F0EstimatorKind kind) noexcept {
        switch (kind) {
            case F0EstimatorKind::harvest:
                return "harvest";
            case F0EstimatorKind::dio:
                return "dio";
            case F0EstimatorKind::llsm:
                return "llsm";
            case F0EstimatorKind::neural:
                return "neural";
            case F0EstimatorKind::auto_select:
            default:
                return "auto";
        }
    }

    inline std::optional<F0EstimatorKind> ParseF0EstimatorKind(std::string_view name) {
        std::string lower(name);
        std::ranges::transform(lower, lower.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });

        if (lower == "auto") {
            return F0EstimatorKind::auto_select;
        }
        if (lower == "harvest") {
            return F0EstimatorKind::harvest;
        }
        if (lower == "dio") {
            return F0EstimatorKind::dio;
        }
        if (lower == "llsm") {
            return F0EstimatorKind::llsm;
        }
        if (lower == "neural") {
            return F0EstimatorKind::neural;
        }
        return std::nullopt;
    }

}// namespace lessampler

#endif//LESSAMPLER_F0ESTIMATORKIND_H
