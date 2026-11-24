#include "TestSupport.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <string_view>
#include <vector>

#include "AudioModel/AudioModel.h"
#include "AudioModel/Synthesis/Synthesis.h"
#include "AudioProcess/AudioProcess.h"
#include "AudioProcess/AutoAMP.h"
#include "ConfigUnit/lessConfigure.h"
#include "FileIO/WavIO.h"
#include "Shine/Binding/libUTAU/libUTAU.h"
#include "Shine/Shine.h"

int main() {
    const auto data_path = std::filesystem::path(LESSAMPLER_TEST_DATA_DIR) / "vaiueo2d.wav";
    const auto wav = WavIO::ReadWav(data_path);

    lessConfigure configure;
    AudioModel audio_model(wav.samples, wav.sample_rate, configure);
    const auto &model = audio_model.GetAudioModel();

    const std::array<std::string_view, 14> args{
            "lessampler", "vaiueo2d.wav", "out.wav", "A4", "100", "", "0", "1000",
            "100", "0", "100", "0", "!120", ""};

    libUTAU utau(args);
    utau.CheckPara(model);

    Shine shine(args, model, Shine::SHINE_MODE::UTAU);
    const auto para = shine.GetShine();

    AudioProcess process(model, para);
    const auto &transformed = process.GetTransAudioModel();
    CHECK_EQ(transformed.f0.size(), static_cast<std::size_t>(para.required_frame));
    CHECK_EQ(transformed.spectrogram.rows(), transformed.f0.size());
    CHECK_EQ(transformed.aperiodicity.rows(), transformed.f0.size());

    Synthesis synthesis(transformed, para.output_samples);
    auto &output = synthesis.GetWavData();
    CHECK_EQ(output.size(), static_cast<std::size_t>(para.output_samples));

    bool finite = true;
    double peak = 0.0;
    for (double value: output) {
        finite = finite && std::isfinite(value);
        peak = std::max(peak, std::abs(value));
    }
    CHECK(finite);
    CHECK(peak > 0.0);

    AutoAMP::ApplyInPlace(output, para.volumes);

    bool amp_finite = true;
    bool amp_in_range = true;
    for (double value: output) {
        amp_finite = amp_finite && std::isfinite(value);
        amp_in_range = amp_in_range && std::abs(value) <= 1.0;
    }
    CHECK(amp_finite);
    CHECK(amp_in_range);

    return test::finish();
}
