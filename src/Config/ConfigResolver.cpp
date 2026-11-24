/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Config/ConfigResolver.h"

#include <system_error>
#include <utility>

#include "Config/VoicebankConfig.h"

namespace lessampler {

    namespace {

        std::string directory_key(const std::filesystem::path &sample_path) {
            std::error_code error;
            const auto directory = std::filesystem::is_directory(sample_path, error)
                                           ? sample_path
                                           : sample_path.parent_path();
            const auto base = directory.empty() ? std::filesystem::path(".") : directory;

            error.clear();
            const auto canonical = std::filesystem::weakly_canonical(base, error);
            return error ? base.lexically_normal().string() : canonical.string();
        }

    }// namespace

    ConfigResolver::ConfigResolver(Config global) : global_(std::move(global)) {}

    Config ConfigResolver::Resolve(const std::filesystem::path &sample_path) const {
        const auto key = directory_key(sample_path);

        std::lock_guard<std::mutex> lock(mutex_);
        if (const auto it = voicebank_cache_.find(key); it != voicebank_cache_.end()) {
            return it->second;
        }

        VoicebankConfig voicebank(key, global_);
        auto resolved = voicebank.GetConfig();
        voicebank_cache_.emplace(key, resolved);
        return resolved;
    }

}// namespace lessampler
