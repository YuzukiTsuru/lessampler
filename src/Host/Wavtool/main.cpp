/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include <exception>
#include <filesystem>
#include <iostream>
#include <string_view>
#include <vector>

#include "Audio/WavIO.h"
#include "Host/Wavtool/WavAssembly.h"
#include "Host/Wavtool/WavtoolPlan.h"
#include "Host/Wavtool/WavtoolProtocol.h"

using namespace lessampler;

int main(int argc, char **argv) {
    try {
        std::vector<std::string_view> args;
        args.reserve(static_cast<std::size_t>(argc));
        for (int i = 0; i < argc; ++i) {
            args.emplace_back(argv[i] == nullptr ? "" : argv[i]);
        }

        if (args.size() >= 2 && (args[1] == "--help" || args[1] == "-h")) {
            wavtool::WavtoolProtocol::PrintUsage(std::cout);
            return 0;
        }

        if (args.size() >= 2 && args[1] == "--plan") {
            if (args.size() != 3) {
                wavtool::WavtoolProtocol::PrintUsage(std::cerr);
                return 2;
            }
            const auto plan = wavtool::WavtoolPlan::Parse(std::filesystem::path(args[2]));
            wavtool::WavtoolPlan::Run(plan);
            return 0;
        }

        if (args.size() < 5) {
            wavtool::WavtoolProtocol::PrintUsage(std::cerr);
            return 2;
        }

        const auto request = wavtool::WavtoolProtocol::Parse(args);

        int sample_rate = 44100;
        int channels = 1;
        if (std::filesystem::exists(request.input)) {
            const auto info = WavIO::ProbeWav(request.input);
            sample_rate = info.sample_rate;
            channels = info.channels;
        }

        auto output = wavtool::WavAssembly::LoadOrCreate(request.output, sample_rate, channels);
        wavtool::WavAssembly::AppendRelative(output,
                                             wavtool::WavSegment{request.input, request.skip_ms, request.length_ms, request.envelope},
                                             request.overlap_ms);
        wavtool::WavAssembly::Write(output, request.output);
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
