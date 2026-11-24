#include "TestSupport.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

#include "Audio/WavIO.h"
#include "Config/Config.h"
#include "Core/Exception.h"
#include "Vocoder/World/WorldModelCache.h"

using namespace lessampler;

int main() {
    const auto root = std::filesystem::temp_directory_path() / "lessampler-model-version-test";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    const auto wav_path = root / "source.wav";
    WavIO::WriteWav(wav_path, std::vector<double>(1000, 0.1), 44100);

    const auto model_path = root / "source.lessaudio";
    Config configure;

    {
        std::ofstream out(model_path, std::ios::binary);
        constexpr std::array<char, 4> magic{'L', 'S', 'A', 'M'};
        const std::uint32_t version = 1;
        out.write(magic.data(), 4);
        out.write(reinterpret_cast<const char *>(&version), sizeof(version));
    }

    WorldModelCache reader(wav_path);
    CHECK(!reader.CheckAudioModel(configure));

    bool rejected = false;
    try {
        static_cast<void>(reader.ReadAudioModel(configure));
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
        static_cast<void>(reader.ReadAudioModel(configure));
    } catch (const std::exception &) {
        rejected = true;
    }
    CHECK(rejected);

    std::filesystem::remove_all(root);
    return test::finish();
}
