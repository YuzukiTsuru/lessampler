/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "VocoderBackend.h"

#include "Utils/exception.h"
#include "WorldBackend.h"

#if defined(LESSAMPLER_ENABLE_LLSM)
#include "LlsmBackend.h"
#endif

namespace lessbackend {

    std::unique_ptr<VocoderBackend> CreateVocoderBackend(VocoderKind kind) {
        switch (kind) {
            case VocoderKind::world:
                return CreateWorldBackend();
            case VocoderKind::llsm:
#if defined(LESSAMPLER_ENABLE_LLSM)
                return CreateLlsmBackend();
#else
                throw parameter_error("The LLSM backend is not enabled in this build");
#endif
        }
        throw parameter_error("Unknown vocoder backend");
    }

}// namespace lessbackend
