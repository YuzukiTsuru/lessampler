/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_NEURALFRONTENDKIND_H
#define LESSAMPLER_NEURALFRONTENDKIND_H

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <string_view>

namespace lessampler {

    enum class NeuralFrontendKind {
        world,
        world_spectral,
        source_mel,
        source_wave,
    };

    inline std::string_view NeuralFrontendKindName(NeuralFrontendKind kind) noexcept {
        switch (kind) {
            case NeuralFrontendKind::world_spectral:
                return "world-spectral";
            case NeuralFrontendKind::source_mel:
                return "source-mel";
            case NeuralFrontendKind::source_wave:
                return "source-wave";
            case NeuralFrontendKind::world:
            default:
                return "world";
        }
    }

    inline std::optional<NeuralFrontendKind> ParseNeuralFrontendKind(std::string_view name) {
        std::string lower(name);
        std::ranges::transform(lower, lower.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        if (lower == "world") {
            return NeuralFrontendKind::world;
        }
        if (lower == "world-spectral" || lower == "world_spectral") {
            return NeuralFrontendKind::world_spectral;
        }
        if (lower == "source-mel" || lower == "source_mel" || lower == "source") {
            return NeuralFrontendKind::source_mel;
        }
        if (lower == "source-wave" || lower == "source_wave") {
            return NeuralFrontendKind::source_wave;
        }
        return std::nullopt;
    }

}// namespace lessampler

#endif//LESSAMPLER_NEURALFRONTENDKIND_H
