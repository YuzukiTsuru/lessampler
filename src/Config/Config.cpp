/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Config.h"

#include <cstdint>
#include <filesystem>
#include <format>
#include <string>
#include <system_error>

#include "Core/SourceFingerprint.h"

namespace lessampler {

    namespace {

        // Bump when an estimator implementation changes its output without a
        // config field changing, so persisted analyses are invalidated.
        constexpr std::string_view f0_estimator_version = "f0-v1";

        std::uint64_t fnv1a(std::string_view data) {
            std::uint64_t hash = 1469598103934665603ULL;
            for (unsigned char c: data) {
                hash ^= c;
                hash *= 1099511628211ULL;
            }
            return hash;
        }

        std::string f0_model_identity(const Config &configure) {
            if (configure.f0_neural_model_path.empty()) {
                return {};
            }

            std::error_code error;
            const auto canonical = std::filesystem::weakly_canonical(configure.f0_neural_model_path, error);
            const std::filesystem::path path = error
                                                       ? std::filesystem::path(configure.f0_neural_model_path).lexically_normal()
                                                       : canonical;
            const auto source = FingerprintSource(path);
            return std::format("{}|{}|{}", path.string(), source.size, source.mtime);
        }

        std::string neural_f0_identity(const Config &configure) {
            return std::format("{}|{}|{}|{}",
                               configure.f0_neural_threshold,
                               configure.f0_neural_floor,
                               configure.f0_neural_ceil,
                               f0_model_identity(configure));
        }

    }// namespace

    std::string Config::fingerprint() const {
        // The vocoder backend is deliberately not part of the fingerprint: the
        // cached .lessaudio holds WORLD analysis only, so switching between WORLD
        // and LLSM must not invalidate an existing WORLD cache. The F0 estimator
        // is part of the fingerprint because it changes the stored F0 track.
        std::string estimator_identity = std::string(f0_estimator_version) + "|";
        if (f0_estimator == F0EstimatorKind::auto_select) {
            estimator_identity += std::format("auto:{}", VocoderKindName(vocoder));
            if (vocoder == VocoderKind::neural) {
                estimator_identity += "|" + neural_f0_identity(*this);
            }
        } else if (f0_estimator == F0EstimatorKind::neural) {
            estimator_identity += "neural|" + neural_f0_identity(*this);
        } else {
            estimator_identity += F0EstimatorKindName(f0_estimator);
        }

        std::string data = std::format(
                "{}|{}|{}|{}|{}|{}|{}|{}|{}|{}|{}",
                audio_model_frame_period,
                fft_size,
                model_amp,
                f0_speed,
                f0_dio_floor,
                f0_harvest_floor,
                f0_ceil,
                f0_cheap_trick_floor,
                f0_allow_range,
                ap_threshold,
                estimator_identity);
        // Keep the default (disabled) fingerprint identical to the previous
        // format. Only an active preprocessor changes the analysis input.
        if (preprocess.enabled) {
            data += std::format("|preprocess:{}|{}|{}|{}|{}",
                                preprocess.highpass_hz,
                                preprocess.declick,
                                preprocess.declick_threshold,
                                preprocess.denoise,
                                preprocess.denoise_db);
        }
        return std::format("{:016x}", fnv1a(data));
    }

}// namespace lessampler
