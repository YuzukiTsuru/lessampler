/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_WAVTOOL_WAVTOOLPROTOCOL_H
#define LESSAMPLER_WAVTOOL_WAVTOOLPROTOCOL_H

#include <filesystem>
#include <iosfwd>
#include <span>
#include <string_view>

#include "Audio/Envelope.h"

namespace lessampler::wavtool {

    struct WavtoolRequest {
        std::filesystem::path output;
        std::filesystem::path input;
        double skip_ms = 0.0;
        double length_ms = 0.0;
        Envelope envelope;
        double overlap_ms = 0.0;
    };

    /*
 * Parser for the classic UTAU wavtool argument list:
 *
 *   wavtool <out> <in> <offset> <length>
 *            <p1> <p2> <p3> <v1> <v2> <v3> <v4> <ove> <p4> <p5> <v5>
 *
 * Missing optional arguments use the UTAU envelope defaults.
 */
    class WavtoolProtocol {
    public:
        [[nodiscard]] static WavtoolRequest Parse(std::span<const std::string_view> args);

        /*
     * Parse plain milliseconds or the UTAU "ticks@tempo±delta" form.
     */
        [[nodiscard]] static double ParseDuration(std::string_view text);

        static void PrintUsage(std::ostream &stream);
    };

}// namespace lessampler::wavtool

#endif//LESSAMPLER_WAVTOOL_WAVTOOLPROTOCOL_H
