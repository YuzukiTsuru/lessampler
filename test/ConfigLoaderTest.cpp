#include "TestSupport.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

#include "Config/ConfigLoader.h"
#include "Config/ConfigResolver.h"
#include "Config/VoicebankConfig.h"
#include "Core/Exception.h"

using namespace lessampler;

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
    CHECK(std::filesystem::exists(defaults_dir / "lessampler.less"));
    CHECK(!config.custom_fft_size);
    CHECK_EQ(config.vocoder, VocoderKind::world);
    CHECK_EQ(config.f0_estimator, F0EstimatorKind::auto_select);
    CHECK(config.audio_model_frame_period > 0.0);
    CHECK(config.neural_model_path.empty());
    CHECK_EQ(config.neural_threads, 0);
    CHECK_NEAR(config.neural_target_rms, 0.111, 1e-12);
    CHECK_EQ(config.style_default, VoiceStyleKind::natural);
    CHECK_NEAR(config.style_amount, 1.0, 1e-12);
    CHECK_NEAR(config.style_morph_ms, 0.0, 1e-12);
    {
        std::ifstream default_config(defaults_dir / "lessampler.less");
        const std::string content((std::istreambuf_iterator<char>(default_config)),
                                  std::istreambuf_iterator<char>());
        CHECK(content.find("backend = \"world\"") != std::string::npos);
        CHECK(content.find("estimator = \"auto\"") != std::string::npos);
        CHECK(content.find("f0_mode") == std::string::npos);
        CHECK(content.find("[neural]") != std::string::npos);
        CHECK(content.find("[style]") != std::string::npos);
    }

    const auto custom_dir = root / "custom";
    std::filesystem::create_directories(custom_dir);
    write_file(custom_dir / "lessampler.less", R"([config]
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

[neural]
model_path = "model.onnx"
threads = 2
target_rms = 0.1

[style]
default = "clear"
amount = 0.5
morph_ms = 120.0
)");
    ConfigLoader custom(custom_dir);
    config = custom.GetConfig();
    CHECK(config.debug_mode);
    CHECK_EQ(config.vocoder, VocoderKind::llsm);
    CHECK(config.custom_fft_size);
    CHECK_EQ(config.fft_size, 2048);
    CHECK_NEAR(config.model_amp, 0.0, 1e-12);
    CHECK_EQ(config.f0_estimator, F0EstimatorKind::dio);
    CHECK_EQ(config.f0_speed, 2);
    CHECK_NEAR(config.ap_threshold, 0.2, 1e-12);
    CHECK_EQ(config.neural_model_path, (custom_dir / "model.onnx").string());
    CHECK_EQ(config.neural_threads, 2);
    CHECK_NEAR(config.neural_target_rms, 0.1, 1e-12);
    CHECK_EQ(config.style_default, VoiceStyleKind::clear);
    CHECK_NEAR(config.style_amount, 0.5, 1e-12);
    CHECK_NEAR(config.style_morph_ms, 120.0, 1e-12);

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
    CHECK(std::filesystem::exists(legacy_dir / "lessampler.less"));
    CHECK(std::filesystem::exists(legacy_dir / "lessampler.fex.bak"));
    CHECK(config.debug_mode);
    CHECK_EQ(config.vocoder, VocoderKind::world);
    CHECK_NEAR(config.audio_model_frame_period, 5.5, 1e-12);
    CHECK_NEAR(config.model_amp, 0.8, 1e-12);
    CHECK_EQ(config.f0_speed, 2);
    CHECK_EQ(config.f0_estimator, F0EstimatorKind::harvest);

    const auto legacy_toml_dir = root / "legacy-toml";
    std::filesystem::create_directories(legacy_toml_dir);
    write_file(legacy_toml_dir / "lessampler.fex", R"([config]
version = "old-toml"
debug = true

[audio_model]
backend = "world"
frame_period = 5.7
)");
    ConfigLoader legacy_toml(legacy_toml_dir);
    config = legacy_toml.GetConfig();
    CHECK(std::filesystem::exists(legacy_toml_dir / "lessampler.less"));
    CHECK(std::filesystem::exists(legacy_toml_dir / "lessampler.fex.bak"));
    CHECK(config.debug_mode);
    CHECK_NEAR(config.audio_model_frame_period, 5.7, 1e-12);

    const auto expect_invalid = [&](const std::string &name, const std::string &content) {
        const auto dir = root / name;
        std::filesystem::create_directories(dir);
        write_file(dir / "lessampler.less", content);
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
    expect_invalid("bad-threads", "[neural]\nthreads = -1\n");
    expect_invalid("bad-rms", "[neural]\ntarget_rms = 0.0\n");
    expect_invalid("bad-style", "[style]\ndefault = \"NOPE\"\n");
    expect_invalid("bad-style-amount", "[style]\namount = 2.0\n");
    expect_invalid("bad-style-morph", "[style]\nmorph_ms = -1.0\n");

    const auto voice_dir = root / "voice";
    std::filesystem::create_directories(voice_dir);
    write_file(voice_dir / "lessvoice.less", R"([audio_model]
frame_period = 6.0

[f0]
f0_mode = "DIO"
neural_model_path = "rmvpe.onnx"
)");
    Config base;
    VoicebankConfig voice_bank(voice_dir / "missing.wav", base);
    const auto voice_config = voice_bank.GetConfig();
    CHECK_NEAR(voice_config.audio_model_frame_period, 6.0, 1e-12);
    CHECK_EQ(voice_config.f0_estimator, F0EstimatorKind::dio);
    CHECK_EQ(voice_config.f0_neural_model_path, (voice_dir / "rmvpe.onnx").string());

    ConfigResolver resolver(base);
    const auto resolved = resolver.Resolve(voice_dir / "a.wav");
    CHECK_NEAR(resolved.audio_model_frame_period, 6.0, 1e-12);
    CHECK_EQ(resolved.f0_estimator, F0EstimatorKind::dio);

    // A voicebank config is parsed once per session.
    write_file(voice_dir / "lessvoice.less", R"([audio_model]
frame_period = 7.0
)");
    CHECK_NEAR(resolver.Resolve(voice_dir / "b.wav").audio_model_frame_period, 6.0, 1e-12);
    ConfigResolver fresh_resolver(base);
    CHECK_NEAR(fresh_resolver.Resolve(voice_dir / "b.wav").audio_model_frame_period, 7.0, 1e-12);

    const auto legacy_voice_dir = root / "voice-legacy";
    std::filesystem::create_directories(legacy_voice_dir);
    write_file(legacy_voice_dir / "lessvoice.fex", R"([audio_model]
frame_period = 6.5
)");
    ConfigResolver legacy_voice_resolver(base);
    CHECK_NEAR(legacy_voice_resolver.Resolve(legacy_voice_dir / "a.wav").audio_model_frame_period, 6.5, 1e-12);

    const auto neural_dir = root / "f0-neural";
    std::filesystem::create_directories(neural_dir);
    write_file(neural_dir / "rmvpe.onnx", "model");
    write_file(neural_dir / "lessampler.less", R"([f0]
estimator = "NEURAL"
f0_mode = "NOPE"
neural_model_path = "rmvpe.onnx"
neural_threads = 2
neural_threshold = 0.35
neural_floor = 50.0
neural_ceil = 1000.0

[neural]
preset = "PC-NSF-HiFiGAN"

[master]
preset = "POWER"
)");
    ConfigLoader neural_config(neural_dir);
    config = neural_config.GetConfig();
    CHECK_EQ(config.f0_estimator, F0EstimatorKind::neural);
    CHECK_EQ(config.f0_neural_model_path, (neural_dir / "rmvpe.onnx").string());
    CHECK_EQ(config.f0_neural_threads, 2);
    CHECK_NEAR(config.f0_neural_threshold, 0.35, 1e-12);
    CHECK_NEAR(config.f0_neural_floor, 50.0, 1e-12);
    CHECK_NEAR(config.f0_neural_ceil, 1000.0, 1e-12);
    CHECK_EQ(config.neural_spec.preset, "pc-nsf-hifigan");
    CHECK_EQ(config.master_preset, "power");

    expect_invalid("bad-estimator", "[f0]\nestimator = \"NOPE\"\n");
    expect_invalid("bad-neural-threshold", "[f0]\nneural_threshold = 2.0\n");
    expect_invalid("bad-neural-floor", "[f0]\nneural_floor = 0.0\n");
    expect_invalid("bad-neural-ceil", "[f0]\nneural_floor = 100.0\nneural_ceil = 50.0\n");

    Config fingerprint_config;
    fingerprint_config.f0_estimator = F0EstimatorKind::harvest;
    const auto before = fingerprint_config.fingerprint();
    CHECK(!before.empty());
    CHECK_EQ(fingerprint_config.fingerprint(), before);
    fingerprint_config.vocoder = VocoderKind::llsm;
    CHECK_EQ(fingerprint_config.fingerprint(), before);
    fingerprint_config.neural_model_path = "some-model.onnx";
    CHECK_EQ(fingerprint_config.fingerprint(), before);
    fingerprint_config.master_preset = "power";
    CHECK_EQ(fingerprint_config.fingerprint(), before);
    fingerprint_config.style_default = VoiceStyleKind::growl;
    fingerprint_config.style_amount = 0.5;
    fingerprint_config.style_morph_ms = 80.0;
    CHECK_EQ(fingerprint_config.fingerprint(), before);
    fingerprint_config.f0_estimator = F0EstimatorKind::dio;
    const auto dio_before = fingerprint_config.fingerprint();
    CHECK(dio_before != before);

    Config auto_config;
    const auto auto_before = auto_config.fingerprint();
    auto_config.vocoder = VocoderKind::llsm;
    CHECK(auto_config.fingerprint() != auto_before);

    Config neural_fingerprint;
    neural_fingerprint.f0_estimator = F0EstimatorKind::neural;
    neural_fingerprint.f0_neural_model_path = (neural_dir / "rmvpe.onnx").string();
    CHECK(std::filesystem::exists(neural_fingerprint.f0_neural_model_path));
    const auto neural_before = neural_fingerprint.fingerprint();
    neural_fingerprint.f0_neural_threshold = 0.5;
    CHECK(neural_fingerprint.fingerprint() != neural_before);

    fingerprint_config.custom_fft_size = true;
    fingerprint_config.fft_size = 2048;
    CHECK(fingerprint_config.fingerprint() != dio_before);

    std::filesystem::remove_all(root);
    return test::finish();
}
