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
#include <fstream>
#include <utility>

#include "AudioModelIO.h"
#include "Utils/LOG.h"
#include "Utils/exception.h"

AudioModelIO::AudioModelIO(std::filesystem::path Path, lessAudioModel audioModel) :
        in_file_path(std::move(Path)), _audioModel(std::move(audioModel)) {
    root_file_path = in_file_path;
    GenerateFilePath();
}

AudioModelIO::AudioModelIO(std::filesystem::path Path) : in_file_path(std::move(Path)) {
    root_file_path = in_file_path;
    GenerateFilePath();
}

AudioModelIO::~AudioModelIO() = default;

[[maybe_unused]] void AudioModelIO::SetFilePath(const std::filesystem::path &Path) {
    root_file_path = Path;
    GenerateFilePath();
}

[[maybe_unused]] void AudioModelIO::SetAudioModel(lessAudioModel audioModel) {
    _audioModel = std::move(audioModel);
}

[[maybe_unused]] lessAudioModel AudioModelIO::GetAudioModel() {
    return _audioModel;
}

[[maybe_unused]] std::filesystem::path AudioModelIO::GetFilePath() {
    return root_file_path;
}

void AudioModelIO::SaveAudioModel() {
    // TODO: impl LZ4Stream to compress files
    WriteAudioContent();
}

lessAudioModel AudioModelIO::ReadAudioModel() {
    ReadAudioContent();
    return _audioModel;
}

bool AudioModelIO::CheckAudioModelFile(const std::filesystem::path &path) {
    return std::filesystem::exists(path);
}

bool AudioModelIO::CheckAudioModel() {
    YALL_DEBUG_ << "Check AudioModel File: " + in_file_path.string();

    if (!CheckAudioModelFile(audio_model_file_path)) {
        YALL_DEBUG_ << "Audio Model NOT Exist.";
        return false;
    } else {
        YALL_DEBUG_ << "Audio Model Exist.";
        return true;
    }
}

void AudioModelIO::WriteAudioContent() {
    std::ofstream audio_out_model(audio_model_file_path, std::ios::out | std::ios::binary);

    // Write model basic data
    audio_out_model.write(reinterpret_cast<const char *>(&_audioModel.fs), sizeof(_audioModel.fs));
    audio_out_model.write(reinterpret_cast<const char *>(&_audioModel.frame_period), sizeof(_audioModel.frame_period));
    audio_out_model.write(reinterpret_cast<const char *>(&_audioModel.f0_length), sizeof(_audioModel.f0_length));
    audio_out_model.write(reinterpret_cast<const char *>(&_audioModel.w_length), sizeof(_audioModel.w_length));

    // Write model data
    // Store f0 size
    auto f0_size = _audioModel.f0.size();
    audio_out_model.write(reinterpret_cast<const char *>(&f0_size), sizeof(f0_size));
    // Store f0 contents
    audio_out_model.write(reinterpret_cast<const char *>(&_audioModel.f0[0]), std::streamsize(f0_size * sizeof(double)));

    auto sp_size = _audioModel.spectrogram.size();
    audio_out_model.write(reinterpret_cast<const char *>(&sp_size), sizeof(sp_size));
    for (auto &item: _audioModel.spectrogram) {
        // Store sp size
        auto size = item.size();
        audio_out_model.write(reinterpret_cast<const char *>(&size), sizeof(size));

        // Store sp contents
        audio_out_model.write(reinterpret_cast<const char *>(&_audioModel.spectrogram[0]), std::streamsize(item.size() * sizeof(double)));
    }

    auto ap_size = _audioModel.aperiodicity.size();
    audio_out_model.write(reinterpret_cast<const char *>(&ap_size), sizeof(ap_size));
    for (auto &item: _audioModel.aperiodicity) {
        // Store ap size
        auto size = item.size();
        audio_out_model.write(reinterpret_cast<const char *>(&size), sizeof(size));

        // Store ap contents
        audio_out_model.write(reinterpret_cast<const char *>(&_audioModel.aperiodicity[0]), std::streamsize(item.size() * sizeof(double)));
    }

    // close audio out model file
    audio_out_model.close();
}

void AudioModelIO::ReadAudioContent() {

}

void AudioModelIO::GenerateFilePath() {
    audio_model_file_path = root_file_path.replace_extension(audio_model_file_ext);
}