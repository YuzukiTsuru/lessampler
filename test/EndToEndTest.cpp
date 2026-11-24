#include "TestSupport.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#if defined(_WIN32)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
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

#if defined(_WIN32)
    // Quote one argument following the rules the MSVC CRT uses to parse
    // argv. _spawnv does not quote empty arguments, which drops them and
    // shifts every later UTAU parameter.
    std::string quote_windows(std::string_view value) {
        if (value.empty()) {
            return "\"\"";
        }
        if (value.find_first_of(" \t\n\v\"") == std::string_view::npos) {
            return std::string(value);
        }

        std::string result = "\"";
        for (auto iterator = value.begin();; ++iterator) {
            std::size_t backslashes = 0;
            while (iterator != value.end() && *iterator == '\\') {
                ++iterator;
                ++backslashes;
            }
            if (iterator == value.end()) {
                result.append(backslashes * 2, '\\');
                break;
            }
            if (*iterator == '"') {
                result.append(backslashes * 2 + 1, '\\');
            } else {
                result.append(backslashes, '\\');
            }
            result.push_back(*iterator);
        }
        result.push_back('"');
        return result;
    }
#endif

    int run(const std::string &binary, const std::vector<std::string> &args) {
#if defined(_WIN32)
        std::string command = quote_windows(binary);
        for (const auto &argument: args) {
            command += ' ';
            command += quote_windows(argument);
        }

        STARTUPINFOA startup{};
        startup.cb = sizeof(startup);
        PROCESS_INFORMATION process{};
        if (!CreateProcessA(binary.c_str(), command.data(), nullptr, nullptr, FALSE, 0,
                            nullptr, nullptr, &startup, &process)) {
            return -1;
        }

        WaitForSingleObject(process.hProcess, INFINITE);
        DWORD exit_code = 1;
        GetExitCodeProcess(process.hProcess, &exit_code);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        return static_cast<int>(exit_code);
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
