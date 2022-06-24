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
// Created by gloom on 2022/5/20.
//

#include "Synthesis.h"

#include <utility>

#include "Utils/LOG.h"
#include "Utils/Timer.h"

#include <world/synthesisrealtime.h>

Synthesis::Synthesis(lessAudioModel audioModel, int x_length) : audioModel(std::move(audioModel)), x_length(x_length) {
    YALL_DEBUG_ << "Allocate Out Memory, Length: " + std::to_string(x_length);
    AllocateMemory();
    YALL_DEBUG_ << "Synthesis Audio...";
    uint64_t tmsStart = get_perf_count();
    SynthesisWav();
    uint64_t tmsEnd = get_perf_count();
    uint64_t usVal = (tmsEnd - tmsStart) / 10000;
    YALL_INFO_ << "Synthesis Audio: " + std::to_string(usVal) + " ms";
}

Synthesis::~Synthesis() {
    delete x;
}

double *Synthesis::GetWavData() {
    return x;
}

void Synthesis::AllocateMemory() {
    x = new double[x_length];
}

void Synthesis::SynthesisWav() const {
    WorldSynthesizer synthesizer = {0};
    int buffer_size = 64;
    InitializeSynthesizer(audioModel.fs, audioModel.frame_period,
                          audioModel.fft_size, buffer_size, 100, &synthesizer);

    auto f0 = new double[audioModel.f0.size()];
    std::copy(audioModel.f0.begin(), audioModel.f0.end(), f0);
    auto spectrogram = new double *[audioModel.f0_length];
    auto aperiodicity = new double *[audioModel.f0_length];
    for (int i = 0; i < audioModel.f0_length; ++i) {
        spectrogram[i] = new double[audioModel.w_length];
        aperiodicity[i] = new double[audioModel.w_length];
        std::copy(audioModel.spectrogram[i].begin(), audioModel.spectrogram[i].end(), spectrogram[i]);
        std::copy(audioModel.aperiodicity[i].begin(), audioModel.aperiodicity[i].end(), aperiodicity[i]);
    }

    int offset = 0;
    int index;
    for (int i = 0; i < audioModel.f0_length;) {
        // Add one frame ('i' shows the frame index that should be added)
        if (AddParameters(&f0[i], 1, &spectrogram[i], &aperiodicity[i], &synthesizer) == 1) {
            ++i;
        }

        // Synthesize speech with length of buffer_size sample.
        // It is repeated until the function returns 0
        // (it suggests that the synthesizer cannot generate speech).
        while (Synthesis2(&synthesizer) != 0) {
            index = offset * buffer_size;
            for (int j = 0; j < buffer_size; ++j)
                x[j + index] = synthesizer.buffer[j];
            offset++;
        }

        // Check the "Lock" (Please see synthesisrealtime.h)
        if (IsLocked(&synthesizer) == 1) {
            YALL_WARN_ << "Synthesis Buffer Locked";
            break;
        }
    }
    DestroySynthesizer(&synthesizer);
}

