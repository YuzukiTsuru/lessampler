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

#include <filesystem>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

#include "Backend/VocoderKind.h"
#include "Config/Config.h"
#include "Core/RenderParams.h"

namespace lessbackend {

    struct RenderRequest {
        const std::filesystem::path &input_path;
        // PCM samples for backends that analyze the waveform directly. A
        // backend that only uses a cached model may receive an empty span.
        std::span<const double> samples;
        int fs = 0;
        const RenderParams &shine;
        const Config &configure;
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

        [[nodiscard]] virtual RenderResult Render(const RenderRequest &request) = 0;
    };

    [[nodiscard]] std::unique_ptr<VocoderBackend> CreateVocoderBackend(VocoderKind kind);

}// namespace lessbackend

#endif//LESSAMPLER_VOCODERBACKEND_H
