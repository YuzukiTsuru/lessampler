#include "TestSupport.h"

#include <cmath>
#include <filesystem>
#include <vector>

#include "Audio/WavIO.h"
#include "Core/Exception.h"
#include "Host/Wavtool/WavAssembly.h"

using namespace lessampler;
using namespace lessampler::wavtool;

namespace {

    Envelope flat_envelope() {
        Envelope envelope;
        envelope.p1 = 0.0;
        envelope.p2 = 0.0;
        envelope.p3 = 0.0;
        envelope.p4 = 0.0;
        envelope.p5 = 0.0;
        envelope.v1 = 100.0;
        envelope.v2 = 100.0;
        envelope.v3 = 100.0;
        envelope.v4 = 100.0;
        envelope.v5 = 100.0;
        return envelope;
    }

    std::filesystem::path make_constant(const std::filesystem::path &path, std::size_t count, double value) {
        std::vector<double> samples(count, value);
        WavIO::WriteWav(path, samples, 1000);
        return path;
    }

    std::filesystem::path make_ramp(const std::filesystem::path &path, std::size_t count) {
        std::vector<double> samples(count);
        for (std::size_t i = 0; i < count; ++i) {
            samples[i] = static_cast<double>(i) / 1000.0;
        }
        WavIO::WriteWav(path, samples, 1000);
        return path;
    }

    WavSegment segment(const std::filesystem::path &input, double skip_ms, double length_ms) {
        WavSegment result{input, skip_ms, length_ms, flat_envelope()};
        result.declick_ms = 0.0;
        return result;
    }

}// namespace

int main() {
    const auto root = std::filesystem::temp_directory_path() / "lessampler-wavassembly-test";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    const auto constant = make_constant(root / "constant.wav", 100, 0.5);
    const auto ramp = make_ramp(root / "ramp.wav", 100);

    {
        WavBuffer output{};
        output.sample_rate = 1000;
        output.channels = 1;
        WavAssembly::AppendAt(output, segment(constant, 0.0, 100.0), 0);
        CHECK_EQ(output.frames(), 100);
        for (double value: output.samples) {
            CHECK_NEAR(value, 0.5, 1e-4);
        }
    }

    {
        WavBuffer output{};
        output.sample_rate = 1000;
        output.channels = 1;
        WavAssembly::AppendRelative(output, segment(constant, 0.0, 100.0), 0.0);
        WavAssembly::AppendRelative(output, segment(constant, 0.0, 100.0), 50.0);
        CHECK_EQ(output.frames(), 150);
        CHECK_NEAR(output.samples[10], 0.5, 1e-4);
        CHECK_NEAR(output.samples[75], 1.0, 1e-4);
        CHECK_NEAR(output.samples[140], 0.5, 1e-4);
    }

    {
        WavBuffer output{};
        output.sample_rate = 1000;
        output.channels = 1;
        WavAssembly::AppendRelative(output, segment(constant, 0.0, 100.0), 0.0);
        WavAssembly::AppendRelative(output, segment(constant, 0.0, 100.0), -20.0);
        CHECK_EQ(output.frames(), 220);
        CHECK_NEAR(output.samples[105], 0.0, 1e-4);
        CHECK_NEAR(output.samples[119], 0.0, 1e-4);
        CHECK_NEAR(output.samples[130], 0.5, 1e-4);
    }

    {
        WavBuffer output{};
        output.sample_rate = 1000;
        output.channels = 1;
        WavAssembly::AppendAt(output, segment(ramp, 10.0, 90.0), 0);
        CHECK_EQ(output.frames(), 90);
        CHECK_NEAR(output.samples[0], 0.010, 1e-4);
        CHECK_NEAR(output.samples[89], 0.099, 1e-4);
    }

    {
        WavBuffer output{};
        output.sample_rate = 1000;
        output.channels = 1;
        WavAssembly::AppendAt(output, segment(ramp, 0.0, 100.0), -20);
        CHECK_EQ(output.frames(), 80);
        CHECK_NEAR(output.samples[0], 0.020, 1e-4);
        CHECK_NEAR(output.samples[79], 0.099, 1e-4);
    }

    {
        WavBuffer output{};
        output.sample_rate = 1000;
        output.channels = 1;
        WavAssembly::AppendAt(output, segment(constant, 0.0, 120.0), 0);
        CHECK_EQ(output.frames(), 120);
        CHECK_NEAR(output.samples[90], 0.5, 1e-4);
        CHECK_NEAR(output.samples[110], 0.0, 1e-4);
    }

    {
        WavBuffer output{};
        output.sample_rate = 1000;
        output.channels = 2;
        WavAssembly::AppendAt(output, segment(constant, 0.0, 100.0), 0);
        CHECK_EQ(output.frames(), 100);
        CHECK_NEAR(output.samples[0], 0.5, 1e-4);
        CHECK_NEAR(output.samples[1], 0.5, 1e-4);
        CHECK_NEAR(output.samples[198], 0.5, 1e-4);
        CHECK_NEAR(output.samples[199], 0.5, 1e-4);
    }

    {
        WavBuffer output{};
        output.sample_rate = 1000;
        output.channels = 1;
        const std::vector<double> values{0.1, -0.1};
        WavIO::WriteWavFrames(root / "mismatch.wav", values, 8000, 1);
        bool rejected = false;
        try {
            WavAssembly::AppendAt(output, segment(root / "mismatch.wav", 0.0, 10.0), 0);
        } catch (const audio_file_error &) {
            rejected = true;
        }
        CHECK(rejected);
    }

    {
        WavBuffer output{};
        output.sample_rate = 1000;
        output.channels = 1;
        auto faded = segment(constant, 0.0, 100.0);
        faded.declick_ms = 5.0;
        WavAssembly::AppendAt(output, faded, 0);
        CHECK_NEAR(output.samples[0], 0.0, 1e-4);
        CHECK_NEAR(output.samples[4], 0.5, 0.02);
        CHECK_NEAR(output.samples[95], 0.5, 0.02);
        CHECK_NEAR(output.samples[99], 0.0, 1e-4);
    }

    {
        WavBuffer output{};
        output.sample_rate = 1000;
        output.channels = 1;
        auto faded = segment(constant, 0.0, 100.0);
        faded.declick_ms = 5.0;
        WavAssembly::AppendAt(output, faded, -20);
        CHECK_NEAR(output.samples[0], 0.5, 1e-4);
        CHECK_NEAR(output.samples[79], 0.0, 1e-4);
    }

    {
        WavBuffer output{};
        output.sample_rate = 1000;
        output.channels = 1;
        output.samples = {2.0, -2.0};
        const auto path = root / "clamped.wav";
        WavAssembly::Write(output, path);
        const auto read = WavIO::ReadWav(path);
        REQUIRE(read.samples.size() == 2);
        CHECK_NEAR(read.samples[0], 1.0, 1e-4);
        CHECK_NEAR(read.samples[1], -1.0, 1e-4);
    }

    {
        WavBuffer output{};
        output.sample_rate = 1000;
        output.channels = 1;
        output.samples = {2.0, -1.0, 0.5};
        const auto path = root / "limited.wav";
        WavAssembly::Write(output, path, 0.99);
        const auto read = WavIO::ReadWav(path);
        REQUIRE(read.samples.size() == 3);
        CHECK_NEAR(WavAssembly::Peak(read.samples), 0.99, 1e-3);
    }

    std::filesystem::remove_all(root);
    return test::finish();
}
