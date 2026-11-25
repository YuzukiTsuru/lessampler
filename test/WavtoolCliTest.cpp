#include "TestSupport.h"
#include "TestProcess.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "Audio/WavIO.h"
#include "WavtoolCliPaths.h"

using namespace lessampler;

namespace {

    std::string quote(const std::filesystem::path &path) {
        return "\"" + path.string() + "\"";
    }

    std::string toml_string(const std::filesystem::path &path) {
        std::string text = path.string();
        std::string escaped;
        escaped.reserve(text.size());
        for (char character: text) {
            if (character == '\\' || character == '"') {
                escaped.push_back('\\');
            }
            escaped.push_back(character);
        }
        return escaped;
    }

    int run(const std::string &command) {
        return test::run_command(command);
    }

    constexpr const char *kFlatEnvelope = "0,0,0,100,100,100,100,%";

}// namespace

int main() {
    const auto root = std::filesystem::temp_directory_path() / "lessampler-wavtool-cli-test";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    const auto input = root / "in.wav";
    const std::vector<double> samples(100, 0.5);
    WavIO::WriteWav(input, samples, 1000);

    const auto output = root / "out.wav";
    const auto tool = std::filesystem::path(LESSAMPLER_BINARY_PATH);
    REQUIRE(std::filesystem::exists(tool));

    {
        // The classic wavtool protocol is selected by its 15 positional arguments.
        const auto result = run(quote(tool) + " " + quote(output) + " " + quote(input) +
                                " 0 100 0 5 35 0 100 100 0 0 0 10 100");
        CHECK_EQ(result, 0);
        const auto read = WavIO::ReadWav(output);
        CHECK_EQ(read.sample_rate, 1000);
        CHECK(read.samples.size() == 100);
        CHECK_NEAR(read.samples[50], 0.5, 1e-3);
        CHECK(read.samples[0] < 0.01);
        CHECK(read.samples[99] < 0.05);
    }

    {
        std::filesystem::remove(output);
        CHECK_EQ(run(quote(tool) + " " + quote(output) + " " + quote(input) +
                     " 0 100 0 0 0 100 100 100 100 0 0 10 100"),
                 0);
        const auto result = run(quote(tool) + " " + quote(output) + " " + quote(input) +
                                " 0 100 0 0 0 100 100 100 100 50 0 10 100");
        CHECK_EQ(result, 0);
        const auto read = WavIO::ReadWav(output);
        CHECK(read.samples.size() == 150);
        CHECK_NEAR(read.samples[75], 1.0, 2e-3);
        CHECK_NEAR(read.samples[140], 0.5, 1e-3);
    }

    {
        const auto plan_output = root / "plan.wav";
        const auto plan_path = root / "plan.toml";
        std::ofstream plan(plan_path);
        plan << "output = \"" << toml_string(plan_output) << "\"\n"
             << "sample_rate = 1000\n"
             << "channels = 1\n"
             << "peak_normalize = 0.0\n"
             << "[[notes]]\n"
             << "input = \"" << toml_string(input) << "\"\n"
             << "position_ms = 0.0\n"
             << "skip_ms = 0.0\n"
             << "length_ms = 100.0\n"
             << "envelope = \"" << kFlatEnvelope << "\"\n"
             << "[[notes]]\n"
             << "input = \"" << toml_string(input) << "\"\n"
             << "position_ms = 50.0\n"
             << "skip_ms = 0.0\n"
             << "length_ms = 100.0\n"
             << "envelope = \"" << kFlatEnvelope << "\"\n";
        plan.close();

        CHECK_EQ(run(quote(tool) + " --plan " + quote(plan_path)), 0);
        const auto read = WavIO::ReadWav(plan_output);
        CHECK(read.samples.size() == 150);
        CHECK_NEAR(read.samples[75], 1.0, 2e-3);
    }

    CHECK(run(quote(tool) + " --plan") != 0);

    std::filesystem::remove_all(root);
    return test::finish();
}
