/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_RENDERENGINE_H
#define LESSAMPLER_RENDERENGINE_H

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "Audio/SampleSource.h"
#include "Config/Config.h"
#include "Config/ConfigResolver.h"
#include "Core/RenderParams.h"
#include "Engine/LruCache.h"
#include "Vocoder/VocoderBackend.h"

namespace lessampler {

    struct AnalysisKey {
        VocoderKind kind = VocoderKind::world;
        std::uint32_t version = 0;
        std::string config_fingerprint;
        std::string source_path;
        SourceFingerprint source;

        friend bool operator==(const AnalysisKey &, const AnalysisKey &) = default;
    };

    struct AnalysisKeyHash {
        std::size_t operator()(const AnalysisKey &key) const noexcept;
    };

    struct EngineStats {
        std::size_t analysis_hits = 0;
        std::size_t analysis_misses = 0;
        std::size_t cached_analyses = 0;
    };

    struct EngineOptions {
        std::size_t max_cached_analyses = 16;
        int max_threads = 0;
    };

    /*
 * Orchestrates a render independently of the UTAU/CLI host:
 *
 *   sample source -> effective config -> backend.Prepare (disk cache aware)
 *                 -> in-memory analysis LRU
 *                 -> backend.Synthesize -> AutoAMP
 *
 * One engine can render many notes in a session and reuse note-independent
 * analyses without touching disk again.
 */
    class RenderEngine {
    public:
        explicit RenderEngine(Config global_config, EngineOptions options = {});

        [[nodiscard]] backend::RenderResult Render(const SampleSource &source, const RenderParams &params);

        [[nodiscard]] backend::RenderResult Render(const std::filesystem::path &sample_path,
                                                   const RenderParams &params);

        [[nodiscard]] EngineStats stats() const;

        void ClearAnalysisCache();

        [[nodiscard]] const ConfigResolver &configs() const noexcept { return configs_; }

    private:
        [[nodiscard]] backend::VocoderBackend &backend_for(VocoderKind kind);

        [[nodiscard]] backend::AnalysisPtr analysis_for(const SampleSource &source, const Config &configure);

        ConfigResolver configs_;
        EngineOptions options_;

        std::mutex backend_mutex_;
        std::unordered_map<VocoderKind, std::unique_ptr<backend::VocoderBackend>> backends_;

        mutable std::mutex cache_mutex_;
        LruCache<AnalysisKey, backend::AnalysisPtr, AnalysisKeyHash> analyses_;
        mutable EngineStats stats_;
    };

}// namespace lessampler

#endif//LESSAMPLER_RENDERENGINE_H
