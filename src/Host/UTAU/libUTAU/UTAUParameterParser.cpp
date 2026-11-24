/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "UTAUParameterParser.h"

#include <charconv>
#include <cmath>
#include <format>
#include <string>

#include "ScaleConvert.h"
#include "Core/Exception.h"

namespace {

    template<typename T>
    T parse_number(std::string_view text, std::string_view name) {
        T value{};
        const auto *begin = text.data();
        const auto *end = begin + text.size();
        const auto result = std::from_chars(begin, end, value);
        if (result.ec != std::errc{} || result.ptr != end) {
            throw parameter_error(std::format("Invalid {}: {}", name, text));
        }
        return value;
    }

    std::string_view argument(std::span<const std::string_view> args, std::size_t index) {
        return index < args.size() ? args[index] : std::string_view{};
    }

}// namespace

UTAUParameterParser::UTAUParameterParser(std::span<const std::string_view> args) {
    if (args.size() < 4) {
        throw parameter_error("too few arguments for the UTAU resampler protocol");
    }

    utauPara.local_name = std::string(args[0]);
    utauPara.input_file_name = std::string(args[1]);
    utauPara.output_file_name = std::string(args[2]);
    utauPara.scale_name = std::string(args[3]);

    if (args.size() > 4) {
        utauPara.time_percent = parse_number<int>(args[4], "time percent");
        utauPara.velocity = std::pow(2.0, utauPara.time_percent / 100.0 - 1.0);
    } else {
        utauPara.velocity = 1.0;
    }

    if (args.size() > 5) {
        utauPara.flags = std::string(args[5]);
    }

    if (args.size() > 6) {
        utauPara.offset = parse_number<double>(args[6], "offset");
    }

    if (args.size() > 7) {
        utauPara.required_length = parse_number<double>(args[7], "required length");
    }

    if (args.size() > 8) {
        utauPara.first_half_fixed_part = parse_number<double>(args[8], "fixed length");
    }

    if (args.size() > 9) {
        utauPara.last_unused_part = parse_number<double>(args[9], "last unused length");
    }

    if (args.size() > 10) {
        utauPara.volumes = parse_number<double>(args[10], "volume") * 0.01;
    } else {
        utauPara.volumes = 1.0;
    }

    if (args.size() > 11) {
        utauPara.modulation = parse_number<int>(args[11], "modulation");
    }

    if (args.size() > 12) {
        utauPara.tempo = std::string(args[12]);
        std::string_view tempo = argument(args, 12);
        if (tempo.starts_with('!')) {
            tempo.remove_prefix(1);
        }
        if (tempo.empty()) {
            utauPara.tempo_num = 120;
        } else {
            utauPara.tempo_num = static_cast<int>(std::lround(parse_number<double>(tempo, "tempo")));
            if (utauPara.tempo_num <= 0) {
                throw parameter_error("Tempo must be greater than zero");
            }
        }
    } else {
        utauPara.tempo_num = 120;
    }

    if (args.size() > 13) {
        utauPara.is_custom_pitch = true;
        utauPara.pitch = std::string(args[13]);
    }

    ScaleConvert scale_convert(utauPara.scale_name);
    const auto scale = scale_convert.GetScaleNum();
    if (!scale) {
        throw parameter_error("Invalid note name '" + utauPara.scale_name + "': " + scale.error());
    }
    utauPara.scale_num = *scale;
}

UTAUPara UTAUParameterParser::get_para() const {
    return utauPara;
}
