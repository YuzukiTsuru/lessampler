#include "TestSupport.h"

#include <vector>

#include "AudioProcess/AudioProcess.h"
#include "ConfigUnit/lessConfigure.h"
#include "Shine/ShinePara.h"

namespace {

    lessAudioModel make_model() {
        lessAudioModel model;
        model.fs = 44100;
        model.frame_period = 5.0;
        model.fft_size = 1024;
        model.w_length = 513;
        model.x_length = 44100;
        model.f0.assign(200, 200.0);
        model.spectrogram.assign(200, 513, 0.1);
        model.aperiodicity.assign(200, 513, 0.5);
        return model;
    }

    ShinePara make_shine() {
        ShinePara shine;
        shine.scale_num = 440.0;
        shine.offset = 0.0;
        shine.velocity = 1.0;
        shine.required_length = 500.0;
        shine.first_half_fixed_part = 100.0;
        shine.last_unused_part = 0.0;
        shine.wave_length = 1000.0;
        shine.pre_cross_length = 900.0;
        shine.base_length = 100.0;
        shine.cross_length = 400.0;
        shine.stretch_length = 0.5;
        shine.output_samples = 11026;
        shine.required_frame = 100;
        shine.pitch_step = 256;
        shine.pitch_length = 10;
        shine.pitch_bend.assign(11, 0);
        shine.modulation = 0;
        return shine;
    }

}// namespace

int main() {
    const auto model = make_model();
    const auto shine = make_shine();

    AudioProcess process(model, shine);
    const auto &transformed = process.GetTransAudioModel();

    CHECK_EQ(transformed.f0.size(), static_cast<std::size_t>(shine.required_frame));
    CHECK_EQ(transformed.spectrogram.rows(), transformed.f0.size());
    CHECK_EQ(transformed.aperiodicity.rows(), transformed.f0.size());
    CHECK_EQ(transformed.x_length, model.x_length);
    CHECK_EQ(transformed.spectrogram[0].size(), static_cast<std::size_t>(model.w_length));
    CHECK_EQ(transformed.aperiodicity[0].size(), static_cast<std::size_t>(model.w_length));
    CHECK_NEAR(transformed.spectrogram[0][0], 0.1, 1e-12);
    CHECK_NEAR(transformed.aperiodicity[0][0], 0.5, 1e-12);

    for (double value: transformed.f0) {
        CHECK_NEAR(value, 440.0, 1e-9);
    }

    auto bent = shine;
    bent.pitch_bend.assign(bent.pitch_bend.size(), 1200);
    AudioProcess bent_process(model, bent);
    const auto &bent_model = bent_process.GetTransAudioModel();
    for (double value: bent_model.f0) {
        CHECK_NEAR(value, 880.0, 1e-9);
    }

    auto short_bend = shine;
    short_bend.pitch_bend.assign(2, 0);
    AudioProcess short_bend_process(model, short_bend);
    CHECK_EQ(short_bend_process.GetTransAudioModel().f0.size(),
             static_cast<std::size_t>(shine.required_frame));

    return test::finish();
}
