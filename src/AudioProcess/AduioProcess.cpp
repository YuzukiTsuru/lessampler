
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
#include <utility>
#include <cstring>

#include "Utils/LOG.h"
#include "AudioProcess.h"
#include "libUTAU/PitchBendDecoder.h"

AduioProcess::AduioProcess(lessAudioModel audioModel, UTAUPara utauPara, UTAUFlags flags) : audioModel(audioModel), utauPara(std::move(utauPara)),
                                                                                            flags(flags) {
    DecodePitchBend();
    YALL_DEBUG_ << "Equalizing Pitch...";
    PitchEqualize();
    YALL_DEBUG_ << "Time Stretch...";
    TimeStretch();
}

TransAudioModel AduioProcess::GetTransAudioModel() {
    return transAudioModel;
}

void AduioProcess::DecodePitchBend() {
    pitch_length = transAudioModel.t_f0_length;

    if (utauPara.tempoNum == 0)
        utauPara.tempoNum = 120;

    if (utauPara.isCustomPitch) {
        pitch_step = static_cast<int>(lround(60.0 / 96.0 / utauPara.tempoNum * audioModel.fs));
        pitch_length = utauPara.output_samples / pitch_step + 1;
        PitchBendDecoder pitchBendDecoder(utauPara.pitch, pitch_length);

        utauPara.pitch_bend = new int[pitch_length + 1];
        std::memcpy(utauPara.pitch_bend, pitchBendDecoder.getPitchBend(), sizeof(int) * pitch_length);
    } else {
        utauPara.pitch_bend = new int[pitch_length + 1];
    }
}

double AduioProcess::GetAvgFreq() const {
    double freq_avg, timePercent, r, p[6], q, base_timePercent = 0;
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
    return freq_avg;
}

void AduioProcess::PitchEqualize() {
    auto freq_avg = GetAvgFreq();
    YALL_DEBUG_ << "The average frequency is " + std::to_string(freq_avg);
    if (freq_avg == 0.0) {
        for (int i = 0; i < audioModel.f0_length; ++i) {
            if (audioModel.f0[i] != 0.0) {
                audioModel.f0[i] = utauPara.scaleNum;
            } else {
                audioModel.f0[i] = 0;
            }
        }
    } else {
        for (int i = 0; i < audioModel.f0_length; ++i) {
            if (audioModel.f0[i] != 0.0) {
                audioModel.f0[i] = ((audioModel.f0[i] - freq_avg) * utauPara.modulation / 100.0 + freq_avg) * (utauPara.scaleNum / freq_avg);
            } else {
                audioModel.f0[i] = 0;
            }
        }
    }
#ifdef DEBUG_MODE
    std::stringstream ss;
    ss << "\n";
    for (int i = 0; i < audioModel.f0_length; ++i) {
        ss << audioModel.f0[i] << std::endl;
    }
    YALL_DEBUG_ << ss.str();
#endif
}

void AduioProcess::TimeStretch() {
    YALL_DEBUG_ << "Allocate memory for target audio f0";

    auto required_frame = static_cast<int>(lround(utauPara.requiredLength / audioModel.frame_period));
    YALL_DEBUG_ << "The required frame is: " + std::to_string(required_frame);
    transAudioModel.t_f0 = new double[required_frame];
    for (int i = 0; i < required_frame; ++i) {
        transAudioModel.t_f0[i] = 0.0;
    }

}

void AduioProcess::interp1(const double *x, const double *y, int x_length, const double *xi, int xi_length, double *yi) {
    auto *h = new double[x_length - 1];
    int *k = new int[xi_length];

    for (int i = 0; i < x_length - 1; ++i) h[i] = x[i + 1] - x[i];
    for (int i = 0; i < xi_length; ++i) {
        k[i] = 0;
    }

    histc(x, x_length, xi, xi_length, k);

    for (int i = 0; i < xi_length; ++i) {
        double s = (xi[i] - x[k[i] - 1]) / h[k[i] - 1];
        yi[i] = y[k[i] - 1] + s * (y[k[i]] - y[k[i] - 1]);
    }

    delete[] k;
    delete[] h;
}

void AduioProcess::histc(const double *x, int x_length, const double *edges, int edges_length, int *index) {
    int count = 1;

    int i = 0;
    for (; i < edges_length; ++i) {
        index[i] = 1;
        if (edges[i] >= x[0]) break;
    }
    for (; i < edges_length; ++i) {
        if (edges[i] < x[count]) {
            index[i] = count;
        } else {
            index[i--] = count++;
        }
        if (count == x_length) break;
    }
    count--;
    for (i++; i < edges_length; ++i) index[i] = count;
}
