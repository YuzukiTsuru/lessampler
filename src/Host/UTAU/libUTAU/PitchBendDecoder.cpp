/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "PitchBendDecoder.h"

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <string_view>

PitchBendDecoder::PitchBendDecoder(std::string_view value, int count)
    : pitch(value), pitch_bend(static_cast<std::size_t>(std::max(0, count)), 0) {
    PitchBendDecode();
}

int PitchBendDecoder::GetDataFromUTAU64(char value) {
    if (value >= '0' && value <= '9') {
        return value - '0' + 52;
    }
    if (value >= 'A' && value <= 'Z') {
        return value - 'A';
    }
    if (value >= 'a' && value <= 'z') {
        return value - 'a' + 26;
    }
    if (value == '+') {
        return 62;
    }
    if (value == '/') {
        return 63;
    }
    return 0;
}

void PitchBendDecoder::PitchBendDecode() {
    std::size_t index = 0;
    std::size_t cursor = 0;
    int last_value = 0;

    while (index < pitch.size()) {
        if (pitch[index] == '#') {
            const auto end = pitch.find('#', index + 1);
            if (end == std::string_view::npos) {
                break;
            }

            int repeat = 0;
            const auto text = pitch.substr(index + 1, end - index - 1);
            const auto result = std::from_chars(text.data(), text.data() + text.size(), repeat);
            if (result.ec == std::errc{} && result.ptr == text.data() + text.size() && repeat > 0) {
                const auto available = pitch_bend.size() - std::min(pitch_bend.size(), cursor);
                const auto count_to_write = std::min(static_cast<std::size_t>(repeat), available);
                std::fill_n(pitch_bend.begin() + static_cast<std::ptrdiff_t>(cursor), count_to_write, last_value);
                cursor += count_to_write;
            }
            index = end + 1;
            continue;
        }

        if (index + 1 >= pitch.size()) {
            break;
        }

        int value = GetDataFromUTAU64(pitch[index]) * 64 + GetDataFromUTAU64(pitch[index + 1]);
        if (value > 2047) {
            value -= 4096;
        }
        last_value = value;
        if (cursor < pitch_bend.size()) {
            pitch_bend[cursor++] = value;
        }
        index += 2;
    }
}

std::vector<int> PitchBendDecoder::GetPitchBend() const {
    return pitch_bend;
}
