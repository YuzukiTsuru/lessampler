/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_DSP_COMPRESSOR_H
#define LESSAMPLER_DSP_COMPRESSOR_H

#include <span>

namespace lessampler {

    /*
     * Linked-channel feed-forward compressor. The detector uses the loudest
     * channel so a stereo image does not shift while gain changes.
     */
    class Compressor {
    public:
        void Configure(int sample_rate, double threshold_db, double ratio,
                       double attack_ms, double release_ms, double makeup_db);
        void Process(std::span<double> interleaved, int channels);

    private:
        double threshold_db_ = 0.0;
        double ratio_ = 1.0;
        double makeup_db_ = 0.0;
        double attack_coefficient_ = 0.0;
        double release_coefficient_ = 0.0;
        double envelope_ = 0.0;
    };

}// namespace lessampler

#endif//LESSAMPLER_DSP_COMPRESSOR_H
