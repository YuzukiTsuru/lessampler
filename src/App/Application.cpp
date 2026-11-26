/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "App/Application.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <format>
#include <fstream>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Audio/Dsp/MasterChain.h"
#include "Audio/Preprocess.h"
#include "Audio/SampleSource.h"
#include "Audio/WavIO.h"
#include "Config/ConfigLoader.h"
#include "Core/AudioInfo.h"
#include "Core/Exception.h"
#include "Core/Log.h"
#include "Core/Timer.h"
#include "Engine/RenderEngine.h"
#include "Engine/VoicebankModeler.h"
#include "Host/UTAU/Shine.h"
#include "Host/Wavtool/WavAssembly.h"
#include "Host/Wavtool/WavtoolPlan.h"
#include "Host/Wavtool/WavtoolProtocol.h"
#include "Vocoder/Common/F0Estimator.h"
#include "Vocoder/VocoderBackend.h"

#include "lessconfig.h"

namespace lessampler {

    namespace {
        constexpr int kBlankSampleRate = 44100;

        std::string compiler_label() {
#if defined(__clang__)
            return std::format("Clang {}", __clang_version__);
#elif defined(__GNUC__)
            return std::format("GCC {}.{}.{}", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#elif defined(_MSC_VER)
            return std::format("MSVC {}", _MSC_VER);
#else
            return "unknown compiler";
#endif
        }

        std::string architecture_label() {
#if defined(__x86_64__) || defined(_M_X64)
            return "x86_64";
#elif defined(__aarch64__) || defined(_M_ARM64)
            return "aarch64";
#elif defined(__i386__) || defined(_M_IX86)
            return "x86";
#elif defined(__arm__) || defined(_M_ARM)
            return "arm";
#else
            return "unknown";
#endif
        }

        std::string platform_label() {
#if defined(_WIN32)
            return "windows";
#elif defined(__APPLE__)
            return "macos";
#elif defined(__linux__)
            return "linux";
#else
            return "unknown";
#endif
        }

    }// namespace

    Application::Application(int argc, char **argv) : argc_(argc), argv_(argv) {
        exec_path_ = std::filesystem::weakly_canonical(std::filesystem::path(argv_[0])).parent_path();
        LOG_DEBUG << "exec path: " + exec_path_.string();
    }

    void Application::load_config() {
        ConfigLoader loader(exec_path_);
        configure_ = loader.GetConfig();
    }

    void Application::show_logo() {
        using namespace lessampler::log;
        enable_ansi();

        static constexpr std::array<std::string_view, 4> logo{
                " _                           _         ",
                "| |___ ___ ___ ___ _____ ___| |___ ___ ",
                "| | -_|_ -|_ -| .'|     | . | | -_|  _|",
                "|_|___|___|___|__,|_|_|_|  _|_|___|_|  ",
        };
        constexpr std::size_t rule_width = 39;
        const std::string rule(rule_width, '*');

        std::cout << '\n';
        for (std::string_view line: logo) {
            std::cout << color::cyan << line << color::reset << '\n';
        }
        std::cout << color::bright_cyan << rule << color::reset << '\n'
                  << color::bold << std::format(" lessampler v{} Commit: {}", PROJECT_VER, PROJECT_GIT_HASH)
                  << color::reset << '\n'
                  << color::dim << " github.com/YuzukiTsuru/lessampler" << color::reset << '\n'
                  << color::bright_cyan << rule << color::reset << '\n'
                  << color::dim << std::format(" Built by: {} ({})", compiler_label(), std::format("{} {}", platform_label(), architecture_label()))
                  << color::reset << '\n'
                  << color::dim << std::format(" Backends: {}", backend::AvailableVocoderBackends())
                  << color::reset << '\n'
                  << color::dim << std::format(" F0 estimators: {}", AvailableF0Estimators())
                  << color::reset << '\n'
                  << color::dim << " Copyright (c) 2018 - 2026 YuzukiTsuru" << color::reset << '\n'
                  << std::endl;
    }

    void Application::show_usage() {
        std::cout << "Usage:\n"
                     "  lessampler --help | --version\n"
                     "  lessampler <voicebank-directory>\n"
                     "  lessampler --polish <in.wav> <out.wav> [none|natural|warm|power]\n"
                     "  lessampler --feature-plan <plan.toml>\n"
                     "  lessampler --f0 <in.wav> <out.csv>\n"
                     "  lessampler <in.wav> <out.wav> <note> [time%] [flags] [offset] [required]\n"
                     "             [fixed] [blank] [volume] [modulation] [tempo] [pitchbend]\n"
                     "  lessampler --plan <plan.toml>\n"
                     "  lessampler <out.wav> <in.wav> <skip_ms> <length_ms> <p1> <p2> <p3> <v1>\n"
                     "             <v2> <v3> <v4> <overlap> <p4> <p5> <v5>\n"
                     "\n"
                     "The last form is the classic UTAU wavtool protocol; it is selected by\n"
                     "passing exactly 15 positional arguments.\n"
                  << std::endl;
    }

