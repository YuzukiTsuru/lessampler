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

#include "Config/VocoderKind.h"

namespace lessampler {

    class Config {
    public:
        enum class F0_MODE {
            F0_MODE_UNKNOWN = 0,
            F0_MODE_HARVEST = 1,
            F0_MODE_DIO = 2,
        };

        std::string version;
        bool debug_mode = false;
        VocoderKind vocoder = VocoderKind::world;
        double model_amp = 0.85;
        double audio_model_frame_period = (1000.0 * 256 / 44100);
        bool custom_fft_size = false;
        int fft_size = 1024;
        F0_MODE f0_mode = F0_MODE::F0_MODE_HARVEST;
        int f0_speed = 1;
        double f0_dio_floor = 40.0;
        double f0_harvest_floor = 40.0;
        double f0_cheap_trick_floor = 71.0;
        double f0_allow_range = 0.1;
        double ap_threshold = 0.10;

        /*
     * Optional neural vocoder. The model is an NHVSing-compatible ONNX graph
     * (mel/f0/uv -> waveform). Paths are resolved relative to the config file
     * that defines them.
     */
        std::string neural_model_path;
        int neural_threads = 0;
        double neural_target_rms = 0.111;

        /*
     * Hash of every setting that changes WORLD analysis. It is stored in
     * .lessaudio files so a configuration change invalidates cached models.
     */
        [[nodiscard]] std::string fingerprint() const;

        [[nodiscard]] static std::string_view f0_mode_name(F0_MODE mode);

        [[nodiscard]] static F0_MODE parse_f0_mode(std::string_view name);
    };

}// namespace lessampler

#endif//LESSAMPLER_LESSCONFIGURE_H
