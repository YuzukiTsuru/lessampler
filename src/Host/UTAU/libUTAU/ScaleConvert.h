/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_SCALECONVERT_H
#define LESSAMPLER_SCALECONVERT_H

#include <expected>
#include <string>
#include <string_view>

namespace lessampler {

    class ScaleConvert {
    public:
        explicit ScaleConvert(std::string_view scale_name);

        [[nodiscard]] std::expected<double, std::string> GetScaleNum() const;

    private:
        static std::expected<double, std::string> ScaleConvertToDouble(std::string_view scale_name);

        std::expected<double, std::string> scale_num_;
    };

}// namespace lessampler

#endif//LESSAMPLER_SCALECONVERT_H