    Application::Command Application::parse_args() const {
        if (argc_ < 2) {
            show_logo();
            show_usage();
            return Command::none;
        }

        const std::string_view first(argv_[1]);
        if (first == "--help" || first == "-h") {
            show_logo();
            show_usage();
            return Command::help;
        }
        if (first == "--version" || first == "-V") {
            show_logo();
            return Command::version;
        }
        if (first == "--plan") {
            if (argc_ != 3) {
                wavtool::WavtoolProtocol::PrintUsage(std::cerr);
                return Command::wavtool_usage;
            }
            return Command::wavtool_plan;
        }
        // The classic UTAU wavtool protocol always passes 15 positional arguments.
        // A UTAU resampler call passes at most 13, so the count is unambiguous.
        if (argc_ == 16 && !first.starts_with("--")) {
            return Command::wavtool;
        }
        if (first == "--polish") {
            if (argc_ != 4 && argc_ != 5) {
                show_usage();
                return Command::usage;
            }
            if (!backend::NeuralPolishAvailable()) {
                throw parameter_error("--polish is unavailable: the neural backend is not enabled in this build");
            }
            return Command::polish;
        }
        if (first == "--f0") {
            if (argc_ != 4) {
                show_usage();
                return Command::usage;
            }
            return Command::f0_dump;
        }
        if (first == "--feature-plan") {
            if (argc_ != 3) {
                show_usage();
                return Command::usage;
            }
            if (!backend::NeuralFeatureAssemblyAvailable()) {
                throw parameter_error("--feature-plan is unavailable: the neural backend is not enabled in this build");
            }
            return Command::feature_plan;
        }

        if (argc_ == 2) {
            show_logo();
            if (!std::filesystem::is_directory(first)) {
                LOG_ERROR << "Modeling input is not a directory: " + std::string(first);
                show_usage();
                return Command::usage;
            }
            return Command::model_voicebank;
        }

        if (argc_ == 3) {
            show_logo();
            show_usage();
            return Command::usage;
        }

        return Command::render_note;
    }

    void Application::render_note() {
        Timer timer;

        if (!std::filesystem::exists(argv_[1])) {
            // UTAU uses missing files such as x.wav for rests.
            render_blank(argc_, argv_);
            LOG_OK << "All Process Done: " + timer.EndTimer();
            return;
        }

        input_path_ = std::filesystem::path(argv_[1]);
        const SampleSource source(input_path_);

        RenderEngine engine(configure_);
        const auto effective = engine.configs().Resolve(input_path_);
        Shine shine(argc_, argv_, source.audio_info(effective.audio_model_frame_period), Shine::SHINE_MODE::UTAU);
        const auto params = shine.GetShine();

        timer.SetTimer();
        auto result = engine.Render(source, params);
        LOG_INFO << timer.GetTimer("Render: ");

        timer.SetTimer();
        WavIO::WriteWav(params.output_file_name, result.samples, result.fs);
        LOG_EVAL << timer.GetTimer("Write WAV: ");

        LOG_OK << "All Process Done: " + timer.EndTimer();
    }

    void Application::render_blank(int argc, char **argv) {
        LOG_WARN << "No Audio File input. Generate Blank Audio";
        Shine shine(argc, argv, Shine::SHINE_MODE::UTAU);
        const auto params = shine.GetShine();
        const auto sample_count = static_cast<std::size_t>(params.required_length * 0.001 * kBlankSampleRate) + 1;
        const std::vector<double> blank(sample_count, 0.0);
        WavIO::WriteWav(params.output_file_name, blank, kBlankSampleRate);
    }

