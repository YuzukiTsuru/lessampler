
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

#include "Utils/exception.h"
#include "Utils/LOG.h"
#include "AudioProcess.h"
#include "libUTAU/PitchBendDecoder.h"

AduioProcess::AduioProcess(lessAudioModel audioModel, UTAUPara utauPara, UTAUFlags flags) : audioModel(audioModel), utauPara(std::move(utauPara)),
                                                                                            flags(flags) {
    YALL_DEBUG_ << "Equalizing Picth...";
    PicthEqualizing();
    YALL_DEBUG_ << "Decode Pitch Bend...";
    DecodePitchBend();
    YALL_DEBUG_ << "Time Stretch...";
    TimeStretch();
}

TransAudioModel AduioProcess::GetTransAudioModel() {
    return transAudioModel;
}

void AduioProcess::PicthEqualizing() {
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
}

void AduioProcess::DecodePitchBend() {
    if (utauPara.tempoNum == 0)
        utauPara.tempoNum = 120;

    if (utauPara.isCustomPitch) {
        pitch_step = static_cast<int>(lround(60.0 / 96.0 / utauPara.tempoNum * audioModel.fs));
        pitch_length = utauPara.output_samples / pitch_step + 1;

        YALL_DEBUG_ << "The Pitch Length is: " + std::to_string(pitch_length);

        PitchBendDecoder pitchBendDecoder(utauPara.pitch, pitch_length);

        utauPara.pitch_bend = new int[pitch_length + 1];
        for (int i = 0; i < pitch_length + 1; ++i) {
            utauPara.pitch_bend[i] = 0;
        }

        std::memcpy(utauPara.pitch_bend, pitchBendDecoder.getPitchBend(), sizeof(int) * pitch_length);
    } else {
        utauPara.pitch_bend = new int[pitch_length + 1];
        for (int i = 0; i < pitch_length + 1; ++i) {
            utauPara.pitch_bend[i] = 0;
        }
    }

    required_frame = static_cast<int>(1000.0 * utauPara.output_samples / audioModel.fs / audioModel.frame_period) + 1;
    YALL_DEBUG_ << "The required frame is: " + std::to_string(required_frame);
    transAudioModel.t_f0_length = required_frame;
}

