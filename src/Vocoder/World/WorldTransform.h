/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_AUDIOPROCESS_H
#define LESSAMPLER_AUDIOPROCESS_H

#include <span>

#include "Core/RenderParams.h"
#include "Vocoder/World/WorldModel.h"

namespace lessampler {

    class WorldTransform {
    public:
        WorldTransform(const WorldModel &audioModel, RenderParams shine);

        [[nodiscard]] const WorldModel &GetTransAudioModel() const;

    private:
        WorldModel transAudioModel{};
        RenderParams shine;
        double source_average_f0_ = 0.0;

    private:
        void PitchEqualizing(std::span<double> f0) const;

        [[nodiscard]] double GetAvgFreq(std::span<const double> f0) const;

        void TimeStretch(const WorldModel &audioModel, std::span<const double> f0);
    };

}// namespace lessampler

#endif//LESSAMPLER_AUDIOPROCESS_H
