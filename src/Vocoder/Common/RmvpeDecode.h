/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_RMVPEDECODE_H
#define LESSAMPLER_RMVPEDECODE_H

#include <cstddef>
#include <cstdint>
#include <vector>

#include "Vocoder/Common/F0EstimatorTypes.h"

namespace lessampler {

    class Config;

    struct RmvpeOutput {
        bool direct_f0 = false;
        std::size_t frames = 0;
        std::size_t bins = 0;
        std::vector<float> salience; // time-major frames * bins when direct_f0 is false
        std::vector<float> f0;       // frames when direct_f0 is true
        std::vector<std::uint8_t> uv;// 1 = unvoiced when direct_f0 is true

        [[nodiscard]] bool Empty() const noexcept {
            return frames == 0 || (direct_f0 ? f0.empty() : salience.empty());
        }
    };

    /*
     * Decode an RMVPE model output into its native 10 ms F0 track. Exposed
     * separately so the salience/direct-F0 decoders can be tested without an
     * ONNX model.
     */
    [[nodiscard]] F0Track DecodeRmvpeOutput(const RmvpeOutput &output, const Config &configure);

}// namespace lessampler

#endif//LESSAMPLER_RMVPEDECODE_H
