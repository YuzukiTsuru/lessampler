/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_VOCODERBACKEND_H
#define LESSAMPLER_VOCODERBACKEND_H

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "Audio/SampleSource.h"
#include "Config/Config.h"
#include "Config/VocoderKind.h"
#include "Core/RenderParams.h"

namespace lessampler {

    namespace backend {

        /*
     * Note-independent analysis result. The engine keeps these in memory and
     * backends persist them on disk; every UTAU note for the same sample can
     * reuse one instance.
     */
        class Analysis {
        public:
            virtual ~Analysis() = default;

            [[nodiscard]] virtual VocoderKind kind() const noexcept = 0;
        };

        using AnalysisPtr = std::shared_ptr<const Analysis>;

        struct AnalysisRequest {
            const SampleSource &source;
            const Config &configure;
            int max_workers = 0;
        };

        struct SynthesisRequest {
            const Analysis &analysis;
            const RenderParams &params;
        };

        struct RenderResult {
            std::vector<double> samples;
            int fs = 0;
        };

        class VocoderBackend {
        public:
            virtual ~VocoderBackend() = default;

            [[nodiscard]] virtual VocoderKind kind() const noexcept = 0;

            [[nodiscard]] virtual std::string_view name() const noexcept = 0;

            /*
         * Bump when the analysis semantics change. The engine includes this
         * in its in-memory cache key.
         */
            [[nodiscard]] virtual std::uint32_t analysis_version() const noexcept = 0;

            /*
         * Load a cached analysis or compute one. The PCM is loaded lazily by
         * the backend only when the cache misses.
         */
            [[nodiscard]] virtual AnalysisPtr Prepare(const AnalysisRequest &request) = 0;

            /*
         * Render one note from an analysis produced by this backend.
         */
            [[nodiscard]] virtual RenderResult Synthesize(const SynthesisRequest &request) = 0;
        };

        [[nodiscard]] std::unique_ptr<VocoderBackend> CreateVocoderBackend(VocoderKind kind);

        /*
         * Space-separated list of the backends compiled into this binary.
         */
        [[nodiscard]] std::string AvailableVocoderBackends();

    }// namespace backend

}// namespace lessampler

#endif//LESSAMPLER_VOCODERBACKEND_H
