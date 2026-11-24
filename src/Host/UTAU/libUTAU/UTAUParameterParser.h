/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_UTAUPARAMETERPARSER_H
#define LESSAMPLER_UTAUPARAMETERPARSER_H

#include <span>
#include <string_view>

#include "libUTAU.h"

namespace lessampler {

    class UTAUParameterParser {
    public:
        explicit UTAUParameterParser(std::span<const std::string_view> args);

        [[nodiscard]] UTAUPara get_para() const;

    private:
        UTAUPara utauPara;
    };

}// namespace lessampler

#endif//LESSAMPLER_UTAUPARAMETERPARSER_H
