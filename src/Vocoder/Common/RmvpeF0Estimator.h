/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_RMVPEF0ESTIMATOR_H
#define LESSAMPLER_RMVPEF0ESTIMATOR_H

#include <memory>

#include "Vocoder/Common/F0EstimatorTypes.h"
#include "Vocoder/Common/RmvpeDecode.h"
#include "Vocoder/Common/RmvpeModel.h"

namespace lessampler {

    /*
     * RMVPE F0 estimator: resample to 16 kHz, run the salience model, decode
     * 10 ms frames, then resample the track onto audio_model_frame_period.
     */
    class RmvpeF0Estimator final : public F0Estimator {
    public:
        explicit RmvpeF0Estimator(const Config &configure);

        [[nodiscard]] std::string_view name() const noexcept override;

        [[nodiscard]] F0Track Estimate(std::span<const double> x, int fs, const Config &configure) const override;

    private:
        std::shared_ptr<RmvpeModel> model_;
    };

}// namespace lessampler

#endif//LESSAMPLER_RMVPEF0ESTIMATOR_H
