/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_NEURALPOLISHER_H
#define LESSAMPLER_NEURALPOLISHER_H

#include <span>
#include <vector>

#include "Config/Config.h"

namespace lessampler {

    struct NeuralPolishResult {
        std::vector<double> samples;
        int sample_rate = 0;
    };

    /*
 * Re-render an assembled song through the NHVSing-compatible vocoder.
 *
 * Unlike the per-note neural backend, this sees the whole timeline at once:
 * F0 and 128-bin mel are extracted from the mixed waveform, then the model
 * generates one continuous waveform. Notes that were rendered by separate
 * processes no longer reset phase at every boundary.
 */
    [[nodiscard]] NeuralPolishResult PolishWithNeural(std::span<const double> samples, int sample_rate,
                                                      const Config &configure);

}// namespace lessampler

#endif//LESSAMPLER_NEURALPOLISHER_H
