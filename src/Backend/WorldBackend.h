/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_WORLDBACKEND_H
#define LESSAMPLER_WORLDBACKEND_H

#include <memory>

#include "VocoderBackend.h"

namespace lessbackend {

    [[nodiscard]] std::unique_ptr<VocoderBackend> CreateWorldBackend();

}// namespace lessbackend

#endif//LESSAMPLER_WORLDBACKEND_H
