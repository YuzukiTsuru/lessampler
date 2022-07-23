//
// Created by YuzukiTsuru on 2021/4/17.
//

#include <iostream>
#include "Shine/Binding/libUTAU/PitchBendDecoder.h"

int main() {
    std::string pitch = R"(1E#40#1F1H1J1N1R1W1b1i1p1w152C2M2W2h2s243F3S3g3t384K4Z4p445I5Y5o556J6a6q677L7b7r778L8b8q859I9W9k9y9/+K+V+g+q+0+8/F/N/V/b/h/n/s/w/1/4/7/9//ABADAFAHAIAKALAMAMAN#3#AMAMALAKAJAIAHAGAFAEADACACABAA#1#)";
    PitchBendDecoder pitchBendDecoder(pitch, 154);
    for (int i = 0; i < 154; i++) {
        std::cout << pitchBendDecoder.GetPitchBend()[i] << " ";
    }
    std::cout << std::endl;
    return 0;
}