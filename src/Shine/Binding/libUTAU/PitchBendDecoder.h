/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_PITCHBENDDECODER_H
#define LESSAMPLER_PITCHBENDDECODER_H

#include <iostream>
#include <vector>

class PitchBendDecoder {
public:
    explicit PitchBendDecoder(std::string &str, int count);

    std::vector<int> GetPitchBend();

private:
    std::string pitch = {};
    int count = 0, pitch_string_length = 0;
    std::vector<int> pitch_bend{};

private:
    // Decode Splited UTAU String
    static int GetDataFromUTAU64(char i);

private:
    // Decode the Pitch Bend
    void PitchBendDecode();
};


#endif //LESSAMPLER_PITCHBENDDECODER_H
