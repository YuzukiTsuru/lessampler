
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

AduioProcess::AduioProcess(lessAudioModel audioModel, UTAUPara utauPara, UTAUFlags flags) : audioModel(audioModel),
                                                                                            utauPara(std::move(
                                                                                                    utauPara)),
                                                                                            flags(flags) {
    AllocateMemory();
    DecodePitchBend();
    PitchShift();
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
    pitch_length = transAudioModel.t_f0_length;

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

void AduioProcess::GetAvgFreq() {
    double timePercent, r, p[6], q, base_timePercent = 0;
    for (int i = 0; i < audioModel.f0_length; ++i) {
        timePercent = audioModel.f0[i];
        if (timePercent < 1000.0 && timePercent > 55.0) {
            r = 1.0;
            for (int j = 0; j <= 5; ++j) {
                if (i > j) {
                    q = audioModel.f0[i - j - 1] - timePercent;
                    p[j] = timePercent / (timePercent + q * q);
                } else {
                    p[j] = 1 / (1 + timePercent);
                }
                r *= p[j];
            }
            freq_avg += timePercent * r;
            base_timePercent += r;
        }
    }
    if (base_timePercent > 0) freq_avg /= base_timePercent;
}

void AduioProcess::PitchShift() {
    double temp_input, temp_output, last_stretch_pos, temp_pre_cross_length = 0, temp_virt, temp_pit, temp_f0;
    int loop_times, loop_count, num_temp_virt, num_temp_pit, num_temp_pit_ap;
    bool direct = false;

    if (!flags.isLoopStratch) {
        temp_pre_cross_length = utauPara.cross_length - audioModel.frame_period / 2.0;
        loop_times = static_cast<int>(floor(utauPara.cross_length / temp_pre_cross_length)) - 1;
        if (loop_times > 0)
            last_stretch_pos = utauPara.pre_cross_length / (utauPara.cross_length - temp_pre_cross_length * loop_times);
        else
            flags.isLoopStratch = true;
    }

    loop_count = 1;

    GetAvgFreq();

    for (int i = 0; i < audioModel.f0_length; ++i) {
        if (flags.isLoopStratch) {
            temp_output = audioModel.frame_period * i;
            if (temp_output < utauPara.base_length) {
                temp_input = utauPara.offset + temp_output * utauPara.velocity;
            } else if (temp_output < (utauPara.base_length + temp_pre_cross_length * loop_times)) {
                if (direct) {
                    temp_input = utauPara.wave_length - utauPara.lastUnusedPart -
                                 (temp_output - utauPara.base_length - temp_pre_cross_length * (loop_count - 1));
                    if (temp_output - utauPara.base_length > temp_pre_cross_length * loop_count) {
                        ++loop_count;
                        direct = false;
                    }
                } else {
                    temp_input = utauPara.offset + utauPara.firstHalfFixedPart +
                                 (temp_output - utauPara.base_length - temp_pre_cross_length * (loop_count - 1));
                    if (temp_output - utauPara.base_length > temp_pre_cross_length * loop_count) {
                        ++loop_count;
                        direct = true;
                    }
                }
            } else {
                if (direct) {
                    temp_input = utauPara.offset + utauPara.firstHalfFixedPart +
                                 (temp_output - utauPara.base_length - temp_pre_cross_length * loop_times) *
                                 last_stretch_pos;
                } else {
                    temp_input = utauPara.wave_length - utauPara.lastUnusedPart -
                                 (temp_output - utauPara.base_length - temp_pre_cross_length * loop_times) *
                                 last_stretch_pos;
                }
            }
        } else {
            temp_output = audioModel.frame_period * i;
            if (temp_output < utauPara.base_length) {
                temp_input = utauPara.offset + temp_output * utauPara.velocity;
            } else {
                temp_input = utauPara.offset + utauPara.firstHalfFixedPart +
                             (temp_output - utauPara.base_length) * utauPara.stretch_length;
            }
        }

        temp_virt = temp_input / audioModel.frame_period;
        num_temp_virt = static_cast<int>(floor(temp_virt));
        temp_virt = temp_virt - num_temp_virt;
        temp_f0 = audioModel.f0[num_temp_virt];
        if (num_temp_virt < audioModel.f0_length - 1) {
            auto temp_f0_n = audioModel.f0[num_temp_virt + 1];
            if (temp_f0 != 0 || temp_f0_n != 0) {
                if (temp_f0 == 0)
                    temp_f0 = freq_avg;
                if (temp_f0_n == 0)
                    temp_f0_n = freq_avg;
                temp_f0 = temp_f0 * (1.0 - temp_virt) + temp_f0_n * temp_virt;
            }
        }

        temp_pit = temp_output * 0.001 * audioModel.fs / pitch_step;
        num_temp_pit = static_cast<int>(floor(temp_pit));
        temp_pit = temp_pit - num_temp_pit;
        if (num_temp_pit >= pitch_length) {
            num_temp_pit = pitch_length - 1;
            temp_virt = 0.0;
        }

        transAudioModel.t_f0[i] = utauPara.scaleNum * pow(2, (utauPara.pitch_bend[num_temp_pit] * (1.0 - temp_pit) +
                                                              utauPara.pitch_bend[num_temp_pit + 1] * temp_pit) /
                                                             1200.0);
        transAudioModel.t_f0[i] *= pow(temp_f0 / freq_avg, utauPara.modulation * 0.01);

        for (int j = 0; j < audioModel.fft_size / 2; ++j) {
            if (num_temp_virt < audioModel.f0_length - 1) {
                transAudioModel.t_spectrogram[i][j] = audioModel.spectrogram[num_temp_virt][j] * (1.0 - temp_virt) +
                                                      audioModel.spectrogram[num_temp_virt + 1][j] * temp_virt;
            } else {
                transAudioModel.t_spectrogram[i][j] = audioModel.spectrogram[audioModel.f0_length - 1][j];
            }
        }

        num_temp_pit_ap = num_temp_pit;
        if (temp_virt > 0.5) {
            ++num_temp_pit_ap;
        }

        for (int j = 0; j < audioModel.fft_size / 2; ++j) {
            if (num_temp_pit_ap < audioModel.f0_length) {
                transAudioModel.t_aperiodicity[i][j] = audioModel.aperiodicity[num_temp_pit_ap][j];
            } else {
                transAudioModel.t_aperiodicity[i][j] = audioModel.aperiodicity[audioModel.f0_length - 1][j];
            }
        }
    }
}

