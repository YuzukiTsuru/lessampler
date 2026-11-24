/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScaleConvert.h"

#include <cmath>
#include <format>

namespace lessampler {

    ScaleConvert::ScaleConvert(std::string_view scale_name) : scale_num_(ScaleConvertToDouble(scale_name)) {}

    std::expected<double, std::string> ScaleConvert::GetScaleNum() const {
        return scale_num_;
    }

    std::expected<double, std::string> ScaleConvert::ScaleConvertToDouble(std::string_view scale_name) {
        if (scale_name.size() < 2 || scale_name.size() > 3) {
            return std::unexpected(std::format("expected a note name such as A4 or A#3, got '{}'", scale_name));
        }

        const char note = scale_name[0];
        int bias = 0;
        std::size_t octave_index = 1;
        if (scale_name.size() == 3) {
            if (scale_name[1] != '#') {
                return std::unexpected(std::format("unexpected accidental in '{}'", scale_name));
            }
            bias = 1;
            octave_index = 2;
        }

        if (scale_name[octave_index] < '0' || scale_name[octave_index] > '9') {
            return std::unexpected(std::format("missing octave in '{}'", scale_name));
        }

        int scale = 0;
        switch (note) {
            case 'C':
                scale = -9 + bias;
                break;
            case 'D':
                scale = -7 + bias;
                break;
            case 'E':
                if (bias != 0) {
                    return std::unexpected(std::format("E# is not supported: '{}'", scale_name));
                }
                scale = -5;
                break;
            case 'F':
                scale = -4 + bias;
                break;
            case 'G':
                scale = -2 + bias;
                break;
            case 'A':
                scale = bias;
                break;
            case 'B':
                if (bias != 0) {
                    return std::unexpected(std::format("B# is not supported: '{}'", scale_name));
                }
                scale = 2;
                break;
            default:
                return std::unexpected(std::format("unknown note '{}'", scale_name));
        }

        const int octave = scale_name[octave_index] - '0' - 4;
        const double frequency = std::pow(2.0, octave) * std::pow(2.0, static_cast<double>(scale) / 12.0) * 440.0;
        return frequency;
    }

}// namespace lessampler