double AduioProcess::GetAvgFreq() const {
    double freq_avg = 0.0, timePercent, r, p[6], q, base_timePercent = 0;
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

void AduioProcess::TimeStretch() {
    YALL_DEBUG_ << "Allocate memory for target audio f0, sp, ap";

    if (transAudioModel.t_f0_length == 0)
        throw parameter_error("The target audio frame length is 0");

    transAudioModel.t_f0 = new double[transAudioModel.t_f0_length];
    for (int i = 0; i < transAudioModel.t_f0_length; ++i) {
        transAudioModel.t_f0[i] = 0.0;
    }

    transAudioModel.t_spectrogram = new double *[transAudioModel.t_f0_length];
    transAudioModel.t_aperiodicity = new double *[transAudioModel.t_f0_length];
    for (int i = 0; i < transAudioModel.t_f0_length; ++i) {
        transAudioModel.t_spectrogram[i] = new double[audioModel.w_length];
        transAudioModel.t_aperiodicity[i] = new double[audioModel.w_length];
        for (int j = 0; j < audioModel.w_length; ++j) {
            transAudioModel.t_spectrogram[i][j] = 0.0;
            transAudioModel.t_aperiodicity[i][j] = 0.0;
        }
    }

    YALL_DEBUG_ << "Get Stretch Paras";
    auto avg_freq = GetAvgFreq();

    double _sample_sp_trans_index, _sample_ap_trans_index, _out_sample_index, _in_sample_index;
    int _sp_trans_index, _ap_trans_index;

    for (int i = 0; i < transAudioModel.t_f0_length; ++i) {
        _out_sample_index = audioModel.frame_period * i;
        if (_out_sample_index < utauPara.base_length) {
            _in_sample_index = utauPara.offset + _out_sample_index * utauPara.velocity;
        } else {
            _in_sample_index = utauPara.offset + utauPara.firstHalfFixedPart + (_out_sample_index - utauPara.base_length) * utauPara.stretch_length;
        }
        YALL_DEBUG_ << "_in_sample_index -> " + std::to_string(_in_sample_index);
        YALL_DEBUG_ << "_out_sample_index -> " + std::to_string(_out_sample_index);

        _sample_sp_trans_index = _in_sample_index / audioModel.frame_period;
        _sp_trans_index = static_cast<int>(floor(_sample_sp_trans_index));
        _sample_sp_trans_index -= _sp_trans_index;

        YALL_DEBUG_ << "_sp_trans_index -> " + std::to_string(_sp_trans_index);
        YALL_DEBUG_ << "_sample_sp_trans_index -> " + std::to_string(_sp_trans_index + _sample_sp_trans_index);

        auto temp_f0 = audioModel.f0[_sp_trans_index];
        if (_sp_trans_index < audioModel.f0_length - 1) {
            auto temp_f0_next = audioModel.f0[_sp_trans_index + 1];
            if (temp_f0 != 0 || temp_f0_next != 0) {
                if (temp_f0 == 0) {
                    temp_f0 = avg_freq;
                }
                if (temp_f0_next == 0) {
                    temp_f0_next = avg_freq;
                }
                temp_f0 = temp_f0 * (1.0 - _sample_sp_trans_index) + temp_f0_next * _sample_sp_trans_index;
            }
        }

        _sample_ap_trans_index = _out_sample_index * 0.001 * audioModel.fs / pitch_step;
        _ap_trans_index = static_cast<int>(floor(_sample_ap_trans_index));
        _sample_ap_trans_index -= _ap_trans_index;

        if (_ap_trans_index >= pitch_length) {
            _ap_trans_index = pitch_length - 1;
            _sample_sp_trans_index = 0.0;
        }

        YALL_DEBUG_ << "_ap_trans_index -> " + std::to_string(_ap_trans_index);
        YALL_DEBUG_ << "_sample_ap_trans_index -> " + std::to_string(_ap_trans_index + _sample_ap_trans_index);

        YALL_DEBUG_ << "Apply Pitch Shift With Pitch Bend";
        auto pitch_base = utauPara.scaleNum * pow(2, (utauPara.pitch_bend[_ap_trans_index] * (1.0 - _sample_ap_trans_index) +
                                                      utauPara.pitch_bend[_ap_trans_index + 1] * _sample_ap_trans_index) / 1200.0);

        YALL_DEBUG_ << "Trans F0 " + std::to_string(transAudioModel.t_f0[i]) + " Add " + std::to_string(pitch_base);
        transAudioModel.t_f0[i] = pitch_base;

        transAudioModel.t_f0[i] = transAudioModel.t_f0[i] * pow(temp_f0 / avg_freq, utauPara.modulation * 0.01);

        YALL_DEBUG_ << "Trans SP ";
        for (int j = 0; j < audioModel.w_length; ++j) {
            if (_sp_trans_index < audioModel.f0_length - 1) {
                transAudioModel.t_spectrogram[i][j] = audioModel.spectrogram[_sp_trans_index][j] * (1.0 - _sample_sp_trans_index) +
                                                      audioModel.spectrogram[_sp_trans_index + 1][j] * _sample_sp_trans_index;
            } else {
                transAudioModel.t_spectrogram[i][j] = audioModel.spectrogram[audioModel.f0_length - 1][j];
            }
        }

        YALL_DEBUG_ << "Trans AP";
        _ap_trans_index = _sp_trans_index;
        if (_sample_sp_trans_index > 0.5) {
            ++_ap_trans_index;
        }

        for (int j = 0; j < audioModel.w_length; ++j) {
            if (_ap_trans_index < audioModel.f0_length) {
                transAudioModel.t_aperiodicity[i][j] = audioModel.aperiodicity[_ap_trans_index][j];
            } else {
                transAudioModel.t_aperiodicity[i][j] = audioModel.aperiodicity[audioModel.f0_length - 1][j];
            }
        }
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

