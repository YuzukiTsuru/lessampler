#include "TestSupport.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

#include "ConfigUnit/lessConfigure.h"
#include "FileIO/AudioModelIO.h"
#include "FileIO/WavIO.h"
#include "Utils/exception.h"

int main() {
    const auto root = std::filesystem::temp_directory_path() / "lessampler-model-version-test";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    const auto wav_path = root / "source.wav";
    WavIO::WriteWav(wav_path, std::vector<double>(1000, 0.1), 44100);

    const auto model_path = root / "source.lessaudio";
    lessConfigure configure;

    {
        std::ofstream out(model_path, std::ios::binary);
        constexpr std::array<char, 4> magic{'L', 'S', 'A', 'M'};
        const std::uint32_t version = 1;
        out.write(magic.data(), 4);
        out.write(reinterpret_cast<const char *>(&version), sizeof(version));
    }

    AudioModelIO reader(wav_path);
    CHECK(!reader.CheckAudioModel(configure));

    bool rejected = false;
    try {
        reader.ReadAudioModel(configure);
    } catch (const std::exception &) {
        rejected = true;
    }
    CHECK(rejected);

    {
        std::ofstream out(model_path, std::ios::binary);
        constexpr std::array<char, 4> magic{'L', 'S', 'A', 'M'};
        const std::uint32_t version = 2;
        out.write(magic.data(), 4);
        out.write(reinterpret_cast<const char *>(&version), sizeof(version));
    }

    CHECK(!reader.CheckAudioModel(configure));
    rejected = false;
    try {
        reader.ReadAudioModel(configure);
    } catch (const std::exception &) {
        rejected = true;
    }
    CHECK(rejected);

    std::filesystem::remove_all(root);
    return test::finish();
}
