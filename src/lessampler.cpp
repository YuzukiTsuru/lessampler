/*
 * Copyright (c)  2022, YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>

#include "Utils/LOG.h"
#include "Dialogs/Dialogs.h"
#include "lessconfig.h"

#include "lessampler.h"

lessampler::lessampler(int argc, char **argv) : argc(argc), argv(argv) {
    // Get the executable file path
    this->exec_path = std::filesystem::weakly_canonical(std::filesystem::path(argv[0])).parent_path();
    YALL_DEBUG_ << "exec path: " + this->exec_path.string();
    // Setting the configure file and get config
    ConfigUnit configUnit((this->exec_path / "less.cfg").string());
    this->configure = configUnit.get_config();
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
              << "\nCopyright (c)  2018 - 2022, YuzukiTsuru <GloomyGhost@GloomyGhost.com>"
              << cc::reset
              << std::endl;
}

void lessampler::run() const {
    // Read configure
    if (this->configure.debug_mode) {
        YALL_DEBUG_.EnableDebug();
    }

    // model generation
    if (argc == 1) {
        show_logo();
        YALL_INFO_ << "Start modeling against the audio files in the provided destination folder...";


    }

    if (argc < 2) {
        show_logo();
        Dialogs::notify("lessampler", "lessampler: no input file");
        return;
    }

}

void lessampler::read_audio_file() {

}

void lessampler::GenerateAudioModel() {
    // AudioModel audioModel();
}

void lessampler::GenerateAudioModelDictory() {
    auto target_voice_folder = std::filesystem::path(argv[1]);
    YALL_INFO_ << "Working on folder: " + target_voice_folder.string();
    for (const auto &entry: std::filesystem::directory_iterator(target_voice_folder)) {
        if (entry.path().extension() == ".wav"){

        }
    }
}

