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

#include <world/synthesis.h>

#include "Utils/LOG.h"
#include "world/synthesisrealtime.h"

Synthesis::Synthesis(lessAudioModel audioModel, int x_length) : audioModel(audioModel), x_length(x_length) {
    YALL_DEBUG_ << "Allocate Out Memory, Length: " + std::to_string(x_length);
    AllocateMemory();
    YALL_DEBUG_ << "Synthesis Wav...";
    SynthesisWav();
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

    int offset = 0;
    int index = 0;
    for (int i = 0; i < audioModel.f0_length;) {
        // Add one frame ('i' shows the frame index that should be added)
        if (AddParameters(&audioModel.f0[i], 1, &audioModel.spectrogram[i], &audioModel.aperiodicity[i], &synthesizer) == 1) {
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

