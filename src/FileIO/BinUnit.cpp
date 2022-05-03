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
#include "LOG.h"
#include "exception.h"

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

void BinUnit::F0BIN(const std::string &Path, AudioModel audioModel) {
    std::ofstream out_f0(Path, std::ios::out | std::ios::binary);
    if (!out_f0) {
        std::cerr << "Cannot open file: " << Path << "while saving." << std::endl;
        return;
    }
    // write f0 to bin
    out_f0.write(reinterpret_cast<const char *>(audioModel.f0), std::streamsize(audioModel.f0_length * sizeof(double)));
    // close stream
    out_f0.close();
}

void BinUnit::SPBIN(const std::string &Path, AudioModel audioModel) {
    std::ofstream out_spectrogram(Path, std::ios::out | std::ios::binary);
    if (!out_spectrogram) {
        std::cerr << "Cannot open file: " << Path << "while saving." << std::endl;
        return;
    }
    // write the sampling frequency
    out_spectrogram.write(reinterpret_cast<const char *>(&audioModel.fs), std::streamsize(sizeof(audioModel.fs)));

    // write the sampling frequency
    out_spectrogram.write(reinterpret_cast<const char *>(&audioModel.frame_period), std::streamsize(sizeof(audioModel.frame_period)));

    // write the spectrogram data
    for (int i = 0; i < audioModel.f0_length; i++) {
        out_spectrogram.write(reinterpret_cast<const char *>(audioModel.spectrogram[i]), std::streamsize((audioModel.fft_size / 2 + 1) * sizeof(double)));
    }

    // close stream
    out_spectrogram.close();
}

void BinUnit::APBIN(const std::string &Path, AudioModel audioModel) {
    std::ofstream out_aperiodicity(Path, std::ios::out | std::ios::binary);
    if (!out_aperiodicity) {
        std::cerr << "Cannot open file: " << Path << "while saving." << std::endl;
        return;
    }

    for (int i = 0; i < audioModel.f0_length; ++i) {
        out_aperiodicity.write(reinterpret_cast<const char *>(audioModel.aperiodicity[i]), std::streamsize((audioModel.fft_size / 2 + 1) * sizeof(double)));
    }

    out_aperiodicity.close();
}

AudioModel BinUnit::BINF0(const std::string &Path) {
    AudioModel audioModel{};
    std::ifstream is_f0(Path, std::ios::binary | std::ios::in);
    if (!is_f0.is_open()) {
        throw file_open_error(Path);
    }
    // read the f0 length and allocate memory
    audioModel.f0_length = static_cast<int>((is_f0.tellg() / sizeof(double)));
    audioModel.f0 = new double[audioModel.f0_length];
    // read the f0 data
    is_f0.read(reinterpret_cast<char *>(audioModel.f0), std::streamsize(audioModel.f0_length * sizeof(double)));
#ifdef DEBUG_MODE
    for (int i = 0; i < audioModel.f0_length; i++)
        LOG::DEBUG(std::to_string(audioModel.f0[i]) + " ");
#endif
    is_f0.close();
    return audioModel;
}