/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_VOICESTYLE_H
#define LESSAMPLER_VOICESTYLE_H

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <string_view>

namespace lessampler {

    enum class VoiceStyleKind {
        natural,
        clear,
        cry,
        growl,
    };

    inline std::string_view VoiceStyleKindName(VoiceStyleKind kind) noexcept {
        switch (kind) {
            case VoiceStyleKind::clear:
                return "clear";
            case VoiceStyleKind::cry:
                return "cry";
            case VoiceStyleKind::growl:
                return "growl";
            case VoiceStyleKind::natural:
            default:
                return "natural";
        }
    }

    inline std::optional<VoiceStyleKind> ParseVoiceStyleKind(std::string_view name) {
        std::string lower(name);
        std::ranges::transform(lower, lower.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        if (lower == "natural") {
            return VoiceStyleKind::natural;
        }
        if (lower == "clear") {
            return VoiceStyleKind::clear;
        }
        if (lower == "cry") {
            return VoiceStyleKind::cry;
        }
        if (lower == "growl") {
            return VoiceStyleKind::growl;
        }
        return std::nullopt;
    }

    struct StyleWeights {
        double clear = 0.0;
        double cry = 0.0;
        double growl = 0.0;

        friend bool operator==(const StyleWeights &, const StyleWeights &) = default;

        [[nodiscard]] bool empty() const noexcept {
            return clear == 0.0 && cry == 0.0 && growl == 0.0;
        }
    };

    struct VoiceStyle {
        StyleWeights start;
        StyleWeights end;
        double morph_ms = 0.0;
        bool from_flags = false;

        [[nodiscard]] bool active() const noexcept {
            return !start.empty() || !end.empty();
        }

        [[nodiscard]] bool morphs() const noexcept {
            return start != end;
        }
    };

    inline StyleWeights WeightsFor(VoiceStyleKind kind, double amount = 1.0) {
        const double scaled = std::clamp(amount, 0.0, 1.0);
        StyleWeights weights;
        switch (kind) {
            case VoiceStyleKind::clear:
                weights.clear = scaled;
                break;
            case VoiceStyleKind::cry:
                weights.cry = scaled;
                break;
            case VoiceStyleKind::growl:
                weights.growl = scaled;
                break;
            case VoiceStyleKind::natural:
            default:
                break;
        }
        return weights;
    }

    inline StyleWeights LerpWeights(const StyleWeights &start, const StyleWeights &end, double position) {
        const double t = std::clamp(position, 0.0, 1.0);
        return StyleWeights{
                start.clear + (end.clear - start.clear) * t,
                start.cry + (end.cry - start.cry) * t,
                start.growl + (end.growl - start.growl) * t,
        };
    }

    inline double StyleMorphPosition(double elapsed_ms, double note_duration_ms, double morph_ms) {
        if (morph_ms > 0.0) {
            return std::clamp(elapsed_ms / morph_ms, 0.0, 1.0);
        }
        if (note_duration_ms > 0.0) {
            return std::clamp(elapsed_ms / note_duration_ms, 0.0, 1.0);
        }
        return 1.0;
    }

    namespace detail {

        inline bool parse_style_group(std::string_view group, StyleWeights &weights) {
            bool found = false;
            for (std::size_t i = 0; i < group.size();) {
                const char key = group[i];
                if (key != 'C' && key != 'Y' && key != 'G') {
                    ++i;
                    continue;
                }
                ++i;
                int value = 0;
                bool has_digits = false;
                while (i < group.size() && std::isdigit(static_cast<unsigned char>(group[i]))) {
                    has_digits = true;
                    value = std::min(100, value * 10 + (group[i] - '0'));
                    ++i;
                }
                if (!has_digits) {
                    value = 100;
                }
                const double weight = static_cast<double>(std::clamp(value, 0, 100)) / 100.0;
                if (key == 'C') {
                    weights.clear = weight;
                } else if (key == 'Y') {
                    weights.cry = weight;
                } else {
                    weights.growl = weight;
                }
                found = true;
            }
            return found;
        }

    }// namespace detail

    /*
     * Parse lessampler style tokens from a UTAU flags string.
     * Grammar: [Cnn][Ynn][Gnn] (">" [Cnn][Ynn][Gnn])?
     * A missing number means 100. Unknown flags are ignored. Uppercase keys are
     * reserved for style so lowercase resampler flags keep their old meaning.
     */
    inline bool ParseVoiceStyleFlags(std::string_view flags, VoiceStyle &style) {
        const auto separator = flags.find('>');
        StyleWeights start;
        StyleWeights end;
        bool has_start = false;
        bool has_end = false;

        if (separator == std::string_view::npos) {
            has_start = detail::parse_style_group(flags, start);
            end = start;
            has_end = has_start;
        } else {
            has_start = detail::parse_style_group(flags.substr(0, separator), start);
            const auto end_text = flags.substr(separator + 1);
            const auto second_separator = end_text.find('>');
            has_end = detail::parse_style_group(end_text.substr(0, second_separator), end);
            if (!has_start && has_end) {
                start = end;
                has_start = true;
            }
            if (has_start && !has_end) {
                end = start;
                has_end = true;
            }
        }

        if (!has_start && !has_end) {
            return false;
        }
        style.start = start;
        style.end = end;
        style.morph_ms = 0.0;
        style.from_flags = true;
        return true;
    }

}// namespace lessampler

#endif//LESSAMPLER_VOICESTYLE_H
