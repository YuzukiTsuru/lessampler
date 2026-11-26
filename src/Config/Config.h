/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_LESSCONFIGURE_H
#define LESSAMPLER_LESSCONFIGURE_H

#include <string>
#include <string_view>

#include "Config/F0EstimatorKind.h"
#include "Config/NeuralVocoderSpec.h"
#include "Config/VocoderKind.h"
#include "Core/PreprocessOptions.h"
#include "Core/VoiceStyle.h"

namespace lessampler {

    class Config {
    public:
        std::string version;
        bool debug_mode = false;
        VocoderKind vocoder = VocoderKind::world;
        double model_amp = 0.85;
        double audio_model_frame_period = (1000.0 * 256 / 44100);
        bool custom_fft_size = false;
        int fft_size = 1024;
        F0EstimatorKind f0_estimator = F0EstimatorKind::auto_select;
        int f0_speed = 1;
        double f0_dio_floor = 40.0;
        double f0_harvest_floor = 40.0;
        double f0_ceil = 800.0;
        double f0_cheap_trick_floor = 71.0;
        double f0_allow_range = 0.1;
        double ap_threshold = 0.10;

        /*
         * Source cleanup applied after decode and before modeling. Disabled by
         * default so existing analyses and caches remain unchanged.
         */
        PreprocessOptions preprocess;

        /*
         * Optional RMVPE F0 estimator. The model maps 16 kHz audio to a
         * 360-bin salience tensor; it is independent of the neural vocoder
         * model in [neural].
         */
        std::string f0_neural_model_path;
        int f0_neural_threads = 0;
        double f0_neural_threshold = 0.03;
        double f0_neural_floor = 40.0;
        double f0_neural_ceil = 1200.0;

        /*
     * Optional neural vocoder. The model is an NHVSing-compatible ONNX graph
     * (mel/f0/uv -> waveform). Paths are resolved relative to the config file
     * that defines them.
     */
        std::string neural_model_path;
        int neural_threads = 0;
        double neural_target_rms = 0.111;
        NeuralVocoderSpec neural_spec;

        /*
         * Final tone/loudness preset for --polish and --feature-plan. It is
         * synthesis-only and intentionally excluded from the analysis
         * fingerprint; "none" disables the master chain.
         */
        std::string master_preset = "warm";

        /*
         * Voice color default for the voicebank. Style is synthesis-only and
         * intentionally excluded from the analysis fingerprint; UST Flags can
         * override it per note.
         */
        VoiceStyleKind style_default = VoiceStyleKind::natural;
        double style_amount = 1.0;
        double style_morph_ms = 0.0;

        /*
     * Hash of every setting that changes WORLD analysis. It is stored in
     * .lessaudio files so a configuration change invalidates cached models.
     */
        [[nodiscard]] std::string fingerprint() const;
    };

}// namespace lessampler

#endif//LESSAMPLER_LESSCONFIGURE_H
