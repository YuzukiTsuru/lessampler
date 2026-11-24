#include "TestSupport.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#if defined(_WIN32)
#include <process.h>
#endif

#include "Audio/WavIO.h"
#include "EndToEndBinary.h"

using namespace lessampler;

namespace {

    std::string native_separators(std::string value) {
#if defined(_WIN32)
        std::ranges::replace(value, '/', '\\');
#endif
        return value;
    }

    std::string quote(std::string_view value) {
        return "\"" + std::string(value) + "\"";
    }

    int run(const std::string &binary, const std::vector<std::string> &args) {
#if defined(_WIN32)
        std::vector<const char *> argv;
        argv.reserve(args.size() + 2);
        argv.push_back(binary.c_str());
        for (const auto &argument: args) {
            argv.push_back(argument.c_str());
        }
        argv.push_back(nullptr);
        return _spawnv(_P_WAIT, binary.c_str(), argv.data());
#else
        std::string command = quote(binary);
        for (const auto &argument: args) {
            command += " " + quote(argument);
        }
        return std::system(command.c_str());
#endif
    }

    double peak_of(const std::vector<double> &samples) {
        double peak = 0.0;
        for (double value: samples) {
            peak = std::max(peak, std::abs(value));
        }
        return peak;
    }

    bool all_finite(const std::vector<double> &samples) {
        for (double value: samples) {
            if (!std::isfinite(value)) {
                return false;
            }
        }
        return true;
    }

    double rms_of(const std::vector<double> &samples) {
        if (samples.empty()) {
            return 0.0;
        }
        double sum = 0.0;
        for (double value: samples) {
            sum += value * value;
        }
        return std::sqrt(sum / static_cast<double>(samples.size()));
    }

}// namespace

int main() {
    const auto root = std::filesystem::temp_directory_path() / "lessampler-e2e-test";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    const std::string binary = native_separators(LESSAMPLER_BINARY_PATH);
    const auto source = std::filesystem::path(LESSAMPLER_TEST_DATA_DIR) / "vaiueo2d.wav";
    const auto input = root / "input.wav";
    const auto output = root / "output.wav";
    std::filesystem::copy_file(source, input);

    const std::vector<std::string> render_args{
            input.string(), output.string(), "A4", "100", "", "0", "1000",
            "100", "0", "100", "0", "!120", ""};
    CHECK_EQ(run(binary, render_args), 0);

    const auto rendered = WavIO::ReadWav(output);
    CHECK_EQ(rendered.sample_rate, 22050);
    CHECK_EQ(rendered.channels, 1);
    CHECK_EQ(rendered.samples.size(), static_cast<std::size_t>(22051));
    CHECK(all_finite(rendered.samples));
    CHECK(peak_of(rendered.samples) > 0.01);
    CHECK(peak_of(rendered.samples) <= 1.0);
    CHECK(rms_of(rendered.samples) > 0.001);

    const auto missing = root / "missing.wav";
    const auto blank_output = root / "blank.wav";
    const std::vector<std::string> blank_args{
            missing.string(), blank_output.string(), "A4", "100", "", "0", "1000",
            "100", "0", "100", "0", "!120", ""};
    CHECK_EQ(run(binary, blank_args), 0);

    const auto silent = WavIO::ReadWav(blank_output);
    CHECK_EQ(silent.sample_rate, 44100);
    CHECK_EQ(silent.channels, 1);
    CHECK_EQ(silent.samples.size(), static_cast<std::size_t>(44101));
    for (double value: silent.samples) {
        CHECK_NEAR(value, 0.0, 1e-12);
    }

    CHECK_EQ(run(binary, {"--version"}), 0);
    CHECK_EQ(run(binary, {"--help"}), 0);

    std::filesystem::remove_all(root);
    return test::finish();
}
