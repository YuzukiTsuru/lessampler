#include "TestSupport.h"

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <numbers>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <world/dio.h>
#include <world/harvest.h>

#include "Config/Config.h"
#include "Core/Exception.h"
#include "Vocoder/Common/F0Estimator.h"
#include "Vocoder/Common/Resample.h"
#include "Vocoder/Common/RmvpeDecode.h"

using namespace lessampler;

namespace {

    std::vector<double> sine(int sample_rate, double frequency, double seconds) {
        const auto count = static_cast<std::size_t>(sample_rate * seconds);
        std::vector<double> samples(count);
        for (std::size_t i = 0; i < count; ++i) {
            const double phase = 2.0 * std::numbers::pi * frequency *
                                 static_cast<double>(i) / static_cast<double>(sample_rate);
            samples[i] = 0.4 * std::sin(phase) + 0.2 * std::sin(2.0 * phase) + 0.1 * std::sin(3.0 * phase);
        }
        return samples;
    }

    double average_voiced_f0(const F0Track &track) {
        double sum = 0.0;
        std::size_t count = 0;
        for (double value: track.f0) {
            if (value > 0.0) {
                sum += value;
                ++count;
            }
        }
        return count == 0 ? 0.0 : sum / static_cast<double>(count);
    }

    bool contains_name(const std::string &names, std::string_view name) {
        return names.find(name) != std::string::npos;
    }

}// namespace

