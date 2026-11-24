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

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "Audio/SampleSource.h"
#include "Audio/WavIO.h"
#include "Config/ConfigLoader.h"
#include "Core/AudioInfo.h"
#include "Core/Log.h"
#include "Core/Timer.h"
#include "Engine/RenderEngine.h"
#include "Engine/VoicebankModeler.h"
#include "Host/UTAU/Shine.h"
#include "Platform/Dialogs.h"

#include "lessconfig.h"

namespace lessampler {

    namespace {
        constexpr int kBlankSampleRate = 44100;
    }

    Application::Application(int argc, char **argv) : argc_(argc), argv_(argv) {
        exec_path_ = std::filesystem::weakly_canonical(std::filesystem::path(argv_[0])).parent_path();
        YALL_DEBUG_ << "exec path: " + exec_path_.string();

        ConfigLoader loader(exec_path_);
        configure_ = loader.GetConfig();
    }

    void Application::show_logo() {
        std::cout << lessampler::log::color::cyan << " _                           _         \n"
                                                     "| |___ ___ ___ ___ _____ ___| |___ ___ \n"
                                                     "| | -_|_ -|_ -| .'|     | . | | -_|  _|\n"
                                                     "|_|___|___|___|__,|_|_|_|  _|_|___|_|  \n"
                                                     " Version: "
                  << PROJECT_GIT_HASH
                  << "       |_|            "
                  << "\n\nCopyright (c)  2018 - 2022, YuzukiTsuru <GloomyGhost@GloomyGhost.com>\n"
                  << lessampler::log::color::reset
                  << std::endl;
    }

    void Application::show_usage() {
        std::cout << "Usage:\n"
                     "  lessampler --help | --version\n"
                     "  lessampler <voicebank-directory>\n"
                     "  lessampler <in.wav> <out.wav> <note> [time%] [flags] [offset] [required]\n"
                     "             [fixed] [blank] [volume] [modulation] [tempo] [pitchbend]\n"
                  << std::endl;
    }

    Application::Command Application::parse_args() const {
        if (argc_ < 2) {
            show_logo();
            show_usage();
            Dialogs::notify("lessampler: Configure", "lessampler");
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

        if (argc_ == 2) {
            show_logo();
            if (!std::filesystem::is_directory(first)) {
                YALL_ERROR_ << "Modeling input is not a directory: " + std::string(first);
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
            YALL_OK_ << "All Process Done: " + timer.EndTimer();
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
        YALL_INFO_ << timer.GetTimer("Render: ");

        timer.SetTimer();
        WavIO::WriteWav(params.output_file_name, result.samples, result.fs);
        YALL_EVAL_ << timer.GetTimer("Write WAV: ");

        YALL_OK_ << "All Process Done: " + timer.EndTimer();
    }

    void Application::render_blank(int argc, char **argv) {
        YALL_WARN_ << "No Audio File input. Generate Blank Audio";
        Shine shine(argc, argv, Shine::SHINE_MODE::UTAU);
        const auto params = shine.GetShine();
        const auto sample_count = static_cast<std::size_t>(params.required_length * 0.001 * kBlankSampleRate) + 1;
        const std::vector<double> blank(sample_count, 0.0);
        WavIO::WriteWav(params.output_file_name, blank, kBlankSampleRate);
    }

    void Application::model_voicebank() {
        Dialogs::notify("Start modeling audio files", "lessampler");
        YALL_INFO_ << "Start modeling against the audio files in the provided destination folder...";
        VoicebankModeler modeler(std::filesystem::path(argv_[1]), configure_);
        Dialogs::notify("Modeling Done!", "lessampler");
    }

    void Application::run() {
        if (configure_.debug_mode) {
            YALL_DEBUG_.EnableDebug();
        }

        switch (parse_args()) {
            case Command::help:
            case Command::version:
            case Command::none:
            case Command::usage:
                return;
            case Command::model_voicebank:
                model_voicebank();
                return;
            case Command::render_note:
                YALL_INFO_ << "lessampler Copyright (c)  2018 - 2022, YuzukiTsuru, Version: " + std::string(PROJECT_GIT_HASH);
                YALL_INFO_ << "Audio File: " + std::string(argv_[1]);
                render_note();
                return;
        }
    }

}// namespace lessampler
