/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_PREPROCESS_H
#define LESSAMPLER_PREPROCESS_H

#include <span>
#include <vector>

#include "Core/PreprocessOptions.h"

namespace lessampler {

    /*
     * Source cleanup before F0 and spectral analysis:
     *
     *   high-pass -> click/pop repair -> spectral denoise
     *
     * The input is not modified; a new buffer is returned. When preprocessing
     * is disabled the input is copied unchanged.
     */
    [[nodiscard]] std::vector<double> PreprocessAudio(std::span<const double> samples,
                                                      int sample_rate,
                                                      const PreprocessOptions &options);

}// namespace lessampler

#endif//LESSAMPLER_PREPROCESS_H
