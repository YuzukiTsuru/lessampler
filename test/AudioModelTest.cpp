#include "TestSupport.h"

#include <cmath>
#include <numbers>
#include <vector>

#include "Analysis/World/WorldModelBuilder.h"
#include "Config/Config.h"

int main() {
    constexpr int fs = 44100;
    constexpr double frequency = 440.0;
    constexpr std::size_t sample_count = 44100;

    std::vector<double> samples(sample_count);
    for (std::size_t i = 0; i < samples.size(); ++i) {
        samples[i] = 0.5 * std::sin(2.0 * std::numbers::pi * frequency * static_cast<double>(i) / static_cast<double>(fs));
    }

    Config configure;
    configure.f0_mode = Config::F0_MODE::F0_MODE_DIO;
    const auto model = BuildWorldModel(samples, fs, configure);

    CHECK_EQ(model.fs, fs);
    CHECK_EQ(model.x_length, samples.size());
    CHECK_NEAR(model.frame_period, configure.audio_model_frame_period, 1e-12);
    CHECK(model.fft_size > 0);
    CHECK_EQ(model.w_length, model.fft_size / 2 + 1);
    CHECK(!model.f0.empty());
    CHECK_EQ(model.spectrogram.rows(), model.f0.size());
    CHECK_EQ(model.aperiodicity.rows(), model.f0.size());

    bool finite = true;
    double f0_sum = 0.0;
    std::size_t voiced = 0;
    for (double value: model.f0) {
        finite = finite && std::isfinite(value);
        if (value > 0.0) {
            f0_sum += value;
            ++voiced;
        }
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

    CHECK(finite);
    CHECK(voiced > 0);
    if (voiced > 0) {
        CHECK_NEAR(f0_sum / static_cast<double>(voiced), frequency, 20.0);
    }

    return test::finish();
}
