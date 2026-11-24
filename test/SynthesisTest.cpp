#include "TestSupport.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

#include "Analysis/World/WorldModelBuilder.h"
#include "Analysis/World/WorldSynthesizer.h"
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

    CHECK(SynthesizeWorld(model, 4410) == output);
    CHECK(SynthesizeWorld(model, 0).empty());

    return test::finish();
}
