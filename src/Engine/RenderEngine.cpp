/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Engine/RenderEngine.h"

#include <functional>
#include <string>
#include <system_error>
#include <utility>

#include "Audio/AutoAMP.h"
#include "Audio/Dsp/Declick.h"
#include "Core/Exception.h"
#include "Core/Log.h"

namespace lessampler {

    namespace {

        void hash_combine(std::size_t &seed, std::size_t value) {
            seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
        }

        std::string source_identity(const std::filesystem::path &path) {
            std::error_code error;
            const auto absolute = std::filesystem::weakly_canonical(path, error);
            return error ? path.lexically_normal().string() : absolute.string();
        }

        /*
     * Backend-specific state that is not part of the WORLD analysis
     * fingerprint (for example, which neural model is loaded).
     */
        std::string backend_data_identity(const Config &configure) {
            if (configure.vocoder != VocoderKind::neural) {
                return {};
            }

            std::string identity = configure.neural_model_path;
            if (identity.empty()) {
                return identity;
            }
            identity += "|" + std::to_string(configure.neural_target_rms);

            const std::filesystem::path model_path(identity);
            std::error_code error;
            const auto size = std::filesystem::file_size(model_path, error);
            if (!error) {
                identity += "|" + std::to_string(size);
            }
            error.clear();
            const auto mtime = std::filesystem::last_write_time(model_path, error);
            if (!error) {
                identity += "|" + std::to_string(mtime.time_since_epoch().count());
            }
            return identity;
        }

    }// namespace

    std::size_t AnalysisKeyHash::operator()(const AnalysisKey &key) const noexcept {
        std::size_t seed = std::hash<int>{}(static_cast<int>(key.kind));
        hash_combine(seed, std::hash<std::uint32_t>{}(key.version));
        hash_combine(seed, std::hash<std::string>{}(key.config_fingerprint));
        hash_combine(seed, std::hash<std::string>{}(key.backend_data));
        hash_combine(seed, std::hash<std::string>{}(key.source_path));
        hash_combine(seed, std::hash<std::uint64_t>{}(key.source.size));
        hash_combine(seed, std::hash<std::int64_t>{}(key.source.mtime));
        return seed;
    }

    RenderEngine::RenderEngine(Config global_config, EngineOptions options)
        : configs_(std::move(global_config)), options_(options), analyses_(options.max_cached_analyses) {}

    backend::VocoderBackend &RenderEngine::backend_for(VocoderKind kind) {
        std::lock_guard<std::mutex> lock(backend_mutex_);
        if (const auto it = backends_.find(kind); it != backends_.end()) {
            return *it->second;
        }

        auto backend = backend::CreateVocoderBackend(kind);
        auto *raw = backend.get();
        backends_.emplace(kind, std::move(backend));
        return *raw;
    }

    backend::AnalysisPtr RenderEngine::analysis_for(const SampleSource &source, const Config &configure) {
        auto &backend = backend_for(configure.vocoder);
        const AnalysisKey key{
                configure.vocoder,
                backend.analysis_version(),
                configure.fingerprint(),
                backend_data_identity(configure),
                source_identity(source.path()),
                source.fingerprint(),
        };

        {
            std::lock_guard<std::mutex> lock(cache_mutex_);
            if (auto cached = analyses_.Get(key)) {
                ++stats_.analysis_hits;
                LOG_DEBUG << "Analysis memory cache hit";
                return *cached;
            }
            ++stats_.analysis_misses;
        }

        auto analysis = backend.Prepare(backend::AnalysisRequest{source, configure, options_.max_threads});
        if (!analysis || analysis->kind() != configure.vocoder) {
            throw parameter_error("Backend returned an invalid analysis");
        }

        {
            std::lock_guard<std::mutex> lock(cache_mutex_);
            analyses_.Put(key, analysis);
            stats_.cached_analyses = analyses_.size();
        }
        return analysis;
    }

    backend::RenderResult RenderEngine::Render(const SampleSource &source, const RenderParams &params) {
        const auto configure = configs_.Resolve(source.path());
        auto analysis = analysis_for(source, configure);
        auto &backend = backend_for(configure.vocoder);
        LOG_INFO << "Backend: " + std::string(backend.name());

        auto result = backend.Synthesize(backend::SynthesisRequest{*analysis, params});
        Declick::Apply(result.samples, result.fs);
        AutoAMP::ApplyInPlace(result.samples, params.volumes);
        return result;
    }

    backend::RenderResult RenderEngine::Render(const std::filesystem::path &sample_path,
                                               const RenderParams &params) {
        const SampleSource source(sample_path);
        return Render(source, params);
    }

    EngineStats RenderEngine::stats() const {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        return stats_;
    }

    void RenderEngine::ClearAnalysisCache() {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        analyses_.Clear();
        stats_.cached_analyses = 0;
    }

}// namespace lessampler
