#include "TestSupport.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <vector>

#include "Audio/Dsp/Fft.h"
#include "Audio/Preprocess.h"
#include "Core/PreprocessOptions.h"

using namespace lessampler;

namespace {

    std::vector<double> make_sine(int sample_rate, double frequency, double seconds, double amplitude = 0.4) {
        const auto count = static_cast<std::size_t>(std::llround(static_cast<double>(sample_rate) * seconds));
        std::vector<double> samples(count);
        for (std::size_t i = 0; i < count; ++i) {
            const double t = static_cast<double>(i) / static_cast<double>(sample_rate);
            samples[i] = amplitude * std::sin(2.0 * std::numbers::pi * frequency * t);
        }
        return samples;
    }

    double rms(std::span<const double> samples) {
        if (samples.empty()) {
            return 0.0;
        }
        double sum = 0.0;
        for (double value: samples) {
            sum += value * value;
        }
        return std::sqrt(sum / static_cast<double>(samples.size()));
    }

    bool all_finite(std::span<const double> samples) {
        return std::ranges::all_of(samples, [](double value) { return std::isfinite(value); });
    }

    std::vector<double> make_noise(std::size_t count, double amplitude) {
        std::vector<double> samples(count);
        std::uint32_t state = 0x12345678U;
        for (double &value: samples) {
            state = state * 1664525U + 1013904223U;
            const double unit = (static_cast<double>(state) / 4294967295.0) * 2.0 - 1.0;
            value = amplitude * unit;
        }
        return samples;
    }

}// namespace

int main() {
    constexpr int fs = 44100;

    {
        const auto input = make_sine(fs, 440.0, 0.25);
        PreprocessOptions options;
        CHECK(!options.enabled);
        const auto output = PreprocessAudio(input, fs, options);
        CHECK(output == input);
    }

    {
        auto input = make_sine(fs, 440.0, 0.5);
        for (double &value: input) {
            value += 0.35;
        }
        PreprocessOptions options;
        options.enabled = true;
        options.highpass_hz = 80.0;
        options.declick = false;
        options.denoise = false;
        const auto output = PreprocessAudio(input, fs, options);
        const auto middle = std::span<const double>(output).subspan(output.size() / 4, output.size() / 2);
        CHECK(std::abs(rms(middle)) > 0.2);
        double mean = 0.0;
        for (double value: middle) {
            mean += value;
        }
        mean /= static_cast<double>(middle.size());
        CHECK(std::abs(mean) < 0.01);
    }

    {
        const auto clean = make_sine(fs, 220.0, 0.5);
        auto input = clean;
        input[input.size() / 2] += 1.0;
        PreprocessOptions options;
        options.enabled = true;
        options.highpass_hz = 0.0;
        options.declick = true;
        options.declick_threshold = 8.0;
        options.denoise = false;
        const auto output = PreprocessAudio(input, fs, options);
        CHECK(all_finite(output));
        CHECK(std::abs(output[input.size() / 2] - clean[input.size() / 2]) < 0.25);
        CHECK(rms(output) > 0.2);
    }

    {
        const auto input = make_noise(fs * 2, 0.2);
        PreprocessOptions options;
        options.enabled = true;
        options.highpass_hz = 0.0;
        options.declick = false;
        options.denoise = true;
        options.denoise_db = 12.0;
        const auto output = PreprocessAudio(input, fs, options);
        CHECK(all_finite(output));
        const auto middle_in = std::span<const double>(input).subspan(input.size() / 4, input.size() / 2);
        const auto middle_out = std::span<const double>(output).subspan(output.size() / 4, output.size() / 2);
        CHECK(rms(middle_out) < 0.75 * rms(middle_in));
    }

    {
        auto input = make_noise(fs, 0.1);
        input[1000] = 1.0;
        PreprocessOptions options;
        options.enabled = true;
        options.highpass_hz = 70.0;
        options.declick = true;
        options.denoise = true;
        const auto first = PreprocessAudio(input, fs, options);
        const auto second = PreprocessAudio(input, fs, options);
        CHECK(first == second);
        CHECK(all_finite(first));
        CHECK(std::abs(first[1000]) < 0.6);
    }

    {
        const auto input = make_sine(fs, 440.0, 0.01);
        PreprocessOptions options;
        options.enabled = true;
        options.highpass_hz = 60.0;
        options.declick = true;
        options.denoise = true;
        const auto output = PreprocessAudio(input, fs, options);
        CHECK_EQ(output.size(), input.size());
        CHECK(all_finite(output));
    }

    {
        const Fft fft(16);
        std::vector<std::complex<double>> data(16);
        for (std::size_t i = 0; i < data.size(); ++i) {
            data[i] = {std::sin(0.4 * static_cast<double>(i)), 0.0};
        }
        const auto original = data;
        fft.Forward(data);
        fft.Inverse(data);
        for (std::size_t i = 0; i < data.size(); ++i) {
            CHECK_NEAR(data[i].real(), original[i].real(), 1e-12);
            CHECK_NEAR(data[i].imag(), original[i].imag(), 1e-12);
        }
    }

    return test::finish();
}
