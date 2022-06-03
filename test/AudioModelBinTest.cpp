/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

//
// Created by gloom on 2022/5/10.
//

#include "FileIO/AudioModelIO.h"
#include "AudioModel/AudioModel.h"
#include "ConfigUnit/lessConfigure.h"
#include "FileIO/JSONFileIO.h"
#include "../lib/World/tools/audioio.h"


int main(int argc, char *argv[]) {
    std::string FileName;

    if (argc < 2) {
        FileName = "../../test/vaiueo2d.wav";
    } else {
        FileName = argv[1];
    }
    // Open wav File

    int x_length = GetAudioLength(FileName.c_str());
    if (x_length <= 0) {
        if (x_length == 0)
            std::cerr << "error: File \"" << FileName << "\" not found" << std::endl;
        else
            std::cerr << "error: File \"" << FileName << "\" is not a .wav format" << std::endl;
        return EXIT_FAILURE;
    }
    auto *x = new double[x_length];

    // wavread() must be called after GetAudioLength().
    int fs, nbit;
    wavread(FileName.c_str(), &fs, &nbit, x);

    lessConfigure configure{};

    AudioModel audioModel(x, x_length, fs, configure);

    auto less_i = audioModel.GetAudioModel();

    AudioModelIO WriteBinUnit("a.o", less_i);
    WriteBinUnit.SaveAudioModel();

    lessAudioModel less_o{};

    AudioModelIO ReadBinUnit("a.o");
    less_o = ReadBinUnit.ReadAudioModel();

    size_t error_count = 0;

    for (int i = 0; i < less_i.f0_length; ++i) {
        if (less_i.f0[i] != less_o.f0[i]) {
            std::cout << " ERROR: " << i << less_i.f0[i] << "->" << less_o.f0[i] << std::endl;
            error_count++;
        }
        for (int j = 0; j < less_i.w_length; ++j) {
            if (less_i.spectrogram[i][j] != less_i.spectrogram[i][j]) {
                std::cout << " ERROR: " << i << ", " << j << less_i.spectrogram[i][j] << "->" << less_o.spectrogram[i][j] << std::endl;
                error_count++;
            }
            if (less_i.aperiodicity[i][j] != less_i.aperiodicity[i][j]) {
                std::cout << " ERROR: " << i << ", " << j << less_i.aperiodicity[i][j] << "->" << less_o.aperiodicity[i][j] << std::endl;
                error_count++;
            }
        }
    }

    std::cout << "All Test Done! " << [&]() {
        if (error_count == 0) {
            return std::string("No Error");
        } else {
            return "Error: " + std::to_string(error_count);
        }
    }() << std::endl;

    JSONFileIO jsonFileIo(less_o, "a.json");
}