#include "TestSupport.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "Config/ConfigLoader.h"
#include "Config/VoicebankConfig.h"
#include "Core/Exception.h"

namespace {

    const std::filesystem::path root = std::filesystem::temp_directory_path() / "lessampler-config-test";

    void write_file(const std::filesystem::path &path, const std::string &content) {
        std::ofstream out(path);
        out << content;
    }

}// namespace

int main() {
    std::filesystem::remove_all(root);

    const auto defaults_dir = root / "defaults";
    std::filesystem::create_directories(defaults_dir);
    ConfigLoader defaults(defaults_dir);
    auto config = defaults.GetConfig();
    CHECK(std::filesystem::exists(defaults_dir / "lessampler.fex"));
    CHECK(!config.custom_fft_size);
    CHECK_EQ(config.vocoder, lessbackend::VocoderKind::world);
    CHECK_EQ(config.f0_mode, Config::F0_MODE::F0_MODE_HARVEST);
    CHECK(config.audio_model_frame_period > 0.0);
    {
        std::ifstream default_config(defaults_dir / "lessampler.fex");
        const std::string content((std::istreambuf_iterator<char>(default_config)),
                                  std::istreambuf_iterator<char>());
        CHECK(content.find("backend = \"world\"") != std::string::npos);
    }

    const auto custom_dir = root / "custom";
    std::filesystem::create_directories(custom_dir);
    write_file(custom_dir / "lessampler.fex", R"([config]
version = "test"
debug = true

[audio_model]
backend = "LLSM"
frame_period = 5.0
fft_size = 2048
model_amp = 0.0

[f0]
f0_mode = "DIO"
f0_speed = 2
f0_dio_floor = 50.0
f0_harvest_floor = 51.0
f0_cheap_trick_floor = 72.0
f0_allow_range = 0.2

[ap]
ap_threshold = 0.2
)");
    ConfigLoader custom(custom_dir);
    config = custom.GetConfig();
    CHECK(config.debug_mode);
    CHECK_EQ(config.vocoder, lessbackend::VocoderKind::llsm);
    CHECK(config.custom_fft_size);
    CHECK_EQ(config.fft_size, 2048);
    CHECK_NEAR(config.model_amp, 0.0, 1e-12);
    CHECK_EQ(config.f0_mode, Config::F0_MODE::F0_MODE_DIO);
    CHECK_EQ(config.f0_speed, 2);
    CHECK_NEAR(config.ap_threshold, 0.2, 1e-12);

    const auto legacy_dir = root / "legacy";
    std::filesystem::create_directories(legacy_dir);
    write_file(legacy_dir / "lessampler.fex", R"([config]
version="old"
debug=1

[audio_model]
frame_period=5.5
fft_size=auto
model_amp=0.8

[f0]
f0_mode=Harvest
f0_speed=2
f0_dio_floor=40.0
f0_harvest_floor=41.0
f0_cheap_trick_floor=71.0
f0_allow_range=0.1

[ap]
ap_threshold=0.1
)");
    ConfigLoader legacy(legacy_dir);
    config = legacy.GetConfig();
    CHECK(std::filesystem::exists(legacy_dir / "lessampler.fex.bak"));
    CHECK(config.debug_mode);
    CHECK_EQ(config.vocoder, lessbackend::VocoderKind::world);
    CHECK_NEAR(config.audio_model_frame_period, 5.5, 1e-12);
    CHECK_NEAR(config.model_amp, 0.8, 1e-12);
    CHECK_EQ(config.f0_speed, 2);
    CHECK_EQ(config.f0_mode, Config::F0_MODE::F0_MODE_HARVEST);

    const auto expect_invalid = [&](const std::string &name, const std::string &content) {
        const auto dir = root / name;
        std::filesystem::create_directories(dir);
        write_file(dir / "lessampler.fex", content);
        bool rejected = false;
        try {
            ConfigLoader unit(dir);
        } catch (const parameter_error &) {
            rejected = true;
        }
        CHECK(rejected);
    };

    expect_invalid("bad-mode", "[f0]\nf0_mode = \"NOPE\"\n");
    expect_invalid("bad-backend", "[audio_model]\nbackend = \"nope\"\n");
    expect_invalid("bad-fft", "[audio_model]\nfft_size = -1\n");
    expect_invalid("bad-ap", "[ap]\nap_threshold = 2.0\n");

    const auto voice_dir = root / "voice";
    std::filesystem::create_directories(voice_dir);
    write_file(voice_dir / "lessvoice.fex", R"([audio_model]
frame_period = 6.0

[f0]
f0_mode = "DIO"
)");
    Config base;
    VoicebankConfig voice_bank(voice_dir / "missing.wav", base);
    const auto voice_config = voice_bank.GetConfig();
    CHECK_NEAR(voice_config.audio_model_frame_period, 6.0, 1e-12);
    CHECK_EQ(voice_config.f0_mode, Config::F0_MODE::F0_MODE_DIO);

    Config fingerprint_config;
    const auto before = fingerprint_config.fingerprint();
    CHECK(!before.empty());
    CHECK_EQ(fingerprint_config.fingerprint(), before);
    fingerprint_config.vocoder = lessbackend::VocoderKind::llsm;
    CHECK_EQ(fingerprint_config.fingerprint(), before);
    fingerprint_config.custom_fft_size = true;
    fingerprint_config.fft_size = 2048;
    CHECK(fingerprint_config.fingerprint() != before);

    std::filesystem::remove_all(root);
    return test::finish();
}
