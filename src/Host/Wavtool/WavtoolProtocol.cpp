/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Host/Wavtool/WavtoolProtocol.h"

#include <array>
#include <charconv>
#include <cmath>
#include <format>
#include <ostream>
#include <string>
#include <string_view>
#include <system_error>

#include "Core/Exception.h"

namespace lessampler::wavtool {

    namespace {

        std::string_view trim(std::string_view text) {
            while (!text.empty() && (text.front() == ' ' || text.front() == '\t' || text.front() == '\r')) {
                text.remove_prefix(1);
            }
            while (!text.empty() && (text.back() == ' ' || text.back() == '\t' || text.back() == '\r')) {
                text.remove_suffix(1);
            }
            return text;
        }

        double parse_number(std::string_view text, std::string_view name) {
            text = trim(text);
            if (!text.empty() && text.front() == '+') {
                text.remove_prefix(1);
            }
            if (text.empty()) {
                throw parameter_error("missing " + std::string(name));
            }
            double value{};
            const auto *begin = text.data();
            const auto *end = begin + text.size();
            const auto result = std::from_chars(begin, end, value);
            if (result.ec != std::errc{} || result.ptr != end || !std::isfinite(value)) {
                throw parameter_error("invalid " + std::string(name) + ": " + std::string(text));
            }
            return value;
        }

        double optional_number(std::span<const std::string_view> args, std::size_t index, double fallback) {
            if (index >= args.size() || trim(args[index]).empty()) {
                return fallback;
            }
            return parse_number(args[index], "wavtool argument");
        }

    }// namespace

    double WavtoolProtocol::ParseDuration(std::string_view text) {
        text = trim(text);
        if (text.empty()) {
            return 0.0;
        }

        const auto at = text.find('@');
        if (at == std::string_view::npos) {
            return parse_number(text, "duration");
        }

        const double ticks = parse_number(text.substr(0, at), "duration ticks");
        const std::string_view tempo_text = text.substr(at + 1);

        double tempo{};
        const auto *tempo_begin = tempo_text.data();
        const auto *tempo_end = tempo_begin + tempo_text.size();
        const auto tempo_result = std::from_chars(tempo_begin, tempo_end, tempo);
        if (tempo_result.ec != std::errc{} || !std::isfinite(tempo) || !(tempo > 0.0)) {
            throw parameter_error("invalid duration tempo: " + std::string(tempo_text));
        }

        double delta = 0.0;
        const std::string_view remainder(tempo_result.ptr, tempo_end);
        if (!remainder.empty()) {
            if (remainder.front() != '+' && remainder.front() != '-') {
                throw parameter_error("invalid duration delta: " + std::string(remainder));
            }
            delta = parse_number(remainder, "duration delta");
        }

        return ticks * 60000.0 / (tempo * 480.0) + delta;
    }

    WavtoolRequest WavtoolProtocol::Parse(std::span<const std::string_view> args) {
        if (args.size() < 5) {
            throw parameter_error("too few arguments for the wavtool protocol");
        }

        WavtoolRequest request;
        request.output = std::filesystem::path(trim(args[1]));
        request.input = std::filesystem::path(trim(args[2]));
        request.skip_ms = ParseDuration(args[3]);
        request.length_ms = ParseDuration(args[4]);

        // p1,p2,p3,v1,v2,v3,v4,ove,p4,p5,v5
        constexpr std::array<double, 11> defaults{0.0, 5.0, 35.0, 0.0, 100.0, 100.0, 0.0, 0.0, 0.0, 10.0, 100.0};
        std::array<double, 11> values = defaults;
        for (std::size_t i = 0; i < values.size(); ++i) {
            values[i] = optional_number(args, 5 + i, defaults[i]);
        }

        request.overlap_ms = values[7];
        const std::string envelope_text = std::format("{},{},{},{},{},{},{},%,{},{},{}",
                                                      values[0], values[1], values[2],
                                                      values[3], values[4], values[5], values[6],
                                                      values[8], values[9], values[10]);
        request.envelope = Envelope::Parse(envelope_text);
        return request;
    }

    void WavtoolProtocol::PrintUsage(std::ostream &stream) {
        stream << "Usage:\n"
                  "  lesswavtool <out.wav> <in.wav> <skip_ms> <length_ms>\n"
                  "               [p1 p2 p3 v1 v2 v3 v4 overlap p4 p5 v5]\n"
                  "  lesswavtool --plan <plan.toml>\n";
    }

}// namespace lessampler::wavtool