int main() {
    CHECK_EQ(F0EstimatorKindName(F0EstimatorKind::auto_select), "auto");
    CHECK_EQ(F0EstimatorKindName(F0EstimatorKind::harvest), "harvest");
    CHECK_EQ(F0EstimatorKindName(F0EstimatorKind::dio), "dio");
    CHECK_EQ(F0EstimatorKindName(F0EstimatorKind::llsm), "llsm");
    CHECK_EQ(F0EstimatorKindName(F0EstimatorKind::neural), "neural");

    CHECK(ParseF0EstimatorKind("AUTO") == F0EstimatorKind::auto_select);
    CHECK(ParseF0EstimatorKind("Harvest") == F0EstimatorKind::harvest);
    CHECK(ParseF0EstimatorKind("DIO") == F0EstimatorKind::dio);
    CHECK(ParseF0EstimatorKind("LLSM") == F0EstimatorKind::llsm);
    CHECK(ParseF0EstimatorKind("Neural") == F0EstimatorKind::neural);
    CHECK(!ParseF0EstimatorKind("unknown").has_value());

    CHECK(F0EstimatorCompiled(F0EstimatorKind::harvest));
    CHECK(F0EstimatorCompiled(F0EstimatorKind::dio));
    CHECK(F0EstimatorCompiled(F0EstimatorKind::auto_select));
    const bool llsm_available = F0EstimatorCompiled(F0EstimatorKind::llsm);
    const bool neural_available = F0EstimatorCompiled(F0EstimatorKind::neural);

    const auto available = AvailableF0Estimators();
    CHECK(contains_name(available, "harvest"));
    CHECK(contains_name(available, "dio"));
    CHECK_EQ(contains_name(available, "llsm"), llsm_available);
    CHECK_EQ(contains_name(available, "neural"), neural_available);

    CHECK(DefaultF0EstimatorFor(VocoderKind::world) == F0EstimatorKind::harvest);
    CHECK(DefaultF0EstimatorFor(VocoderKind::llsm) == F0EstimatorKind::llsm);
    CHECK(DefaultF0EstimatorFor(VocoderKind::neural) == F0EstimatorKind::neural);

    const auto root = std::filesystem::temp_directory_path() / "lessampler-f0-estimator-test";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);
    const auto model_path = root / "rmvpe.onnx";
    {
        std::ofstream model(model_path);
        model << "not-a-real-onnx-model";
    }

    Config base;
    CHECK_EQ(ResolveF0EstimatorKind(base, VocoderKind::world), F0EstimatorKind::harvest);
    CHECK_EQ(ResolveF0EstimatorKind(base, VocoderKind::llsm),
             llsm_available ? F0EstimatorKind::llsm : F0EstimatorKind::harvest);

    Config neural_config;
    neural_config.f0_neural_model_path = model_path.string();
    CHECK_EQ(ResolveF0EstimatorKind(neural_config, VocoderKind::neural),
             neural_available ? F0EstimatorKind::neural : F0EstimatorKind::harvest);
    neural_config.f0_neural_model_path = (root / "missing.onnx").string();
    CHECK_EQ(ResolveF0EstimatorKind(neural_config, VocoderKind::neural), F0EstimatorKind::harvest);

    {
        bool rejected = false;
        try {
            (void) CreateF0Estimator(F0EstimatorKind::llsm, base);
        } catch (const parameter_error &) {
            rejected = true;
        }
        CHECK_EQ(rejected, !llsm_available);
    }
    {
        bool rejected = false;
        try {
            (void) CreateF0Estimator(F0EstimatorKind::neural, base);
        } catch (const parameter_error &) {
            rejected = true;
        }
        CHECK(rejected);
    }

    constexpr int fs = 44100;
    constexpr double frequency = 440.0;
    const auto samples = sine(fs, frequency, 1.0);

    for (const auto kind: {F0EstimatorKind::harvest, F0EstimatorKind::dio}) {
        Config config;
        config.f0_estimator = kind;
        config.f0_harvest_floor = 71.0;
        config.f0_dio_floor = 71.0;

        auto estimator = CreateF0Estimator(kind, config);
        CHECK_EQ(estimator->name(), F0EstimatorKindName(kind));
        const auto track = estimator->Estimate(samples, fs, config);
        CHECK_EQ(track.f0.size(), track.time_axis.size());
        CHECK(!track.f0.empty());
        CHECK_NEAR(average_voiced_f0(track), frequency, 20.0);
        if (kind == F0EstimatorKind::harvest) {
            CHECK_EQ(track.f0.size(), static_cast<std::size_t>(
                                              GetSamplesForHarvest(fs, static_cast<int>(samples.size()),
                                                                   config.audio_model_frame_period)));
        } else {
            CHECK_EQ(track.f0.size(), static_cast<std::size_t>(
                                              GetSamplesForDIO(fs, static_cast<int>(samples.size()),
                                                               config.audio_model_frame_period)));
        }

        const auto repeated = estimator->Estimate(samples, fs, config);
        CHECK(repeated.f0 == track.f0);
        CHECK(repeated.time_axis == track.time_axis);
    }

    if (llsm_available) {
        Config llsm_config;
        llsm_config.f0_estimator = F0EstimatorKind::llsm;
        llsm_config.f0_harvest_floor = 71.0;

        Config harvest_config_for_compare;
        harvest_config_for_compare.f0_estimator = F0EstimatorKind::harvest;
        harvest_config_for_compare.f0_harvest_floor = 71.0;
        const auto harvest_track = CreateF0Estimator(F0EstimatorKind::harvest, harvest_config_for_compare)
                                           ->Estimate(samples, fs, harvest_config_for_compare);

        auto llsm_estimator = CreateF0Estimator(F0EstimatorKind::llsm, llsm_config);
        CHECK_EQ(llsm_estimator->name(), "llsm");
        const auto llsm_track = llsm_estimator->Estimate(samples, fs, llsm_config);
        CHECK_EQ(llsm_track.f0.size(), harvest_track.f0.size());
        CHECK_EQ(llsm_track.time_axis.size(), llsm_track.f0.size());
        CHECK_NEAR(average_voiced_f0(llsm_track), frequency, 20.0);
        for (std::size_t i = 0; i < llsm_track.f0.size(); ++i) {
            if (harvest_track.f0[i] == 0.0) {
                CHECK_EQ(llsm_track.f0[i], 0.0);
            }
        }
        const auto llsm_repeated = llsm_estimator->Estimate(samples, fs, llsm_config);
        CHECK(llsm_repeated.f0 == llsm_track.f0);
    }

    {
        F0Track constant;
        constant.frame_period = 10.0;
        constant.f0.assign(2, 100.0);
        constant.time_axis = {0.0, 0.01};
        const auto resampled = ResampleF0Track(constant, 1600, 16000, 5.0);
        CHECK_EQ(resampled.f0.size(), static_cast<std::size_t>(1000 * 1600 / 16000 / 5 + 1));
        CHECK_EQ(resampled.f0.size(), resampled.time_axis.size());
        for (double value: resampled.f0) {
            CHECK_NEAR(value, 100.0, 1e-12);
        }

        F0Track ramp;
        ramp.frame_period = 10.0;
        ramp.f0 = {100.0, 200.0};
        ramp.time_axis = {0.0, 0.01};
        const auto midpoint = ResampleF0Track(ramp, 160, 16000, 5.0);
        REQUIRE(midpoint.f0.size() >= 3);
        CHECK_NEAR(midpoint.f0[0], 100.0, 1e-12);
        CHECK_NEAR(midpoint.f0[1], 150.0, 1e-12);
        CHECK_NEAR(midpoint.f0[2], 200.0, 1e-12);

        F0Track mixed;
        mixed.frame_period = 10.0;
        mixed.f0 = {100.0, 0.0};
        mixed.time_axis = {0.0, 0.01};
        const auto boundary = ResampleF0Track(mixed, 1600, 16000, 10.0);
        REQUIRE(!boundary.f0.empty());
        CHECK_NEAR(boundary.f0.front(), 100.0, 1e-12);
        CHECK_NEAR(boundary.f0.back(), 0.0, 1e-12);
    }

    {
        RmvpeOutput salience;
        salience.frames = 1;
        salience.bins = 360;
        salience.salience.assign(360, 0.0f);
        salience.salience[228] = 1.0f;
        Config decode_config;
        const auto decoded = DecodeRmvpeOutput(salience, decode_config);
        REQUIRE(decoded.f0.size() == static_cast<std::size_t>(1));
        CHECK_NEAR(decoded.f0[0], 440.0, 20.0);
        CHECK_NEAR(decoded.time_axis[0], 0.0, 1e-12);

        salience.salience[228] = 0.01f;
        CHECK_EQ(DecodeRmvpeOutput(salience, decode_config).f0[0], 0.0);

        RmvpeOutput direct;
        direct.direct_f0 = true;
        direct.frames = 2;
        direct.f0 = {440.0f, 0.0f};
        direct.uv = {0, 1};
        const auto direct_decoded = DecodeRmvpeOutput(direct, decode_config);
        REQUIRE(direct_decoded.f0.size() == static_cast<std::size_t>(2));
        CHECK_NEAR(direct_decoded.f0[0], 440.0, 1e-9);
        CHECK_EQ(direct_decoded.f0[1], 0.0);
    }

    if (const char *model = std::getenv("LESSAMPLER_RMVPE_MODEL"); model != nullptr && *model != '\0') {
        Config neural;
        neural.f0_estimator = F0EstimatorKind::neural;
        neural.f0_neural_model_path = model;
        auto estimator = CreateF0Estimator(F0EstimatorKind::neural, neural);
        const auto track = estimator->Estimate(samples, fs, neural);
        CHECK_EQ(track.f0.size(), track.time_axis.size());
        CHECK(!track.f0.empty());
        CHECK_NEAR(average_voiced_f0(track), frequency, 30.0);
    }

    Config harvest_config;
    harvest_config.f0_estimator = F0EstimatorKind::harvest;
    const auto harvest_fingerprint = harvest_config.fingerprint();
    harvest_config.f0_estimator = F0EstimatorKind::dio;
    CHECK(harvest_config.fingerprint() != harvest_fingerprint);

    Config explicit_harvest;
    explicit_harvest.f0_estimator = F0EstimatorKind::harvest;
    const auto explicit_before = explicit_harvest.fingerprint();
    explicit_harvest.vocoder = VocoderKind::llsm;
    CHECK_EQ(explicit_harvest.fingerprint(), explicit_before);

    Config auto_config;
    const auto auto_before = auto_config.fingerprint();
    auto_config.vocoder = VocoderKind::llsm;
    CHECK(auto_config.fingerprint() != auto_before);
    CHECK_EQ(ResolveF0EstimatorConfig(Config{}, VocoderKind::world).fingerprint(), explicit_before);

    Config neural_fingerprint;
    neural_fingerprint.f0_estimator = F0EstimatorKind::neural;
    neural_fingerprint.f0_neural_model_path = model_path.string();
    const auto neural_before = neural_fingerprint.fingerprint();
    neural_fingerprint.f0_neural_threshold = 0.5;
    CHECK(neural_fingerprint.fingerprint() != neural_before);
    neural_fingerprint.f0_neural_threshold = 0.03;
    {
        std::ofstream model(model_path, std::ios::app);
        model << "-changed";
    }
    CHECK(neural_fingerprint.fingerprint() != neural_before);

    std::filesystem::remove_all(root);
    return test::finish();
}
