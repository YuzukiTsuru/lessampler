#include "TestSupport.h"

#include <algorithm>
#include <vector>

#include "Config/Config.h"
#include "Core/RenderParams.h"
#include "Vocoder/World/WorldTransform.h"

using namespace lessampler;

namespace {

    WorldModel make_model() {
        WorldModel model;
        model.fs = 44100;
        model.frame_period = 5.0;
        model.fft_size = 1024;
        model.w_length = 513;
        model.x_length = 44100;
        model.f0.assign(200, 440.0);
        model.spectrogram.assign(200, 513, 0.1);
        model.aperiodicity.assign(200, 513, 0.5);
        return model;
    }

    RenderParams make_shine() {
        RenderParams shine;
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

    WorldTransform process(model, shine);
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
    WorldTransform bent_process(model, bent);
    const auto &bent_model = bent_process.GetTransAudioModel();
    for (double value: bent_model.f0) {
        CHECK_NEAR(value, 880.0, 1e-9);
    }

    auto short_bend = shine;
    short_bend.pitch_bend.assign(2, 0);
    WorldTransform short_bend_process(model, short_bend);
    CHECK_EQ(short_bend_process.GetTransAudioModel().f0.size(),
             static_cast<std::size_t>(shine.required_frame));

    // A run of unvoiced source frames must stay unvoiced when modulation is
    // zero. pow(0, 0) used to turn them into target-pitch frames.
    auto unvoiced = make_model();
    for (std::size_t i = 0; i < 50; ++i) {
        unvoiced.f0[i] = 0.0;
    }
    auto unvoiced_shine = shine;
    unvoiced_shine.required_frame = 10;
    unvoiced_shine.output_samples = 10 * unvoiced_shine.pitch_step;
    WorldTransform unvoiced_process(unvoiced, unvoiced_shine);
    const auto &unvoiced_model = unvoiced_process.GetTransAudioModel();
    for (std::size_t i = 0; i < 10; ++i) {
        CHECK_NEAR(unvoiced_model.f0[i], 0.0, 1e-12);
    }

    // Aperiodicity follows the same interpolation as the spectral envelope.
    auto interpolated = make_model();
    for (std::size_t i = 0; i < interpolated.aperiodicity.rows(); ++i) {
        std::fill(interpolated.aperiodicity[i].begin(), interpolated.aperiodicity[i].end(),
                  i % 2 == 0 ? 0.2 : 0.8);
    }
    auto ap_shine = shine;
    ap_shine.base_length = 0.0;
    ap_shine.first_half_fixed_part = 100.0;
    ap_shine.stretch_length = 0.5;
    ap_shine.required_frame = 4;
    ap_shine.output_samples = 4 * ap_shine.pitch_step;
    WorldTransform ap_process(interpolated, ap_shine);
    const auto &ap_model = ap_process.GetTransAudioModel();
    CHECK_NEAR(ap_model.aperiodicity[1][0], 0.5, 1e-12);

    // A sample shifted up more than five semitones gets more harmonic energy
    // and less noise; the source pitch here is 200 Hz and the target is 440.
    auto high_model = make_model();
    high_model.f0.assign(200, 200.0);
    WorldTransform high_process(high_model, shine);
    const auto &high_transformed = high_process.GetTransAudioModel();
    CHECK(high_transformed.spectrogram[0][0] > 0.1);
    CHECK(high_transformed.aperiodicity[0][0] < 0.5);

    // A growl-only style is a post-synthesis shaper; the WORLD model itself
    // stays bit-identical to the natural path.
    auto growl_shine = shine;
    growl_shine.style.start = StyleWeights{0.0, 0.0, 1.0};
    growl_shine.style.end = growl_shine.style.start;
    WorldTransform growl_process(model, growl_shine);
    const auto &growl_model = growl_process.GetTransAudioModel();
    CHECK(std::ranges::equal(growl_model.spectrogram[0], transformed.spectrogram[0]));
    CHECK(std::ranges::equal(growl_model.aperiodicity[0], transformed.aperiodicity[0]));

    // Clear lowers the aperiodic share, cry raises harmonic energy.
    auto clear_shine = shine;
    clear_shine.style.start = StyleWeights{1.0, 0.0, 0.0};
    clear_shine.style.end = clear_shine.style.start;
    WorldTransform clear_process(model, clear_shine);
    const auto &clear_model = clear_process.GetTransAudioModel();
    CHECK(clear_model.spectrogram[0][0] < transformed.spectrogram[0][0]);
    CHECK(clear_model.aperiodicity[0][0] < transformed.aperiodicity[0][0]);
    CHECK_NEAR(clear_model.aperiodicity[0][0], 0.3212, 1e-3);

    auto cry_shine = shine;
    cry_shine.style.start = StyleWeights{0.0, 1.0, 0.0};
    cry_shine.style.end = cry_shine.style.start;
    WorldTransform cry_process(model, cry_shine);
    const auto &cry_model = cry_process.GetTransAudioModel();
    CHECK(cry_model.spectrogram[0][0] > transformed.spectrogram[0][0]);
    CHECK(cry_model.aperiodicity[0][0] < transformed.aperiodicity[0][0]);
    CHECK(cry_model.aperiodicity[0][0] > clear_model.aperiodicity[0][0]);

    // A note that morphs from clear to cry holds the end color after
    // morph_ms, and its midpoint lies between the two steady states.
    auto morph_shine = shine;
    morph_shine.style.start = StyleWeights{1.0, 0.0, 0.0};
    morph_shine.style.end = StyleWeights{0.0, 1.0, 0.0};
    morph_shine.style.morph_ms = 200.0;
    WorldTransform morph_process(model, morph_shine);
    const auto &morph_model = morph_process.GetTransAudioModel();
    CHECK_NEAR(morph_model.aperiodicity[0][0], clear_model.aperiodicity[0][0], 1e-12);
    CHECK_NEAR(morph_model.aperiodicity[50][0], cry_model.aperiodicity[0][0], 1e-12);
    CHECK_NEAR(morph_model.aperiodicity[60][0], morph_model.aperiodicity[50][0], 1e-12);

    return test::finish();
}
