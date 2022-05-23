// Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

//
// Created by gloom on 2022/5/23.
//

#include <utility>
#include "GenerateAudioModel.h"

#include <Utils/LOG.h>
#include <AudioModel/lessAudioModel.h>
#include <FileIO/FileReadUnit.h>

GenerateAudioModel::GenerateAudioModel(std::filesystem::path path) : target_voice_path(std::move(path)) {
    YALL_INFO_ << "Get the audio files in the directory";
    GetWavFileLists();
#ifdef DEBUG_MODE
    PrintWavFiles();
#endif
}

void GenerateAudioModel::PrintWavFiles() {
    for (const auto &file: wav_files) {
        YALL_DEBUG_ << file.string() + "\n";
    }
}

void GenerateAudioModel::GetWavFileLists() {
    YALL_INFO_ << "Working on folder: " + target_voice_path.string();
    for (const auto &entry: std::filesystem::directory_iterator(target_voice_path)) {
        if (entry.path().extension() == ".wav") {
            wav_files.push_back(entry.path());
        }
    }
}

void GenerateAudioModel::ReadWavFile(std::filesystem::path wav_path) {
    auto x_length = FileReadUnit::GetAudioLength(wav_path.string().c_str());
    auto x = new double[x_length];
    auto fs = FileReadUnit::WavRead(wav_path.string().c_str(), x);
}
