#include "TestSupport.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#include "EndToEndBinary.h"
#include "FileIO/WavIO.h"

namespace {

    std::string quote(const std::filesystem::path &path) {
        return "\"" + path.string() + "\"";
    }

    int run(const std::string &command) {
        return std::system(command.c_str());
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

    const std::string binary = LESSAMPLER_BINARY_PATH;
    const auto source = std::filesystem::path(LESSAMPLER_TEST_DATA_DIR) / "vaiueo2d.wav";
    const auto input = root / "input.wav";
    const auto output = root / "output.wav";
    std::filesystem::copy_file(source, input);

    const std::string render = quote(binary) + " " + quote(input) + " " + quote(output) + " A4 100 \"\" 0 1000 100 0 100 0 \"!120\" \"\"";
    CHECK_EQ(run(render), 0);

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
    const std::string blank = quote(binary) + " " + quote(missing) + " " + quote(blank_output) + " A4 100 \"\" 0 1000 100 0 100 0 \"!120\" \"\"";
    CHECK_EQ(run(blank), 0);

    const auto silent = WavIO::ReadWav(blank_output);
    CHECK_EQ(silent.sample_rate, 44100);
    CHECK_EQ(silent.channels, 1);
    CHECK_EQ(silent.samples.size(), static_cast<std::size_t>(44101));
    for (double value: silent.samples) {
        CHECK_NEAR(value, 0.0, 1e-12);
    }

    CHECK_EQ(run(quote(binary) + " --version"), 0);
    CHECK_EQ(run(quote(binary) + " --help"), 0);

    std::filesystem::remove_all(root);
    return test::finish();
}
