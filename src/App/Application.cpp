/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * Application is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with Application. If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Core/AudioInfo.h"
#include "Analysis/World/AutoAMP.h"
#include "Backend/VocoderBackend.h"
#include "Config/VoicebankConfig.h"
#include "Platform/Dialogs.h"
#include "Engine/VoicebankModeler.h"
#include "Audio/WavIO.h"
#include "Host/UTAU/Shine.h"
#include "Core/Log.h"
#include "Core/Timer.h"

#include "App/Application.h"
#include "lessconfig.h"

Application::Application(int _argc, char **_argv) : argc(_argc), argv(_argv) {
    // Get the executable file path
    exec_path = std::filesystem::weakly_canonical(std::filesystem::path(argv[0])).parent_path();
    YALL_DEBUG_ << "exec path: " + this->exec_path.string();
    // Setting the configure file and get config
    ConfigLoader configUnit(this->exec_path);
    configure = configUnit.GetConfig();
}

void Application::show_logo() {
    std::cout << lessampler_log::color::cyan << " _                           _         \n"
                                                "| |___ ___ ___ ___ _____ ___| |___ ___ \n"
                                                "| | -_|_ -|_ -| .'|     | . | | -_|  _|\n"
                                                "|_|___|___|___|__,|_|_|_|  _|_|___|_|  \n"
                                                " Version: "
              << PROJECT_GIT_HASH
              << "       |_|            "
              << "\n\nCopyright (c)  2018 - 2022, YuzukiTsuru <GloomyGhost@GloomyGhost.com>\n"
              << lessampler_log::color::reset
              << std::endl;
}

void Application::ShowUsage() {
    std::cout << "Usage:\n"
                 "  Application --help | --version\n"
                 "  Application <voicebank-directory>\n"
                 "  Application <in.wav> <out.wav> <note> [time%] [flags] [offset] [required]\n"
                 "             [fixed] [blank] [volume] [modulation] [tempo] [pitchbend]\n"
              << std::endl;
}

bool Application::ParseArgs() {
    if (argc >= 2) {
        const std::string_view first(argv[1]);
        if (first == "--help" || first == "-h") {
            show_logo();
            ShowUsage();
            return false;
        }
        if (first == "--version" || first == "-V") {
            show_logo();
            return false;
        }
    }

    if (argc < 2) {
        show_logo();
        ShowUsage();
        Dialogs::notify("Application: Configure", "Application");
        return false;
    }

    if (argc == 2) {
        show_logo();
        const std::filesystem::path target(argv[1]);
        if (!std::filesystem::is_directory(target)) {
            YALL_ERROR_ << "Modeling input is not a directory: " + target.string();
            ShowUsage();
            return false;
        }

        Dialogs::notify("Start modeling audio files", "Application");
        YALL_INFO_ << "Start modeling against the audio files in the provided destination folder...";
        VoicebankModeler genmodule(target, configure);
        Dialogs::notify("Modeling Done!", "Application");
        return false;
    }

    if (argc == 3) {
        show_logo();
        ShowUsage();
        return false;
    }

    in_file_path = std::filesystem::path(argv[1]);
    return true;
}

void Application::run() {
    // Read configure
    if (this->configure.debug_mode) {
        YALL_DEBUG_.EnableDebug();
    }

    // Enable Timer
    Timer timer;

    // Parse CommandLine Args
    if (ParseArgs()) {
        YALL_INFO_ << "Application Copyright (c)  2018 - 2022, YuzukiTsuru, Version: " + std::string(PROJECT_GIT_HASH);
        YALL_INFO_ << "Audio File: " + in_file_path.string();

        VoicebankConfig voice_bank_config(in_file_path, configure);
        configure = voice_bank_config.GetConfig();

        // In some projects, the creator will use x.wav, z.wav or other non-existing audio to force truncation of audio
        // here is a basic handle。
        if (!std::filesystem::exists(in_file_path)) {
            YALL_WARN_ << "No Audio File input. Generate Blank Audio";
            Shine shine(argc, argv, Shine::SHINE_MODE::UTAU);
            auto shine_para = shine.GetShine();
            auto blank_audio_size = static_cast<int>(shine_para.required_length * 0.001 * default_fs) + 1;
            std::vector<double> blank_audio(static_cast<std::size_t>(blank_audio_size), 0.0);
            WavIO::WriteWav(shine_para.output_file_name, blank_audio, default_fs);
        } else {
            const auto wav_info = WavIO::ProbeWav(in_file_path);
            const AudioInfo audio_info{wav_info.sample_rate, wav_info.frames,
                                       configure.audio_model_frame_period};

            Shine shine(argc, argv, audio_info, Shine::SHINE_MODE::UTAU);
            const auto shine_para = shine.GetShine();

            auto backend = lessbackend::CreateVocoderBackend(configure.vocoder);
            YALL_INFO_ << "Backend: " + std::string(backend->name());

            // WORLD uses its cached .lessaudio model. LLSM uses its layer-1
            // cache when it can, and only reads the PCM on a cache miss. Pass
            // an empty span rather than paying for the WAV decode up front.
            timer.SetTimer();
            lessbackend::RenderRequest request{in_file_path, std::span<const double>{}, wav_info.sample_rate,
                                               shine_para, configure};
            auto result = backend->Render(request);
            YALL_INFO_ << timer.GetTimer("Render: ");

            timer.SetTimer();
            AutoAMP::ApplyInPlace(result.samples, shine_para.volumes);
            YALL_INFO_ << timer.GetTimer("Auto AMP Audio: ");

            WavIO::WriteWav(shine_para.output_file_name, result.samples, result.fs);
        }
        YALL_OK_ << "All Process Done: " + timer.EndTimer();
    }
}
