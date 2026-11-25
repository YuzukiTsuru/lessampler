/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_NEURALVOCODERPRESETKIND_H
#define LESSAMPLER_NEURALVOCODERPRESETKIND_H

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <string_view>

namespace lessampler {

    enum class NeuralVocoderPresetKind {
        nhv,
        pc_nsf_hifigan,
    };

    inline std::string_view NeuralVocoderPresetKindName(NeuralVocoderPresetKind kind) noexcept {
        switch (kind) {
            case NeuralVocoderPresetKind::pc_nsf_hifigan:
                return "pc-nsf-hifigan";
            case NeuralVocoderPresetKind::nhv:
            default:
                return "nhv";
        }
    }

    inline std::optional<NeuralVocoderPresetKind> ParseNeuralVocoderPresetKind(std::string_view name) {
        std::string lower(name);
        std::ranges::transform(lower, lower.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });

        if (lower == "nhv") {
            return NeuralVocoderPresetKind::nhv;
        }
        if (lower == "pc-nsf-hifigan") {
            return NeuralVocoderPresetKind::pc_nsf_hifigan;
        }
        return std::nullopt;
    }

}// namespace lessampler

#endif//LESSAMPLER_NEURALVOCODERPRESETKIND_H
