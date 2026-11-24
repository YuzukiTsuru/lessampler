/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_CONSTANTS_H
#define LESSAMPLER_CONSTANTS_H

#include <string_view>

namespace lessampler {
    namespace constants {

        inline constexpr std::string_view config_file_name = "lessampler.fex";
        inline constexpr std::string_view voicebank_config_file_name = "lessvoice.fex";

    }// namespace constants
}// namespace lessampler

#endif//LESSAMPLER_CONSTANTS_H
