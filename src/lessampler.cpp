/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>

#include "Utils/LOG.h"
#include "Utils/Timer.h"
#include "Dialogs/Dialogs.h"
#include "Shine/Shine.h"
#include "AudioProcess/AudioProcess.h"
#include "AudioProcess/AutoAMP.h"
#include "AudioModel/Synthesis/Synthesis.h"
#include "FileIO/GenerateAudioModel.h"
#include "FileIO/WavIO.h"

#include "lessconfig.h"
#include "StaticCast.h"
#include "lessampler.h"

lessampler::lessampler(int argc, char **argv) : argc(argc), argv(argv) {
    // Get the executable file path
    exec_path = std::filesystem::weakly_canonical(std::filesystem::path(argv[0])).parent_path();
    YALL_DEBUG_ << "exec path: " + this->exec_path.string();
    // Setting the configure file and get config
    ConfigUnit configUnit(this->exec_path / CONFIGFILENAME);
    configure = configUnit.get_config();
}

void lessampler::show_logo() {
    std::cout << cc::cyan <<
              " _                           _         \n"
              "| |___ ___ ___ ___ _____ ___| |___ ___ \n"
              "| | -_|_ -|_ -| .'|     | . | | -_|  _|\n"
              "|_|___|___|___|__,|_|_|_|  _|_|___|_|  \n"
              " Version: "
              << PROJECT_GIT_HASH
              << "       |_|            "
              << "\n\nCopyright (c)  2018 - 2022, YuzukiTsuru <GloomyGhost@GloomyGhost.com>\n"
              << cc::reset
              << std::endl;
}

bool lessampler::ParseArgs() {
    // Basic Open
    if (argc < 2) {
        show_logo();
        Dialogs::notify("lessampler: Configure", "lessampler");
        // ADD Qt/TUI
        return false;
    }

    // model generation operator
    if (argc == 2) {
        show_logo();
        Dialogs::notify("Start modeling audio files", "lessampler");
        YALL_INFO_ << "Start modeling against the audio files in the provided destination folder...";
        GenerateAudioModel genmodule(std::filesystem::path(argv[1]), configure);
        Dialogs::notify("Modeling Done!", "lessampler");
        return false;
    }

    // lessampler resampler
    if (argc > 3) {
        in_file_path = std::filesystem::path(argv[1]);
        return true;
    }

    // default
    return false;
}

void lessampler::run() {
    // Read configure
    if (this->configure.debug_mode) {
        YALL_DEBUG_.EnableDebug();
    }

    // Enable Timer
    Timer timer;

    // Parse CommandLine Args
    if (ParseArgs()) {
        YALL_INFO_ << "lessampler Copyright (c)  2018 - 2022, YuzukiTsuru, Version: " + std::string(PROJECT_GIT_HASH);
        AudioModelIO audio_model_io(in_file_path);
        YALL_INFO_ << "Audio File: " + in_file_path.string();

        // In some projects, the creator will use x.wav, z.wav or other non-existing audio to force truncation of audio
        // here is a basic handle。
        if (!std::filesystem::exists(in_file_path)) {
            YALL_WARN_ << "No Audio File input. Generate Blank Audio";
            Shine shine(argc, argv, Shine::SHINE_MODE::UTAU);
            auto shine_para = shine.GetShine();
            auto blank_audio_size = static_cast<int>(shine_para.required_length * 0.001 * default_fs) + 1;
            auto x = new double[blank_audio_size];

            for (int i = 0; i < blank_audio_size; ++i) {
                x[i] = 0.0;
            }
            WavIO::WriteWav(shine_para.output_file_name, x, blank_audio_size, default_fs);
        } else {
            // Check if an audio model exists。 If it does not exist, generate new model
            if (!audio_model_io.CheckAudioModel(configure)) {
                YALL_INFO_ << "Generating Audio Model: " + in_file_path.string();
                timer.SetTimer();
                GenerateAudioModel genmodule(argv[1], configure);
                YALL_INFO_ << timer.GetTimer("Generate Audio Model: ");
            } else {
                YALL_INFO_ << "Found Audio Model, Processing...";
            }

            // Read audio model
            timer.SetTimer();
            audio_model_io.ReadAudioModel(configure);
            auto origin_audio_model = audio_model_io.GetAudioModel();
            YALL_INFO_ << timer.GetTimer("Read Audio Model: ");

            ShowAudioInfo(origin_audio_model);

            // Generate Shine with audio model and parameters
            Shine shine(argc, argv, origin_audio_model, Shine::SHINE_MODE::UTAU);
            auto shine_para = shine.GetShine();

            // Audio transform
            timer.SetTimer();
            AudioProcess audioProcess(origin_audio_model, shine_para);
            auto trans_audio_model = audioProcess.GetTransAudioModel();
            YALL_INFO_ << timer.GetTimer("Processing Model: ");

            // Synthesize audio from a model
            timer.SetTimer();
            Synthesis synthesis(trans_audio_model, shine_para.output_samples);
            auto out_wav_data = synthesis.GetWavData();
            YALL_INFO_ << timer.GetTimer("Synthesis Audio: ");

            // Perform automatic amplification
            timer.SetTimer();
            AutoAMP amp(shine_para, out_wav_data);
            out_wav_data = amp.GetAMP();
            YALL_INFO_ << timer.GetTimer("Auto AMP Audio: ");

            // Save to target wav file
            WavIO::WriteWav(shine_para.output_file_name, out_wav_data, shine_para.output_samples, trans_audio_model.fs);
        }
        YALL_OK_ << "All Process Done: " + timer.EndTimer();
    }
}

void lessampler::ShowAudioInfo(const lessAudioModel &audioModel) {
    YALL_INFO_ << "Audio Sample Rate: " + std::to_string(audioModel.fs);
    YALL_INFO_ << "Model Sample Rate: " + std::to_string(audioModel.frame_period);
    YALL_INFO_ << "Model FFT Size: " + std::to_string(audioModel.fft_size);
}


