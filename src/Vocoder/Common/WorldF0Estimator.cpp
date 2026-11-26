/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "WorldF0Estimator.h"

#include <utility>
#include <vector>

#include <world/dio.h>
#include <world/harvest.h>
#include <world/stonemask.h>

#include "Config/Config.h"
#include "Core/Exception.h"

namespace lessampler {

    WorldF0Estimator::WorldF0Estimator(F0EstimatorKind kind) : kind_(kind) {
        if (kind_ != F0EstimatorKind::harvest && kind_ != F0EstimatorKind::dio) {
            throw parameter_error("WorldF0Estimator requires harvest or dio");
        }
    }

    std::string_view WorldF0Estimator::name() const noexcept {
        return F0EstimatorKindName(kind_);
    }

    F0Track WorldF0Estimator::Estimate(std::span<const double> x, int fs, const Config &configure) const {
        if (x.empty() || fs <= 0) {
            throw parameter_error("Empty audio input");
        }

        F0Track track;
        track.frame_period = configure.audio_model_frame_period;

        if (kind_ == F0EstimatorKind::dio) {
            DioOption option{};
            InitializeDioOption(&option);
            option.frame_period = track.frame_period;
            option.speed = configure.f0_speed;
            option.f0_floor = configure.f0_dio_floor;
            option.f0_ceil = configure.f0_ceil;
            option.allowed_range = configure.f0_allow_range;

            const int f0_length = GetSamplesForDIO(fs, static_cast<int>(x.size()), track.frame_period);
            track.f0.resize(static_cast<std::size_t>(f0_length));
            track.time_axis.resize(static_cast<std::size_t>(f0_length));
            std::vector<double> refined_f0(static_cast<std::size_t>(f0_length));

            Dio(x.data(), static_cast<int>(x.size()), fs, &option, track.time_axis.data(), track.f0.data());
            StoneMask(x.data(), static_cast<int>(x.size()), fs, track.time_axis.data(), track.f0.data(), f0_length,
                      refined_f0.data());
            track.f0 = std::move(refined_f0);
            return track;
        }

        HarvestOption option{};
        InitializeHarvestOption(&option);
        option.frame_period = track.frame_period;
        option.f0_floor = configure.f0_harvest_floor;
        option.f0_ceil = configure.f0_ceil;

        const int f0_length = GetSamplesForHarvest(fs, static_cast<int>(x.size()), track.frame_period);
        track.f0.resize(static_cast<std::size_t>(f0_length));
        track.time_axis.resize(static_cast<std::size_t>(f0_length));

        Harvest(x.data(), static_cast<int>(x.size()), fs, &option, track.time_axis.data(), track.f0.data());
        return track;
    }

}// namespace lessampler
