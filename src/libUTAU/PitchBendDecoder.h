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

#ifndef LESSAMPLER_PITCHBENDDECODER_H
#define LESSAMPLER_PITCHBENDDECODER_H

#include <iostream>

class PitchBendDecoder {
public:
    explicit PitchBendDecoder(std::string &str, int count);

private:
    std::string pitch = {};
    int count = 0, pitch_length = 0;

    int *dst = nullptr;

    static int GetDataFromUTAU64(char i);
    void PitchBendDecode();
};


#endif //LESSAMPLER_PITCHBENDDECODER_H