    int Application::run_wavtool() {
        try {
            std::vector<std::string_view> args;
            args.reserve(static_cast<std::size_t>(argc_));
            for (int i = 0; i < argc_; ++i) {
                args.emplace_back(argv_[i] == nullptr ? "" : argv_[i]);
            }

            const auto request = wavtool::WavtoolProtocol::Parse(args);

            int sample_rate = kBlankSampleRate;
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

    int Application::run_wavtool_plan() {
        try {
            const auto plan = wavtool::WavtoolPlan::Parse(std::filesystem::path(argv_[2]));
            wavtool::WavtoolPlan::Run(plan);
            return 0;
        } catch (const std::exception &error) {
            std::cerr << error.what() << '\n';
            return 1;
        }
    }

    void Application::model_voicebank() {
        LOG_INFO << "Start modeling against the audio files in the provided destination folder...";
        VoicebankModeler modeler(std::filesystem::path(argv_[1]), configure_);
        LOG_OK << "Modeling done.";
    }

    void Application::polish_audio() {
        Timer timer;
        const WavData input = WavIO::ReadWav(argv_[2]);
        timer.SetTimer();
        auto result = backend::PolishWaveform(input.samples, input.sample_rate, configure_);
        LOG_INFO << timer.GetTimer("Neural polish: ");
        const std::string master = argc_ >= 5 ? std::string(argv_[4]) : configure_.master_preset;
        MasterChain::Process(result.samples, result.fs, 1, CreateMasterOptions(master));
        WavIO::WriteWav(argv_[3], result.samples, result.fs);
        LOG_OK << "Polished " + std::string(argv_[2]) + " -> " + std::string(argv_[3]);
    }

    void Application::f0_dump() {
        const WavData input = WavIO::ReadWav(argv_[2]);
        const auto resolved = ResolveF0EstimatorConfig(configure_, configure_.vocoder);
        std::vector<double> processed;
        std::span<const double> samples = input.samples;
        if (resolved.preprocess.enabled) {
            processed = PreprocessAudio(input.samples, input.sample_rate, resolved.preprocess);
            samples = std::span<const double>(processed);
        }
        auto estimator = CreateF0Estimator(resolved.f0_estimator, resolved);
        const auto track = estimator->Estimate(samples, input.sample_rate, resolved);
        const std::filesystem::path output(argv_[3]);
        if (!output.parent_path().empty()) {
            std::filesystem::create_directories(output.parent_path());
        }
        std::ofstream stream(output);
        if (!stream) {
            throw file_open_error(output.string());
        }
        stream << "time_ms,f0_hz\n";
        const auto count = std::min(track.time_axis.size(), track.f0.size());
        for (std::size_t i = 0; i < count; ++i) {
            stream << track.time_axis[i] * 1000.0 << ',' << track.f0[i] << '\n';
        }
        if (!stream) {
            throw file_open_error(output.string());
        }
        LOG_OK << "F0 written to " + output.string();
    }

    void Application::feature_plan() {
        const auto plan = wavtool::WavtoolPlan::Parse(argv_[2]);
        std::vector<FeatureSegment> segments;
        segments.reserve(plan.notes.size());
        for (const auto &note: plan.notes) {
            segments.push_back(FeatureSegment{note.input, note.position_ms, note.skip_ms,
                                              note.length_ms, note.envelope});
        }
        if (!plan.output.parent_path().empty()) {
            std::filesystem::create_directories(plan.output.parent_path());
        }
        Timer timer;
        const int sample_rate = plan.sample_rate > 0 ? plan.sample_rate : 44100;
        auto result = backend::AssembleFeatureTimeline(segments, sample_rate, plan.peak_normalize, configure_);
        LOG_INFO << timer.GetTimer("Feature assembly: ");
        MasterChain::Process(result.samples, result.sample_rate, 1, CreateMasterOptions(plan.master));
        WavIO::WriteWav(plan.output, result.samples, result.sample_rate);
        LOG_OK << "Feature assembly written to " + plan.output.string();
    }

    int Application::run() {
        const Command command = parse_args();

        // Wavtool calls are independent of the analysis configuration; handle
        // them before loading lessampler.less so a broken config cannot stop a
        // UTAU host from concatenating.
        switch (command) {
            case Command::wavtool:
                return run_wavtool();
            case Command::wavtool_plan:
                return run_wavtool_plan();
            case Command::wavtool_usage:
                return 2;
            case Command::help:
            case Command::version:
            case Command::none:
            case Command::usage:
                return 0;
            default:
                break;
        }

        load_config();
        if (configure_.debug_mode) {
            lessampler::log::SetDebug(true);
        }

        switch (command) {
            case Command::model_voicebank:
                model_voicebank();
                break;
            case Command::polish:
                polish_audio();
                break;
            case Command::feature_plan:
                feature_plan();
                break;
            case Command::f0_dump:
                f0_dump();
                break;
            case Command::render_note:
                LOG_INFO << "lessampler Copyright (c) 2018 - 2026, YuzukiTsuru, Version: " + std::string(PROJECT_GIT_HASH);
                LOG_INFO << "Audio File: " + std::string(argv_[1]);
                render_note();
                break;
            default:
                break;
        }
        return 0;
    }

}// namespace lessampler
