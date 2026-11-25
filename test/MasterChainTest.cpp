#include "TestSupport.h"

#include <cmath>
#include <numbers>
#include <stdexcept>
#include <vector>

#include "Audio/Dsp/MasterChain.h"

using namespace lessampler;

namespace {

    std::vector<double> sine(double amplitude, std::size_t count, int sample_rate = 1000, double frequency = 100.0) {
        std::vector<double> samples(count);
        for (std::size_t i = 0; i < count; ++i) {
            samples[i] = amplitude * std::sin(2.0 * std::numbers::pi * frequency * static_cast<double>(i) /
                                              static_cast<double>(sample_rate));
        }
        return samples;
    }

    double rms(const std::vector<double> &samples) {
        double sum = 0.0;
        for (double value: samples) {
            sum += value * value;
        }
        return std::sqrt(sum / static_cast<double>(samples.size()));
    }

}// namespace

int main() {
    {
        const auto input = sine(0.1, 1000);
        auto output = input;
        MasterOptions options;
        options.ceiling_db = -1.0;
        MasterChain::Process(output, 1000, 1, options);
        CHECK(output == input);
    }

    {
        auto output = sine(2.0, 1000);
        MasterOptions options;
        options.ceiling_db = -6.0;
        MasterChain::Process(output, 1000, 1, options);
        const double ceiling = std::pow(10.0, -6.0 / 20.0);
        for (double value: output) {
            CHECK(std::isfinite(value));
            CHECK(std::abs(value) <= ceiling + 1e-9);
        }
    }

    {
        std::vector<double> output(2000, 1.0);
        MasterOptions options;
        options.highpass_hz = 100.0;
        options.ceiling_db = 0.0;
        MasterChain::Process(output, 1000, 1, options);
        CHECK(std::abs(output.back()) < 0.01);
    }

    {
        auto output = sine(0.02, 4000, 1000, 97.0);
        MasterOptions options;
        options.target_rms_db = -20.0;
        options.ceiling_db = 0.0;
        MasterChain::Process(output, 1000, 1, options);
        const double target = std::pow(10.0, -20.0 / 20.0);
        CHECK(std::abs(rms(output) - target) < 0.01);
    }

    {
        bool rejected = false;
        try {
            (void) MasterOptions::Preset("not-a-preset");
        } catch (const std::invalid_argument &) {
            rejected = true;
        }
        CHECK(rejected);
    }

    return test::finish();
}
