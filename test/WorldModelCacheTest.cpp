#include "TestSupport.h"

#include <exception>
#include <filesystem>
#include <vector>

#include "Audio/WavIO.h"
#include "Config/Config.h"
#include "Vocoder/World/WorldModelCache.h"

using namespace lessampler;

namespace {

    WorldModel make_model() {
        WorldModel model;
        model.fs = 44100;
        model.frame_period = 5.0;
        model.fft_size = 1024;
        model.w_length = 513;
        model.x_length = 1000;
        model.f0 = {100.0, 110.0, 120.0};
        model.spectrogram.assign(3, 513, 0.25);
        model.aperiodicity.assign(3, 513, 0.5);
        return model;
    }

}// namespace

int main() {
    const auto root = std::filesystem::temp_directory_path() / "lessampler-modelio-test";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    const auto wav_path = root / "source.wav";
    WavIO::WriteWav(wav_path, std::vector<double>(1000, 0.1), 44100);

    Config configure;
    const auto model = make_model();

    WorldModelCache writer(wav_path, model, configure);
    writer.SaveAudioModel();
    CHECK(std::filesystem::exists(root / "source.lessaudio"));

    WorldModelCache reader(wav_path);
    CHECK(reader.CheckAudioModel(configure));

    const auto &read = reader.ReadAudioModel(configure);
    CHECK_EQ(read.fs, model.fs);
    CHECK_NEAR(read.frame_period, model.frame_period, 1e-12);
    CHECK_EQ(read.fft_size, model.fft_size);
    CHECK_EQ(read.w_length, model.w_length);
    CHECK_EQ(read.x_length, model.x_length);
    CHECK_EQ(read.f0, model.f0);
    CHECK_EQ(read.spectrogram.rows(), model.spectrogram.rows());
    CHECK_EQ(read.aperiodicity.rows(), model.aperiodicity.rows());

    for (std::size_t i = 0; i < read.spectrogram.rows(); ++i) {
        for (std::size_t j = 0; j < read.spectrogram.cols(); ++j) {
            CHECK_EQ(read.spectrogram[i][j], model.spectrogram[i][j]);
            CHECK_EQ(read.aperiodicity[i][j], model.aperiodicity[i][j]);
        }
    }

    auto changed = configure;
    changed.custom_fft_size = true;
    changed.fft_size = 2048;
    CHECK(!reader.CheckAudioModel(changed));

    WavIO::WriteWav(wav_path, std::vector<double>(2000, 0.2), 44100);
    CHECK(!reader.CheckAudioModel(configure));
    bool rejected_source_change = false;
    try {
        static_cast<void>(reader.ReadAudioModel(configure));
    } catch (const std::exception &) {
        rejected_source_change = true;
    }
    CHECK(rejected_source_change);

    std::filesystem::remove_all(root);
    return test::finish();
}
