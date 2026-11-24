#include "TestSupport.h"

#include <algorithm>
#include <string>

#include "Host/UTAU/libUTAU/PitchBendDecoder.h"

using namespace lessampler;

int main() {
    const std::string pitch = R"(1E#40#1F1H1J1N1R1W1b1i1p1w152C2M2W2h2s243F3S3g3t384K4Z4p445I5Y5o556J6a6q677L7b7r778L8"
                               "b8q859I9W9k9y9/+K+V+g+q+0+8/F/N/V/b/h/n/s/w/1/4/7/9//ABADAFAHAIAKALAMAMAN#3#AMAMALAKAJ"
                               "AIAHAGAFAEADACACABAA#1#)";

    PitchBendDecoder decoder(pitch, 154);
    const auto values = decoder.GetPitchBend();
    REQUIRE(values.size() == 154);

    for (std::size_t i = 0; i < 41; ++i) {
        CHECK_EQ(values[i], -700);
    }
    CHECK_EQ(values[41], -699);
    CHECK_EQ(values[42], -697);

    PitchBendDecoder repeat("1E#4#", 5);
    const auto repeated = repeat.GetPitchBend();
    REQUIRE(repeated.size() == 5);
    for (int value: repeated) {
        CHECK_EQ(value, -700);
    }

    PitchBendDecoder empty("", 5);
    for (int value: empty.GetPitchBend()) {
        CHECK_EQ(value, 0);
    }

    PitchBendDecoder truncated("1", 4);
    CHECK_EQ(truncated.GetPitchBend().size(), 4);

    return test::finish();
}
