/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Audio/SampleSource.h"

#include <utility>

namespace lessampler {

    SampleSource::SampleSource(std::filesystem::path path)
        : path_(std::move(path)), info_(WavIO::ProbeWav(path_)), fingerprint_(FingerprintSource(path_)) {}

    std::span<const double> SampleSource::samples() const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!pcm_) {
            pcm_ = WavIO::ReadWav(path_);
        }
        return pcm_->samples;
    }

}// namespace lessampler
