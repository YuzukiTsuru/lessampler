#include "TestProcess.h"
#include "TestSupport.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "Audio/WavIO.h"
#include "Host/Wavtool/WavAssembly.h"
#include "WavtoolEndToEndPaths.h"

using namespace lessampler;

namespace {

    std::string quote(const std::filesystem::path &path) {
        return "\"" + path.string() + "\"";
    }

    std::string toml_string(const std::filesystem::path &path) {
        std::string text = path.string();
        std::string escaped;
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

}// namespace

int main() {
    const auto root = std::filesystem::temp_directory_path() / "lessampler-wavtool-e2e-test";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    const auto source = std::filesystem::path(LESSAMPLER_TEST_DATA_DIR) / "vaiueo2d.wav";
    REQUIRE(std::filesystem::exists(source));
    REQUIRE(std::filesystem::exists(std::filesystem::path(LESSAMPLER_BINARY_PATH)));

    const auto note0 = root / "note0.wav";
    const auto note1 = root / "note1.wav";
    const std::string common = " 100 S1 0 1000 100 0 100 0 120";
    CHECK_EQ(run(quote(LESSAMPLER_BINARY_PATH) + " " + quote(source) + " " + quote(note0) + " A4" + common), 0);
    CHECK_EQ(run(quote(LESSAMPLER_BINARY_PATH) + " " + quote(source) + " " + quote(note1) + " A4" + common), 0);
    CHECK(std::filesystem::exists(note0));
    CHECK(std::filesystem::exists(note1));

    const auto plan_path = root / "plan.toml";
    const auto output = root / "joined.wav";
    std::ofstream plan(plan_path);
    plan << "output = \"" << toml_string(output) << "\"\n"
         << "sample_rate = 22050\n"
         << "channels = 1\n"
         << "[[notes]]\n"
         << "input = \"" << toml_string(note0) << "\"\n"
         << "position_ms = 0.0\n"
         << "skip_ms = 0.0\n"
         << "length_ms = 1000.0\n"
         << "[[notes]]\n"
         << "input = \"" << toml_string(note1) << "\"\n"
         << "position_ms = 900.0\n"
         << "skip_ms = 0.0\n"
         << "length_ms = 1000.0\n";
    plan.close();

    CHECK_EQ(run(quote(LESSAMPLER_BINARY_PATH) + " --plan " + quote(plan_path)), 0);
    REQUIRE(std::filesystem::exists(output));

    const auto read = WavIO::ReadWav(output);
    CHECK_EQ(read.sample_rate, 22050);
    CHECK(read.samples.size() == 41895);
    const double peak = wavtool::WavAssembly::Peak(read.samples);
    CHECK(peak > 0.01);
    CHECK(peak <= 1.0);
    CHECK(std::isfinite(peak));

    std::filesystem::remove_all(root);
    return test::finish();
}
