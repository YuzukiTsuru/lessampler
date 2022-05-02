//
// Created by Ghost Gloomy on 2020/8/27.
//

#include "BinWriteUnit.h"
#include <fstream>

void less::BinWriteUnit::F0BIN(const std::string &Path, AudioModel audioModel) {
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

void less::BinWriteUnit::SPBIN(const std::string &Path, AudioModel audioModel) {
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

void less::BinWriteUnit::APBIN(const std::string &Path, AudioModel audioModel) {
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
