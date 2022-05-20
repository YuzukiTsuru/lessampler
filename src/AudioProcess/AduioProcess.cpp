
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

#include "AudioProcess.h"

#include <utility>

AduioProcess::AduioProcess(lessAudioModel audioModel, UTAUPara utauPara) : audioModel(audioModel), utauPara(std::move(utauPara)) {

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