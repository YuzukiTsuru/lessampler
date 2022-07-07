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
    auto audio_out_model = WriteAudioContent();
    audio_out_model.close();
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

std::ofstream AudioModelIO::WriteAudioContent() {
    std::ofstream audio_out_model(audio_model_file_path, std::ios::out | std::ios::binary);

    // Write Header
    audio_out_model.write(lessaudio_header, sizeof(char) * 6);

    // Write model basic data
    int x_length = _audioModel.x.size();
    audio_out_model.write(reinterpret_cast<const char *>(&x_length), sizeof(int));
    audio_out_model.write(reinterpret_cast<const char *>(&_audioModel.fs), sizeof(int));
    audio_out_model.write(reinterpret_cast<const char *>(&_audioModel.frame_period), sizeof(double));
    audio_out_model.write(reinterpret_cast<const char *>(&_audioModel.w_length), sizeof(int));
    audio_out_model.write(reinterpret_cast<const char *>(&_audioModel.fft_size), sizeof(int));

    // Write model data
    // Store f0 size
    std::streamsize f0_size = _audioModel.f0.size();
    audio_out_model.write(reinterpret_cast<const char *>(&f0_size), sizeof(std::streamsize));
    // Store f0 contents
    audio_out_model.write(reinterpret_cast<const char *>(&_audioModel.f0[0]), f0_size * sizeof(double));

    std::streamsize sp_size = _audioModel.spectrogram.size();
    audio_out_model.write(reinterpret_cast<const char *>(&sp_size), sizeof(std::streamsize));
    for (auto &item: _audioModel.spectrogram) {
        // Store sp size
        std::streamsize size = item.size();
        audio_out_model.write(reinterpret_cast<const char *>(&size), sizeof(std::streamsize));

        // Store sp contents
        audio_out_model.write(reinterpret_cast<const char *>(&item[0]), item.size() * sizeof(double));
    }

    std::streamsize ap_size = _audioModel.aperiodicity.size();
    audio_out_model.write(reinterpret_cast<const char *>(&ap_size), sizeof(std::streamsize));
    for (auto &item: _audioModel.aperiodicity) {
        // Store ap size
        auto size = item.size();
        audio_out_model.write(reinterpret_cast<const char *>(&size), sizeof(std::streamsize));

        // Store ap contents
        audio_out_model.write(reinterpret_cast<const char *>(&item[0]), item.size() * sizeof(double));
    }

    return audio_out_model;
}

void AudioModelIO::ReadAudioContent() {
    std::ifstream audio_in_model(audio_model_file_path, std::ios::in | std::ios::binary);

    // Check Header
    auto header = new char[6];
    audio_in_model.read(header, sizeof(char) * 6);
    if (std::string(lessaudio_header) != std::string(header)) {
        throw header_check_error(header, lessaudio_header);
    }

    // Read basic audio
    int x_length;
    audio_in_model.read(reinterpret_cast<char *>(&x_length), sizeof(int));
    audio_in_model.read(reinterpret_cast<char *>(&_audioModel.fs), sizeof(int));
    audio_in_model.read(reinterpret_cast<char *>(&_audioModel.frame_period), sizeof(double));
    audio_in_model.read(reinterpret_cast<char *>(&_audioModel.w_length), sizeof(int));
    audio_in_model.read(reinterpret_cast<char *>(&_audioModel.fft_size), sizeof(int));

    _audioModel.x.resize(x_length);

    // Read f0 data
    std::streamsize f0_length_size = 0;
    audio_in_model.read(reinterpret_cast<char *>(&f0_length_size), sizeof(std::streamsize));
    _audioModel.f0.resize(f0_length_size);
    audio_in_model.read(reinterpret_cast<char *>(&_audioModel.f0[0]), std::streamsize(f0_length_size * sizeof(double)));

    std::streamsize sp_length_size = 0;
    audio_in_model.read(reinterpret_cast<char *>(&sp_length_size), sizeof(std::streamsize));
    for (std::streamsize n = 0; n < sp_length_size; ++n) {
        std::streamsize sp_inner_length_size = 0;
        audio_in_model.read(reinterpret_cast<char *>(&sp_inner_length_size), sizeof(std::streamsize));
        _audioModel.spectrogram.resize(sp_length_size, std::vector<double>(sp_inner_length_size));
        audio_in_model.read(reinterpret_cast<char *>(&_audioModel.spectrogram[n][0]), std::streamsize(sp_inner_length_size * sizeof(double)));
    }

    std::streamsize ap_length_size = 0;
    audio_in_model.read(reinterpret_cast<char *>(&ap_length_size), sizeof(std::streamsize));
    for (std::streamsize n = 0; n < ap_length_size; ++n) {
        std::streamsize ap_inner_length_size = 0;
        audio_in_model.read(reinterpret_cast<char *>(&ap_inner_length_size), sizeof(std::streamsize));
        _audioModel.aperiodicity.resize(ap_length_size, std::vector<double>(ap_inner_length_size));
        audio_in_model.read(reinterpret_cast<char *>(&_audioModel.aperiodicity[n][0]), std::streamsize(ap_inner_length_size * sizeof(double)));
    }
}

void AudioModelIO::GenerateFilePath() {
    audio_model_file_path = root_file_path.replace_extension(audio_model_file_ext);
}
