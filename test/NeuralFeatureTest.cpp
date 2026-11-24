#include "TestSupport.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <vector>

#include "Vocoder/Neural/MelSpectrogram.h"

using namespace lessampler;

namespace {

    constexpr double pi = std::numbers::pi;

    std::vector<double> sine(int sample_rate, double frequency, double seconds) {
        const auto count = static_cast<std::size_t>(sample_rate * seconds);
        std::vector<double> samples(count);
        for (std::size_t i = 0; i < count; ++i) {
            samples[i] = 0.5 * std::sin(2.0 * pi * frequency * static_cast<double>(i) / sample_rate);
        }
        return samples;
    }

    double pitch_of(const std::vector<double> &samples, int sample_rate) {
        const std::size_t start = samples.size() / 4;
        const std::size_t length = samples.size() / 2;
        const int min_lag = std::max(2, sample_rate / 1000);
        const int max_lag = std::min(sample_rate / 80, static_cast<int>(length) - 1);
        double best_score = 0.0;
        int best_lag = 0;
        for (int lag = min_lag; lag <= max_lag; ++lag) {
            double score = 0.0;
            for (std::size_t i = 0; i + static_cast<std::size_t>(lag) < length; ++i) {
                score += samples[start + i] * samples[start + i + static_cast<std::size_t>(lag)];
            }
            if (score > best_score) {
                best_score = score;
                best_lag = lag;
            }
        }
        return best_lag > 0 ? static_cast<double>(sample_rate) / best_lag : 0.0;
    }

}// namespace

int main() {
    const auto source = sine(44100, 440.0, 1.0);
    const auto same = MelSpectrogram::Resample(source, 44100, 44100);
    CHECK_EQ(same.size(), source.size());
    CHECK(same == source);

    const auto half = sine(22050, 440.0, 1.0);
    const auto upsampled = MelSpectrogram::Resample(half, 22050, 44100);
    CHECK_EQ(upsampled.size(), half.size() * 2);
    CHECK_NEAR(pitch_of(upsampled, 44100), 440.0, 2.0);
    const auto downsampled = MelSpectrogram::Resample(upsampled, 44100, 22050);
    CHECK_EQ(downsampled.size(), half.size());
    CHECK_NEAR(pitch_of(downsampled, 22050), 440.0, 2.0);

    MelSpectrogram extractor;
    const auto features = extractor.Extract(source, 44100, 0.111);
    CHECK_EQ(features.size(), static_cast<std::size_t>(172) * MelSpectrogram::mel_bins);
    double maximum = -1e30;
    for (float value: features) {
        CHECK(std::isfinite(value));
        maximum = std::max(maximum, static_cast<double>(value));
    }
    CHECK(maximum > std::log(1e-5));

    const std::vector<double> silence(44100, 0.0);
    const auto silent_features = extractor.Extract(silence, 44100, 0.111);
    CHECK_EQ(silent_features.size(), static_cast<std::size_t>(172) * MelSpectrogram::mel_bins);
    for (float value: silent_features) {
        CHECK_NEAR(value, std::log(1e-5), 1e-5);
    }

    return test::finish();
}
