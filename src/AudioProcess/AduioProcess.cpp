
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
// Created by gloom on 2022/5/7.
//
#include <cmath>
#include <iostream>
#include <memory>
#include <utility>
#include <cstring>

#include "AudioProcess.h"
#include "libUTAU/PitchBendDecoder.h"

AduioProcess::AduioProcess(lessAudioModel audioModel, UTAUPara utauPara) : audioModel(audioModel), utauPara(std::move(utauPara)) {
    AllocateMemory();


}

TransAudioModel AduioProcess::GetTransAudioModel() {
    return transAudioModel;
}

void AduioProcess::AllocateMemory() {
    transAudioModel.t_f0_length = audioModel.f0_length;
    transAudioModel.t_f0 = new double[transAudioModel.t_f0_length];
    transAudioModel.t_spectrogram = new double *[transAudioModel.t_f0_length];
    transAudioModel.t_aperiodicity = new double *[transAudioModel.t_f0_length];
    for (int i = 0; i < transAudioModel.t_f0_length; ++i) {
        transAudioModel.t_spectrogram[i] = new double[audioModel.w_length];
        transAudioModel.t_aperiodicity[i] = new double[audioModel.w_length];
    }
}

void AduioProcess::DecodePitchBend() {
    int pitch_length = transAudioModel.t_f0_length;
    int pitch_step = 256;

    if (utauPara.tempoNum == 0)
        utauPara.tempoNum = 120;

    if (utauPara.custom_pitch) {
        pitch_step = static_cast<int>(lround(60.0 / 96.0 / utauPara.tempoNum * audioModel.fs));
        pitch_length = utauPara.output_samples / pitch_step + 1;
        PitchBendDecoder pitchBendDecoder(utauPara.pitch, pitch_length);

        utauPara.pitch_bend = new int[pitch_length + 1];
        std::memcpy(utauPara.pitch_bend, pitchBendDecoder.getPitchBend(), sizeof(int) * pitch_length);
    } else {
        utauPara.pitch_bend = new int[pitch_length + 1];
    }
}
