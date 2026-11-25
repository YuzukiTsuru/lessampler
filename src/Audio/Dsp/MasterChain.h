/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_DSP_MASTERCHAIN_H
#define LESSAMPLER_DSP_MASTERCHAIN_H

#include <span>
#include <string_view>

namespace lessampler {

    struct MasterOptions {
        bool enabled = true;
        double highpass_hz = 0.0;
        double low_shelf_hz = 0.0;
        double low_shelf_db = 0.0;
        double presence_hz = 0.0;
        double presence_db = 0.0;
        double compressor_threshold_db = 0.0;
        double compressor_ratio = 1.0;
        double compressor_attack_ms = 10.0;
        double compressor_release_ms = 120.0;
        double compressor_makeup_db = 0.0;
        double saturation = 0.0;
        double target_rms_db = 0.0;
        double ceiling_db = -1.0;
        double limiter_lookahead_ms = 5.0;
        double limiter_release_ms = 50.0;

        [[nodiscard]] static MasterOptions Preset(std::string_view name);
    };

    class MasterChain {
    public:
        /*
         * Apply the complete master chain to an interleaved mix in place.
         * Natural is a transparent limiter; warm/power add tone and density.
         */
        static void Process(std::span<double> interleaved, int sample_rate, int channels,
                            const MasterOptions &options);
    };

}// namespace lessampler

#endif//LESSAMPLER_DSP_MASTERCHAIN_H
