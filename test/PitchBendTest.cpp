//
// Created by YuzukiTsuru on 2021/4/17.
//

#include <iostream>
#include <string>
#include "Shine/Binding/libUTAU/PitchBendDecoder.h"

int main() {
    std::string pitch = R"(1E#40#1F1H1J1N1R1W1b1i1p1w152C2M2W2h2s243F3S3g3t384K4Z4p445I5Y5o556J6a6q677L7b7r778L8"
                         "b8q859I9W9k9y9/+K+V+g+q+0+8/F/N/V/b/h/n/s/w/1/4/7/9//ABADAFAHAIAKALAMAMAN#3#AMAMALAKAJ"
                         "AIAHAGAFAEADACACABAA#1#)";
    std::string decode_pitch = "-700-700-700-700-700-700-700-700-700-700-700-700-700-700-700-700-700-700-700-700-700-700-700-700-700-700-700-700-700-700"
                               "-700-700-700-700-700-700-700-700-700-700-700-699-697-695-691-687-682-677-670-663-656-647-638-628-618-607-596-584-571-558"
                               "-544-531-516-502-487-471-456-440-424-408-391-375-358-342-325-309-293-277-261-245-256000000000000027-214-199-184-170-156-"
                               "142-129-118-107-96-86-76-68-59-51-43-37-31-25-20-16-11-8-5-3-1135781011121213121211109000000000000008765";

    PitchBendDecoder pitchBendDecoder(pitch, 154);

    std::string check_data = {};
    for (int i = 0; i < 154; i++) {
        check_data.append(std::to_string(pitchBendDecoder.GetPitchBend()[i]));
    }
    if (check_data == decode_pitch) {
        std::cout << "Pitch Check OK" << std::endl;
        return 0;
    } else {
        std::cout << "Pitch Check Error" << std::endl;
        std::cout << check_data << "\n" << decode_pitch;
        return -1;
    }
}