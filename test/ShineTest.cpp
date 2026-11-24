#include "TestSupport.h"

#include <array>
#include <string_view>

#include "Core/AudioInfo.h"
#include "Host/UTAU/Shine.h"

using namespace lessampler;

int main() {
    const AudioInfo info{44100, 44100, 5.0};

    std::array<std::string_view, 14> args{
            "lessampler", "in.wav", "out.wav", "A4", "100", "", "0", "1000",
            "100", "0", "100", "0", "!120", ""};

    Shine shine(args, info, Shine::SHINE_MODE::UTAU);
    const auto para = shine.GetShine();
    CHECK_NEAR(para.scale_num, 440.0, 1e-9);
    CHECK_EQ(para.output_samples, 44101);
    CHECK_EQ(para.pitch_step, 230);
    CHECK_EQ(para.pitch_length, 192);
    CHECK_EQ(para.pitch_bend.size(), static_cast<std::size_t>(para.pitch_length + 1));
    CHECK(para.is_custom_pitch);
    for (int value: para.pitch_bend) {
        CHECK_EQ(value, 0);
    }
    CHECK_EQ(para.required_frame, 201);

    args[13] = "1E#4#";
    Shine bent(args, info, Shine::SHINE_MODE::UTAU);
    const auto bent_para = bent.GetShine();
    REQUIRE(bent_para.pitch_bend.size() >= 5);
    for (std::size_t i = 0; i < 5; ++i) {
        CHECK_EQ(bent_para.pitch_bend[i], -700);
    }
    for (std::size_t i = 5; i < bent_para.pitch_bend.size(); ++i) {
        CHECK_EQ(bent_para.pitch_bend[i], 0);
    }

    return test::finish();
}
