#include "TestSupport.h"

#include <cmath>
#include <filesystem>
#include <numbers>
#include <span>
#include <vector>

#include "Audio/WavIO.h"
#include "Config/Config.h"
#include "Vocoder/World/WorldAnalyzer.h"
#include "Vocoder/World/WorldModelBuilder.h"

using namespace lessampler;

namespace {

    void check_dimensions(const WorldModel &model, int expected_fs) {
        CHECK_EQ(model.fs, expected_fs);
        CHECK(!model.f0.empty());
        CHECK_EQ(model.spectrogram.rows(), model.f0.size());
        CHECK_EQ(model.aperiodicity.rows(), model.f0.size());
        CHECK_EQ(model.w_length, model.fft_size / 2 + 1);
        CHECK(model.fft_size > 0);
    }

    bool is_finite_model(const WorldModel &model) {
        bool finite = true;
        for (double value: model.f0) {
            finite = finite && std::isfinite(value);
        }
        for (std::size_t i = 0; i < model.spectrogram.rows(); ++i) {
            const auto row = model.spectrogram[i];
            CHECK_EQ(row.size(), static_cast<std::size_t>(model.w_length));
            for (double value: row) {
                finite = finite && std::isfinite(value);
            }
        }
        for (std::size_t i = 0; i < model.aperiodicity.rows(); ++i) {
            const auto row = model.aperiodicity[i];
            CHECK_EQ(row.size(), static_cast<std::size_t>(model.w_length));
            for (double value: row) {
                finite = finite && std::isfinite(value);
            }
        }
        return finite;
    }

    double average_voiced_f0(std::span<const double> f0) {
        double sum = 0.0;
        std::size_t count = 0;
        for (double value: f0) {
            if (value > 0.0) {
                sum += value;
                ++count;
            }
        }
        return count == 0 ? 0.0 : sum / static_cast<double>(count);
    }

}// namespace

int main() {
    const auto data_path = std::filesystem::path(LESSAMPLER_TEST_DATA_DIR) / "vaiueo2d.wav";
    const auto wav = WavIO::ReadWav(data_path);

    Config configure;
    const auto fixture_model = BuildWorldModel(wav.samples, wav.sample_rate, configure);

    check_dimensions(fixture_model, wav.sample_rate);
    CHECK(is_finite_model(fixture_model));
    CHECK(average_voiced_f0(fixture_model.f0) > 0.0);

    // The fixed D4C chunk partition keeps analysis reproducible in-process.
    const auto repeated = BuildWorldModel(wav.samples, wav.sample_rate, configure);
    CHECK(repeated.f0 == fixture_model.f0);
    CHECK(repeated.spectrogram.values() == fixture_model.spectrogram.values());
    CHECK(repeated.aperiodicity.values() == fixture_model.aperiodicity.values());

    // Worker count only changes concurrency, not the fixed D4C partition.
    const auto single_threaded = BuildWorldModel(wav.samples, wav.sample_rate, configure, 1);
    CHECK(single_threaded.f0 == fixture_model.f0);
    CHECK(single_threaded.spectrogram.values() == fixture_model.spectrogram.values());
    CHECK(single_threaded.aperiodicity.values() == fixture_model.aperiodicity.values());

    constexpr int fs = 44100;
    constexpr double frequency = 440.0;
    std::vector<double> sine(44100);
    for (std::size_t i = 0; i < sine.size(); ++i) {
        const double phase = 2.0 * std::numbers::pi * frequency * static_cast<double>(i) / static_cast<double>(fs);
        sine[i] = 0.4 * std::sin(phase) + 0.2 * std::sin(2.0 * phase) + 0.1 * std::sin(3.0 * phase);
    }

    for (const auto mode: {Config::F0_MODE::F0_MODE_DIO,
                           Config::F0_MODE::F0_MODE_HARVEST}) {
        Config sine_config;
        sine_config.f0_mode = mode;
        sine_config.f0_harvest_floor = 71.0;
        sine_config.f0_dio_floor = 71.0;
        WorldAnalyzer module(sine, fs, sine_config);
        auto world = module.TakeModule();

        CHECK_EQ(world.fs, fs);
        CHECK_EQ(world.spectrogram.rows(), world.f0.size());
        CHECK_EQ(world.aperiodicity.rows(), world.f0.size());
        CHECK(!world.f0.empty());
        CHECK(world.fft_size > 0);
        CHECK_EQ(world.time_axis.size(), world.f0.size());
        CHECK_NEAR(average_voiced_f0(world.f0), frequency, 20.0);
    }

    return test::finish();
}
