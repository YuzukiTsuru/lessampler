#include "TestSupport.h"

#include <filesystem>

#include "ConfigUnit/lessConfigure.h"
#include "FileIO/AudioModelIO.h"
#include "FileIO/GenerateAudioModel.h"

int main() {
    const auto root = std::filesystem::temp_directory_path() / "lessampler-generate-model-test";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);

    const auto source = std::filesystem::path(LESSAMPLER_TEST_DATA_DIR) / "vaiueo2d.wav";
    const auto target = root / "vaiueo2d.wav";
    std::filesystem::copy_file(source, target);

    lessConfigure configure;
    GenerateAudioModel generator(root, configure);

    CHECK(std::filesystem::exists(root / "vaiueo2d.lessaudio"));

    AudioModelIO reader(target);
    CHECK(reader.CheckAudioModel(configure));
    const auto &model = reader.ReadAudioModel(configure);
    CHECK(!model.f0.empty());
    CHECK_EQ(model.spectrogram.rows(), model.f0.size());

    std::filesystem::remove_all(root);
    return test::finish();
}
