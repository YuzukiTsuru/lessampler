/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Vocoder/World/WorldModelStore.h"

#include <span>
#include <string>
#include <vector>

#include "Audio/AutoAMP.h"
#include "Core/Log.h"
#include "Vocoder/World/WorldModelBuilder.h"
#include "Vocoder/World/WorldModelCache.h"

namespace lessampler {

    WorldModel LoadOrBuildWorldModel(const SampleSource &source, const Config &configure, int max_workers) {
        WorldModelCache cache(source.path());
        if (cache.CheckAudioModel(configure)) {
            try {
                return cache.ReadAudioModel(configure);
            } catch (const std::exception &error) {
                LOG_WARN << "Cannot read WORLD model cache, rebuilding: " + std::string(error.what());
            }
        }

        std::span<const double> samples = source.samples();
        std::vector<double> amplified;
        if (configure.model_amp != 0.0) {
            amplified.assign(samples.begin(), samples.end());
            AutoAMP::ApplyInPlace(amplified, configure.model_amp);
            samples = amplified;
        }

        auto model = BuildWorldModel(samples, source.info().sample_rate, configure, max_workers);
        try {
            WorldModelCache writer(source.path(), model, configure);
            writer.SaveAudioModel();
        } catch (const std::exception &error) {
            LOG_WARN << "Cannot save WORLD model cache: " + std::string(error.what());
        }
        return model;
    }

}// namespace lessampler
