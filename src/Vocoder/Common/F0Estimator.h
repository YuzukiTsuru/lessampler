/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_F0ESTIMATOR_H
#define LESSAMPLER_F0ESTIMATOR_H

#include <memory>
#include <string>

#include "Config/Config.h"
#include "Config/VocoderKind.h"
#include "Vocoder/Common/F0EstimatorTypes.h"

namespace lessampler {

    /*
     * F0 estimator factory. The concrete estimator changes the stored F0
     * track, so callers must resolve auto_select before computing a cache
     * fingerprint.
     */
    [[nodiscard]] bool F0EstimatorCompiled(F0EstimatorKind kind) noexcept;

    [[nodiscard]] F0EstimatorKind DefaultF0EstimatorFor(VocoderKind backend) noexcept;

    [[nodiscard]] F0EstimatorKind ResolveF0EstimatorKind(const Config &configure, VocoderKind backend);

    /*
     * Return a copy of configure whose f0_estimator is concrete. Use this
     * before LoadOrBuildWorldModel or any other analysis cache lookup.
     */
    [[nodiscard]] Config ResolveF0EstimatorConfig(const Config &configure, VocoderKind backend);

    /*
     * Throws parameter_error for an explicitly requested estimator that is not
     * available in this build or has no usable model. auto_select is resolved
     * against configure.vocoder as a defensive fallback.
     */
    [[nodiscard]] std::unique_ptr<F0Estimator> CreateF0Estimator(F0EstimatorKind kind, const Config &configure);

    /*
     * Space-separated list of the F0 estimators compiled into this binary.
     */
    [[nodiscard]] std::string AvailableF0Estimators();

}// namespace lessampler

#endif//LESSAMPLER_F0ESTIMATOR_H
