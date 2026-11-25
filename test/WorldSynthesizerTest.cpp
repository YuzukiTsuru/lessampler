#include "TestSupport.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <vector>

#include "Config/Config.h"
#include "Vocoder/World/WorldModelBuilder.h"
#include "Vocoder/World/WorldSynthesizer.h"

using namespace lessampler;

int main() {
    constexpr int fs = 44100;
    constexpr double frequency = 440.0;
    constexpr std::size_t sample_count = 44100;

    std::vector<double> samples(sample_count);
    for (std::size_t i = 0; i < samples.size(); ++i) {
        samples[i] = 0.5 * std::sin(2.0 * std::numbers::pi * frequency * static_cast<double>(i) / static_cast<double>(fs));
    }

    Config configure;
    const auto model = BuildWorldModel(samples, fs, configure);

    const auto output = SynthesizeWorld(model, 4410);
    CHECK_EQ(output.size(), static_cast<std::size_t>(4410));

    bool finite = true;
    double peak = 0.0;
    for (double value: output) {
        finite = finite && std::isfinite(value);
        peak = std::max(peak, std::abs(value));
    }
    CHECK(finite);
    CHECK(peak > 0.0);

    // The final frame interval must be flushed, not left as an exact-zero tail.
    const auto tail_begin = output.size() > 100 ? output.size() - 100 : 0;
    const bool tail_active = std::any_of(output.begin() + static_cast<std::ptrdiff_t>(tail_begin),
                                         output.end(),
                                         [](double value) { return std::abs(value) > 1e-6; });
    CHECK(tail_active);

    CHECK(SynthesizeWorld(model, 4410) == output);
    CHECK(SynthesizeWorld(model, 0).empty());

    return test::finish();
}
