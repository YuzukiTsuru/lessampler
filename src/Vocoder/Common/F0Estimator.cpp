/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "F0Estimator.h"

#include <filesystem>
#include <format>
#include <string>
#include <system_error>
#include <utility>

#include "Core/Exception.h"
#include "Core/Log.h"
#include "Vocoder/Common/WorldF0Estimator.h"

#if defined(LESSAMPLER_ENABLE_LLSM)
#include "Vocoder/Common/LlsmF0Estimator.h"
#endif

#if defined(LESSAMPLER_ENABLE_NEURAL)
#include "Vocoder/Common/RmvpeF0Estimator.h"
#endif

namespace lessampler {

    namespace {

        bool neural_model_available(const Config &configure) {
            if (configure.f0_neural_model_path.empty()) {
                return false;
            }
            std::error_code error;
            const bool regular = std::filesystem::is_regular_file(configure.f0_neural_model_path, error);
            return !error && regular;
        }

    }// namespace

    bool F0EstimatorCompiled(F0EstimatorKind kind) noexcept {
        switch (kind) {
            case F0EstimatorKind::harvest:
            case F0EstimatorKind::dio:
            case F0EstimatorKind::auto_select:
                return true;
            case F0EstimatorKind::llsm:
#if defined(LESSAMPLER_ENABLE_LLSM)
                return true;
#else
                return false;
#endif
            case F0EstimatorKind::neural:
#if defined(LESSAMPLER_ENABLE_NEURAL)
                return true;
#else
                return false;
#endif
        }
        return false;
    }

    F0EstimatorKind DefaultF0EstimatorFor(VocoderKind backend) noexcept {
        switch (backend) {
            case VocoderKind::llsm:
                return F0EstimatorKind::llsm;
            case VocoderKind::neural:
                return F0EstimatorKind::neural;
            case VocoderKind::world:
            default:
                return F0EstimatorKind::harvest;
        }
    }

    F0EstimatorKind ResolveF0EstimatorKind(const Config &configure, VocoderKind backend) {
        if (configure.f0_estimator != F0EstimatorKind::auto_select) {
            return configure.f0_estimator;
        }

        const auto kind = DefaultF0EstimatorFor(backend);
        if (!F0EstimatorCompiled(kind)) {
            LOG_WARN << std::format(
                    "F0 estimator '{}' is not enabled in this build; using 'harvest'",
                    F0EstimatorKindName(kind));
            return F0EstimatorKind::harvest;
        }
        if (kind == F0EstimatorKind::neural && !neural_model_available(configure)) {
            LOG_WARN << "automatic neural F0 has no usable [f0] neural_model_path; using 'harvest'";
            return F0EstimatorKind::harvest;
        }
        return kind;
    }

    Config ResolveF0EstimatorConfig(const Config &configure, VocoderKind backend) {
        Config resolved = configure;
        resolved.f0_estimator = ResolveF0EstimatorKind(configure, backend);
        return resolved;
    }

    std::unique_ptr<F0Estimator> CreateF0Estimator(F0EstimatorKind kind, const Config &configure) {
        const auto resolved = kind == F0EstimatorKind::auto_select
                                      ? ResolveF0EstimatorKind(configure, configure.vocoder)
                                      : kind;

        switch (resolved) {
            case F0EstimatorKind::harvest:
            case F0EstimatorKind::dio:
                return std::make_unique<WorldF0Estimator>(resolved);
            case F0EstimatorKind::llsm:
#if defined(LESSAMPLER_ENABLE_LLSM)
                return std::make_unique<LlsmF0Estimator>();
#else
                throw parameter_error("The LLSM F0 estimator is not enabled in this build");
#endif
            case F0EstimatorKind::neural:
#if defined(LESSAMPLER_ENABLE_NEURAL)
                if (configure.f0_neural_model_path.empty()) {
                    throw parameter_error("The neural F0 estimator requires [f0] neural_model_path");
                }
                if (!neural_model_available(configure)) {
                    throw parameter_error(std::format(
                            "RMVPE model not found: {}", configure.f0_neural_model_path));
                }
                return std::make_unique<RmvpeF0Estimator>(configure);
#else
                throw parameter_error("The neural F0 estimator is not enabled in this build");
#endif
            case F0EstimatorKind::auto_select:
            default:
                throw parameter_error("The F0 estimator is unresolved");
        }
    }

    std::string AvailableF0Estimators() {
        std::string estimators = "harvest dio";
#if defined(LESSAMPLER_ENABLE_LLSM)
        estimators += " llsm";
#endif
#if defined(LESSAMPLER_ENABLE_NEURAL)
        estimators += " neural";
#endif
        return estimators;
    }

}// namespace lessampler
