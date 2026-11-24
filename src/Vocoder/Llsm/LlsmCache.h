/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_LLSMCACHE_H
#define LESSAMPLER_LLSMCACHE_H

#include <filesystem>
#include <memory>

#include "Config/Config.h"

extern "C" {
#include "llsm.h"
}

namespace lessampler {

    namespace backend {

        struct LlsmChunkDeleter {
            void operator()(llsm_chunk *value) const;
        };

        using LlsmChunkPtr = std::unique_ptr<llsm_chunk, LlsmChunkDeleter>;

        /*
     * Layer-1 cache for the LLSM backend. The analysis only depends on the
     * source waveform and the F0 settings, not on the UTAU note parameters, so
     * every note rendered from the same sample can reuse it.
     */
        [[nodiscard]] std::filesystem::path LlsmCachePath(const std::filesystem::path &input_path);

        /*
     * Returns nullptr when the cache is missing, stale, or malformed.
     */
        [[nodiscard]] LlsmChunkPtr LoadLlsmCache(const std::filesystem::path &input_path,
                                                 const Config &configure);

        /*
     * Best-effort save. Throws only for an actual I/O failure; callers may
     * treat a failed cache write as non-fatal.
     */
        void SaveLlsmCache(const std::filesystem::path &input_path,
                           const Config &configure, const llsm_chunk *chunk);

    }// namespace backend

}// namespace lessampler

#endif//LESSAMPLER_LLSMCACHE_H
