/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "libUTAU.h"

#include <algorithm>
#include <format>
#include <limits>
#include <sstream>
#include <string_view>
#include <vector>

#include "Core/Exception.h"
#include "Core/Log.h"
#include "UTAUParameterParser.h"

namespace lessampler {

    namespace {

        std::vector<std::string_view> make_args(int argc, char **argv) {
            std::vector<std::string_view> args;
            args.reserve(static_cast<std::size_t>(argc));
            for (int i = 0; i < argc; ++i) {
                args.emplace_back(argv[i] == nullptr ? "" : argv[i]);
            }
            return args;
        }

    }// namespace

    libUTAU::libUTAU(std::span<const std::string_view> args) {
        if (args.size() < 4) {
            throw parameter_error("too few arguments for the UTAU resampler protocol");
        }

        UTAUParameterParser parser(args);
        utauPara = parser.get_para();
    }

    libUTAU::libUTAU(int argc, char **argv) : libUTAU(make_args(argc, argv)) {}

    void libUTAU::PrintUTAUPara() const {
        std::stringstream stream;
        stream << "\nProgram Name:           " << utauPara.local_name;
        stream << "\nIn File Name:           " << utauPara.input_file_name;
        stream << "\nOut File Name:          " << utauPara.output_file_name;
        stream << "\nScale Name:             " << utauPara.scale_name;
        stream << "\nScale Number:           " << utauPara.scale_num;
        stream << "\nTime Percent:           " << utauPara.time_percent;
        stream << "\nVelocity:               " << utauPara.velocity;
        stream << "\nFlags:                  " << utauPara.flags;
        stream << "\nOffset:                 " << utauPara.offset;
        stream << "\nHost Required Length:   " << utauPara.required_length;
        stream << "\nFirst Half Fixed Part:  " << utauPara.first_half_fixed_part;
        stream << "\nLast Unused Part:       " << utauPara.last_unused_part;
        stream << "\nVolumes:                " << utauPara.volumes;
        stream << "\nTempo:                  " << utauPara.tempo;
        stream << "\nTempo Number:           " << utauPara.tempo_num;
        stream << "\nPitch String:           " << utauPara.pitch;
        YALL_DEBUG_ << stream.str();
    }

    UTAUPara libUTAU::GetUTAUPara() const {
        return utauPara;
    }

    void libUTAU::CheckPara(const AudioInfo &audioInfo) {
        if (audioInfo.fs <= 0) {
            throw parameter_error("The audio sample rate is invalid");
        }

        utauPara.wave_length = static_cast<double>(audioInfo.x_length) / static_cast<double>(audioInfo.fs) * 1000.0;
        if (!(utauPara.wave_length > 0.0)) {
            throw parameter_error("The input audio is empty");
        }

        if (!(utauPara.velocity > 0.0)) {
            utauPara.velocity = 1.0;
        }
        if (utauPara.offset < 0.0) {
            utauPara.offset = 0.0;
        }

        if (utauPara.last_unused_part < 0.0) {
            utauPara.last_unused_part = utauPara.wave_length - utauPara.offset + utauPara.last_unused_part;
            if (utauPara.last_unused_part < 0.0) {
                utauPara.last_unused_part = 0.0;
            }
        }

        if (utauPara.offset + utauPara.last_unused_part >= utauPara.wave_length) {
            throw parameter_error("The audio offset and whitespace exceed the audio length");
        }

        if (utauPara.offset + utauPara.last_unused_part + utauPara.first_half_fixed_part >= utauPara.wave_length) {
            utauPara.first_half_fixed_part = utauPara.wave_length - utauPara.offset - utauPara.last_unused_part;
        }
        if (utauPara.first_half_fixed_part < 0.0) {
            utauPara.first_half_fixed_part = 0.0;
        }

        utauPara.pre_cross_length = utauPara.wave_length - utauPara.offset - utauPara.first_half_fixed_part - utauPara.last_unused_part;
        utauPara.base_length = utauPara.first_half_fixed_part / utauPara.velocity;
        utauPara.cross_length = utauPara.required_length - utauPara.base_length;

        if (utauPara.pre_cross_length <= 0.0 && utauPara.cross_length > 0.0) {
            throw parameter_error("The input audio is too short for the requested stretch");
        }

        if (utauPara.cross_length <= 0.0) {
            if (utauPara.pre_cross_length > 0.0) {
                throw parameter_error("The required length is shorter than the fixed part");
            }
            utauPara.stretch_length = 1.0;
        } else {
            utauPara.stretch_length = std::clamp(utauPara.pre_cross_length / utauPara.cross_length, 0.0, 1.0);
        }

        const double output_samples = utauPara.required_length * 0.001 * static_cast<double>(audioInfo.fs) + 1.0;
        if (output_samples <= 0.0 || output_samples > static_cast<double>(std::numeric_limits<int>::max())) {
            throw parameter_error("The requested output length is invalid");
        }
        utauPara.output_samples = static_cast<int>(output_samples);
    }

}// namespace lessampler
