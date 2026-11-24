#include "TestSupport.h"

#include <array>
#include <string_view>

#include "Core/AudioInfo.h"
#include "Host/UTAU/libUTAU/libUTAU.h"

using namespace lessampler;

int main() {
    const std::array<std::string_view, 14> args{
            "lessampler", "in.wav", "out.wav", "A4", "100", "", "0", "1000",
            "100", "0", "100", "0", "!120", ""};

    libUTAU utau(args);
    const auto para = utau.GetUTAUPara();
    CHECK_EQ(para.input_file_name, "in.wav");
    CHECK_EQ(para.output_file_name, "out.wav");
    CHECK_NEAR(para.scale_num, 440.0, 1e-9);
    CHECK_NEAR(para.velocity, 1.0, 1e-9);
    CHECK_NEAR(para.volumes, 1.0, 1e-9);
    CHECK_EQ(para.tempo_num, 120);
    CHECK(para.is_custom_pitch);

    const AudioInfo info{44100, 44100, 5.0};
    utau.CheckPara(info);
    CHECK_NEAR(utau.GetUTAUPara().wave_length, 1000.0, 1e-9);
    CHECK_EQ(utau.GetUTAUPara().output_samples, 44101);

    const std::array<std::string_view, 10> oversized_fixed{
            "lessampler", "in.wav", "out.wav", "A4", "100", "", "0", "1000",
            "100000", "0"};
    libUTAU fixed(oversized_fixed);
    fixed.CheckPara(info);
    CHECK_NEAR(fixed.GetUTAUPara().first_half_fixed_part, 1000.0, 1e-9);

    return test::finish();
}
