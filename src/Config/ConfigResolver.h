/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_CONFIGRESOLVER_H
#define LESSAMPLER_CONFIGRESOLVER_H

#include <filesystem>
#include <mutex>
#include <string>
#include <unordered_map>

#include "Config/Config.h"

namespace lessampler {

    /*
 * Resolves the effective config for a sample. The global config is
 * overridden by <voicebank>/lessvoice.less, and each voicebank directory is
 * parsed at most once per session.
 */
    class ConfigResolver {
    public:
        explicit ConfigResolver(Config global);

        [[nodiscard]] Config Resolve(const std::filesystem::path &sample_path) const;

        [[nodiscard]] const Config &global() const noexcept { return global_; }

    private:
        Config global_;
        mutable std::mutex mutex_;
        mutable std::unordered_map<std::string, Config> voicebank_cache_;
    };

}// namespace lessampler

#endif//LESSAMPLER_CONFIGRESOLVER_H
