/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_FEATURETYPES_H
#define LESSAMPLER_FEATURETYPES_H

#include <filesystem>
#include <vector>

#include "Audio/Envelope.h"

namespace lessampler {

    /*
 * One note segment on the feature timeline. position_ms is the absolute
 * placement of the used segment; skip_ms/length_ms select it from the note
 * WAV, exactly like the wavtool plan.
 */
    struct FeatureSegment {
        std::filesystem::path input;
        double position_ms = 0.0;
        double skip_ms = 0.0;
        double length_ms = 0.0;
        Envelope envelope;
    };

    struct FeatureAssembleResult {
        std::vector<double> samples;
        int sample_rate = 0;
    };

}// namespace lessampler

#endif//LESSAMPLER_FEATURETYPES_H
