/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "SourceFingerprint.h"

#include <system_error>

namespace lessampler {

    SourceFingerprint FingerprintSource(const std::filesystem::path &path) {
        std::error_code error;
        const auto size = std::filesystem::file_size(path, error);
        if (error) {
            return {};
        }

        const auto mtime = std::filesystem::last_write_time(path, error);
        if (error) {
            return {size, 0};
        }

        return {size, static_cast<std::int64_t>(mtime.time_since_epoch().count())};
    }

}// namespace lessampler
