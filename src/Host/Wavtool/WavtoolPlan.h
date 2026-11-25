/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_WAVTOOL_WAVTOOLPLAN_H
#define LESSAMPLER_WAVTOOL_WAVTOOLPLAN_H

#include <cstddef>
#include <filesystem>
#include <vector>

#include "Audio/Envelope.h"

namespace lessampler::wavtool {

    struct WavtoolPlanNote {
        std::filesystem::path input;
        double position_ms = 0.0;
        double skip_ms = 0.0;
        double length_ms = 0.0;
        Envelope envelope;
    };

    struct WavtoolPlan {
        std::filesystem::path output;
        int sample_rate = 0;
        int channels = 0;
        double peak_normalize = 0.0;
        std::vector<WavtoolPlanNote> notes;

        [[nodiscard]] static WavtoolPlan Parse(const std::filesystem::path &path);
        static void Run(const WavtoolPlan &plan);
    };

}// namespace lessampler::wavtool

#endif//LESSAMPLER_WAVTOOL_WAVTOOLPLAN_H
