/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>

#include "Utils/LOG.h"
#include "Dialogs/Dialogs.h"
#include "Shine/Shine.h"
#include "AudioProcess/AudioProcess.h"
#include "AudioProcess/AutoAMP.h"
#include "AudioModel/Synthesis/Synthesis.h"
#include "FileIO/GenerateAudioModel.h"
#include "FileIO/FileWriteUnit.h"

#include "lessconfig.h"
#include "StaticCast.h"
#include "lessampler.h"

lessampler::lessampler(int argc, char **argv) : argc(argc), argv(argv) {
    // Get the executable file path
    exec_path = std::filesystem::weakly_canonical(std::filesystem::path(argv[0])).parent_path();
    YALL_DEBUG_ << "exec path: " + this->exec_path.string();
    // Setting the configure file and get config
    ConfigUnit configUnit((this->exec_path / CONFIGFILENAME).string());
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

void lessampler::run() {
    // Read configure
    if (this->configure.debug_mode) {
        YALL_DEBUG_.EnableDebug();
    }

    // Parse CommandLine Args
    if (ParseArgs()) {
        AudioModelIO audio_model_io(in_file_path);

        // Check if an audio model exists。 If it does not exist, turn on multithreaded generation
        if (!audio_model_io.CheckAudioModel()) {
            YALL_INFO_ << "Audio model not found, generating...";
            GenerateAudioModel genmodule(std::filesystem::weakly_canonical(std::filesystem::path(argv[1])).parent_path(), configure);
        }

        // Read audio model
        audio_model_io.ReadAudioModel();
        auto origin_audio_model = audio_model_io.GetAudioModel();

        // Generate Shine with audio model and parameters
        Shine shine(argc, argv, origin_audio_model, Shine::SHINE_MODE::UTAU);
        auto shine_para = shine.GetShine();

        // Audio transform
        AudioProcess aduioProcess(origin_audio_model, shine_para);
        auto trans_audio_model = aduioProcess.GetTransAudioModel();

        // Synthesize audio from a model
        Synthesis synthesis(trans_audio_model, shine_para.output_samples);
        auto out_wav_data = synthesis.GetWavData();

        // Perform automatic amplification
        AutoAMP amp(shine_para, out_wav_data);
        out_wav_data = amp.GetAMP();

        // Save to target wav file
        FileWriteUnit::WriteWav(shine_para.output_file_name, out_wav_data, shine_para.output_samples, trans_audio_model.fs);
    }
}

bool lessampler::ParseArgs() {
    // Basic Open
    if (argc < 2) {
        show_logo();
        Dialogs::notify("lessampler", "lessampler: Configure");
        // ADD Qt/TUI
        return false;
    }

    // model generation operator
    if (argc == 2) {
        show_logo();
        Dialogs::notify("lessampler", "Start modeling against the audio files");
        YALL_INFO_ << "Start modeling against the audio files in the provided destination folder...";
        GenerateAudioModel genmodule(argv[1], configure);
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


