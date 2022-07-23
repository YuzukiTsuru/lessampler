/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <sstream>
#include "PitchBendDecoder.h"

PitchBendDecoder::PitchBendDecoder(std::string &str, int count) : pitch(std::move(str)), count(count) {
    pitch_string_length = static_cast<int>(pitch.length());
    pitch_bend.resize(count);
    std::fill(pitch_bend.begin(), pitch_bend.end(), 0);
    PitchBendDecode();
}

int PitchBendDecoder::GetDataFromUTAU64(char i) {
    if (i >= '0' && i <= '9') {
        return i - '0' + 52;
    } else if (i >= 'A' && i <= 'Z') {
        return i - 'A';
    } else if (i >= 'a' && i <= 'z') {
        return i - 'a' + 26;
    } else if (i == '+') {
        return 62;
    } else if (i == '/') {
        return 63;
    } else {
        return 0;
    }
}

void PitchBendDecoder::PitchBendDecode() {
    int i, n = 0;
    int k = 0, num, ii;
    std::stringstream ss;
    char *str = const_cast<char *>(pitch.c_str());
    if (str != nullptr) {
        for (i = 0; i < pitch_string_length; i += 2) {
            if (str[i] == '#') {
                i++;
                ss << pitch.substr(pitch.find('#', i - 1) + 1, pitch.find('#', i + pitch.find('#')) - 1);
                ss >> num;
                for (ii = 0; ii < num && k < count; ii++) {
                    pitch_bend[k++] = n;
                }
                while (str[i] != '#' && str[i] != 0)
                    i++;
                i--;
            } else {
                n = GetDataFromUTAU64(str[i]) * 64 + GetDataFromUTAU64(str[i + 1]);
                if (n > 2047)
                    n -= 4096;
                if (k < count) {
                    pitch_bend[k++] = n;
                }
            }
        }
    }
}

std::vector<int> PitchBendDecoder::GetPitchBend() {
    return pitch_bend;
}
