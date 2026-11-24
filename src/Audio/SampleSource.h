/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_SAMPLESOURCE_H
#define LESSAMPLER_SAMPLESOURCE_H

#include <filesystem>
#include <mutex>
#include <optional>
#include <span>

#include "Audio/WavIO.h"
#include "Core/AudioInfo.h"
#include "Core/SourceFingerprint.h"

namespace lessampler {

    /*
 * A voicebank sample with header information available immediately and PCM
 * decoded on first use. Backends that can answer from an analysis cache never
 * touch the PCM.
 */
    class SampleSource {
    public:
        explicit SampleSource(std::filesystem::path path);

        [[nodiscard]] const std::filesystem::path &path() const noexcept { return path_; }

        [[nodiscard]] const WavInfo &info() const noexcept { return info_; }

        [[nodiscard]] SourceFingerprint fingerprint() const noexcept { return fingerprint_; }

        [[nodiscard]] AudioInfo audio_info(double frame_period) const noexcept {
            return AudioInfo{info_.sample_rate, info_.frames, frame_period};
        }

        [[nodiscard]] std::span<const double> samples() const;

    private:
        std::filesystem::path path_;
        WavInfo info_;
        SourceFingerprint fingerprint_;
        mutable std::mutex mutex_;
        mutable std::optional<WavData> pcm_;
    };

}// namespace lessampler

#endif//LESSAMPLER_SAMPLESOURCE_H
