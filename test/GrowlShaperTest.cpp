#include "TestSupport.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <vector>

#include "Audio/Dsp/GrowlShaper.h"
#include "Core/VoiceStyle.h"

using namespace lessampler;

namespace {

    std::vector<double> make_tone(int sample_rate, double seconds) {
        const auto count = static_cast<std::size_t>(std::llround(static_cast<double>(sample_rate) * seconds));
        std::vector<double> samples(count);
        for (std::size_t i = 0; i < count; ++i) {
            const double t = static_cast<double>(i) / static_cast<double>(sample_rate);
            samples[i] = 0.9 * std::sin(2.0 * std::numbers::pi * 220.0 * t);
        }
        return samples;
    }

    bool all_finite(const std::vector<double> &samples) {
        return std::ranges::all_of(samples, [](double value) { return std::isfinite(value); });
    }

}// namespace

int main() {
    constexpr int fs = 48000;

    {
        const auto input = make_tone(fs, 0.5);
        auto samples = input;
        VoiceStyle style;
        style.start = StyleWeights{1.0, 0.0, 0.0};
        style.end = style.start;
        GrowlShaper::Apply(samples, fs, style);
        CHECK(samples == input);
    }

    {
        const auto input = make_tone(fs, 0.5);
        auto samples = input;
        GrowlShaper::Apply(samples, fs, StyleWeights{0.0, 0.0, 1.0});
        CHECK(samples != input);
        CHECK(all_finite(samples));
        CHECK(std::ranges::all_of(samples, [](double value) { return std::abs(value) <= 1.0 + 1e-12; }));

        auto again = input;
        GrowlShaper::Apply(again, fs, StyleWeights{0.0, 0.0, 1.0});
        CHECK(samples == again);
    }

    {
        const auto input = make_tone(fs, 0.5);
        auto samples = input;
        VoiceStyle style;
        style.start = StyleWeights{0.0, 0.0, 1.0};
        style.end = StyleWeights{};
        style.morph_ms = 100.0;
        GrowlShaper::Apply(samples, fs, style);
        constexpr std::size_t early = 100;
        CHECK(samples[early] != input[early]);
        const auto held = static_cast<std::size_t>(0.2 * fs);
        REQUIRE(held < samples.size());
        CHECK_EQ(samples[held], input[held]);
        CHECK_EQ(samples.back(), input.back());
    }

    return test::finish();
}
