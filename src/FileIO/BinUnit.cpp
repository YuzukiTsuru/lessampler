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
#include <fstream>

#include "BinUnit.h"
#include "Utils/LOG.h"
#include "Utils/exception.h"

#if (defined (__WIN32__) || defined (_WIN32)) && !defined (__MINGW32__)
#include <conio.h>
#include <windows.h>
#pragma comment(lib, "winmm.lib")
#pragma warning(disable : 4996)
#endif
#if (defined (__linux__) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(__APPLE__))

#include <cstdint>
#include <ctime>

#endif

BinUnit::BinUnit(const std::string &FilePath, lessAudioModel audioModel) : FilePath(FilePath), _audioModel(audioModel) {}

BinUnit::BinUnit(const std::string &FilePath) : FilePath(FilePath) {}

BinUnit::~BinUnit() {
    delete[] this->_audioModel.time_axis;
    delete[] this->_audioModel.f0;
    for (int i = 0; i < this->_audioModel.f0_length; ++i) {
        delete[] this->_audioModel.spectrogram[i];
        delete[] this->_audioModel.aperiodicity[i];
    }
    delete[] this->_audioModel.aperiodlength;
    delete[] this->_audioModel.speclength;
    delete[] this->_audioModel.spectrogram;
    delete[] this->_audioModel.aperiodicity;
}

[[maybe_unused]] void BinUnit::SetFilePath(const std::string &Path) {
    this->FilePath = Path;
}

[[maybe_unused]] void BinUnit::SetAudioModel(lessAudioModel audioModel) {
    this->_audioModel = audioModel;
}


[[maybe_unused]] lessAudioModel BinUnit::GetAudioModel() {
    return this->_audioModel;
}

[[maybe_unused]] std::string BinUnit::GetFilePath() {
    return this->FilePath.string();
}

void BinUnit::SaveAudioModel() {
    // TODO: impl LZ4Stream to compress files
    WriteAudioContent();
}

lessAudioModel BinUnit::ReadAudioModel() {
    ReadAudioContent();
    return this->_audioModel;
}

void BinUnit::WriteAudioContent() {
    std::ofstream out_audio_model(FilePath, std::ios::out | std::ios::binary);
    if (!out_audio_model) {
        throw file_open_error(this->FilePath.string());
    }

    // FIXME: Find a proprietary way to solve this
    out_audio_model.write(reinterpret_cast<const char *>(&this->_audioModel.f0_length), std::streamsize(sizeof(this->_audioModel.f0_length)));
    out_audio_model.tellp();

    // write f0 to bin
    out_audio_model.write(reinterpret_cast<const char *>(this->_audioModel.f0), std::streamsize(this->_audioModel.f0_length * sizeof(double)));

    // write the sampling frequency
    out_audio_model.write(reinterpret_cast<const char *>(&this->_audioModel.fs), std::streamsize(sizeof(this->_audioModel.fs)));

    // write the sampling frequency
    out_audio_model.write(reinterpret_cast<const char *>(&this->_audioModel.frame_period), std::streamsize(sizeof(this->_audioModel.frame_period)));

    // write the fft size
    out_audio_model.write(reinterpret_cast<const char *>(&this->_audioModel.fft_size), std::streamsize(sizeof(this->_audioModel.fft_size)));

    // write the spectrogram data
    for (int i = 0; i < this->_audioModel.f0_length; i++) {
        out_audio_model.write(reinterpret_cast<const char *>(this->_audioModel.spectrogram[i]), std::streamsize((this->_audioModel.fft_size / 2 + 1) * sizeof(double)));
    }

    for (int i = 0; i < this->_audioModel.f0_length; ++i) {
        out_audio_model.write(reinterpret_cast<const char *>(this->_audioModel.aperiodicity[i]), std::streamsize((this->_audioModel.fft_size / 2 + 1) * sizeof(double)));
    }

    // Close file stream
    out_audio_model.close();
}

void BinUnit::ReadAudioContent() {
    std::ifstream in_audio_model(this->FilePath, std::ios::binary | std::ios::in);
    if (!in_audio_model.is_open()) {
        throw file_open_error(this->FilePath.string());
    }

    this->_audioModel.f0_length = 0;
    // Read f0 length
    in_audio_model.read(reinterpret_cast<char *>(&this->_audioModel.f0_length), std::streamsize(sizeof(this->_audioModel.f0_length)));
    this->_audioModel.f0 = new double[this->_audioModel.f0_length];

    // read the f0
    in_audio_model.read(reinterpret_cast<char *>(this->_audioModel.f0), std::streamsize(this->_audioModel.f0_length * sizeof(double)));

    // read the sampling frequency
    in_audio_model.read(reinterpret_cast<char *>(&this->_audioModel.fs), std::streamsize(sizeof(this->_audioModel.fs)));

    // read the frame period
    in_audio_model.read(reinterpret_cast<char *>(&this->_audioModel.frame_period), std::streamsize(sizeof(this->_audioModel.frame_period)));

    // read the fft size
    in_audio_model.read(reinterpret_cast<char *>(&this->_audioModel.fft_size), std::streamsize(sizeof(this->_audioModel.fft_size)));

    // alloc spectrogram memory
    this->_audioModel.spectrogram = new double *[this->_audioModel.f0_length];
    for (int i = 0; i < this->_audioModel.f0_length; ++i) {
        this->_audioModel.spectrogram[i] = new double[this->_audioModel.fft_size / 2 + 1];
    }

    for (int i = 0; i < this->_audioModel.f0_length; i++) {
        in_audio_model.read(reinterpret_cast<char *>(this->_audioModel.spectrogram[i]), std::streamsize((this->_audioModel.fft_size / 2 + 1) * sizeof(double)));
    }

    // alloc spectrogram memory
    this->_audioModel.aperiodicity = new double *[this->_audioModel.f0_length];
    for (int i = 0; i < this->_audioModel.f0_length; ++i) {
        this->_audioModel.aperiodicity[i] = new double[this->_audioModel.fft_size / 2 + 1];
    }

    for (int i = 0; i < this->_audioModel.f0_length; i++) {
        in_audio_model.read(reinterpret_cast<char *>(this->_audioModel.aperiodicity[i]), std::streamsize((this->_audioModel.fft_size / 2 + 1) * sizeof(double)));
    }

    in_audio_model.close();
}






