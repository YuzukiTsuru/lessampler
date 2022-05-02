/*
 * Copyright (c)  2022, YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <sstream>
#include "PitchBendDecoder.h"

PitchBendDecoder::PitchBendDecoder(std::string &str, int count) : pitch(std::move(str)), count(count) {
    pitch_length = static_cast<int>(pitch.length());
    dst = new int[count];
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
        for (i = 0; i < pitch_length; i += 2) {
            if (str[i] == '#') {
                i++;
                ss << pitch.substr(pitch.find('#', i - 1) + 1, pitch.find('#', i + pitch.find('#')) - 1);
                ss >> num;
                for (ii = 0; ii < num && k < count; ii++) {
                    dst[k++] = n;
                }
                while (str[i] != '#' && str[i] != 0)
                    i++;
                i--;
            } else {
                n = GetDataFromUTAU64(str[i]) * 64 + GetDataFromUTAU64(str[i + 1]);
                if (n > 2047)
                    n -= 4096;
                if (k < count) {
                    dst[k++] = n;
                }
            }
        }
    }
}
