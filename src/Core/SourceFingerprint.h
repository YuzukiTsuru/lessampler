/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_SOURCEFINGERPRINT_H
#define LESSAMPLER_SOURCEFINGERPRINT_H

#include <cstdint>
#include <filesystem>

namespace lessampler {

    /*
 * Identifies a source file by its size and modification time. Cache formats
 * store this so a changed WAV invalidates derived data without hashing the
 * entire file.
 */
    struct SourceFingerprint {
        std::uint64_t size = 0;
        std::int64_t mtime = 0;

        [[nodiscard]] bool valid() const noexcept { return size != 0; }

        friend bool operator==(const SourceFingerprint &, const SourceFingerprint &) = default;
    };

    [[nodiscard]] SourceFingerprint FingerprintSource(const std::filesystem::path &path);

}// namespace lessampler

#endif//LESSAMPLER_SOURCEFINGERPRINT_H
