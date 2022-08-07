
/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

//
// Created by gloom on 2022/5/7.
//
#include <cmath>
#include <utility>

#include "Utils/exception.h"
#include "Utils/LOG.h"
#include "AudioProcess.h"
#include "StaticCast.h"
#include <fstream>

AudioProcess::AudioProcess(lessAudioModel audioModel, ShinePara shine) : audioModel(std::move(audioModel)), shine(std::move(shine)) {
    YALL_DEBUG_ << "Init TransAudioModel default data...";
    InitTransAudioModel();
    YALL_DEBUG_ << "Equalizing Picth...";
    PicthEqualizing();
    YALL_DEBUG_ << "Time Stretch...";
    TimeStretch();
}

lessAudioModel AudioProcess::GetTransAudioModel() {
    return transAudioModel;
}

void AudioProcess::InitTransAudioModel() {
    transAudioModel = audioModel;
}

void AudioProcess::PicthEqualizing() {
    auto freq_avg = GetAvgFreq();
    YALL_DEBUG_ << "The average frequency is " + std::to_string(freq_avg);
    if (freq_avg == 0.0) {
        for (double &i: audioModel.f0) {
            if (i != 0.0) {
                i = shine.scale_num;
            } else {
                i = 0;
            }
        }
    } else {
        for (double &i: audioModel.f0) {
            if (i != 0.0) {
                i = ((i - freq_avg) * shine.modulation / 100.0 + freq_avg) * (shine.scale_num / freq_avg);
            } else {
                i = 0;
            }
        }
    }
}

double AudioProcess::GetAvgFreq() const {
    double freq_avg = 0.0, timePercent, r, p[6], q, base_timePercent = 0;
    for (int i = 0; i < audioModel.f0.size(); ++i) {
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

void AudioProcess::TimeStretch() {
    YALL_DEBUG_ << "Allocate memory for target audio f0, sp, ap";

    if (shine.required_frame == 0)
        throw parameter_error("The target audio frame length is 0");

    transAudioModel.f0.resize(shine.required_frame);
    transAudioModel.spectrogram.resize(transAudioModel.f0.size(), std::vector<double>(audioModel.w_length));
    transAudioModel.aperiodicity.resize(transAudioModel.f0.size(), std::vector<double>(audioModel.w_length));

    auto avg_freq = GetAvgFreq();
    YALL_DEBUG_ << "Get Stretch Paras, AVG: " + std::to_string(avg_freq);

    double _sample_sp_trans_index, _sample_ap_trans_index, _out_sample_index, _in_sample_index;
    int _sp_trans_index, _ap_trans_index;

    for (int i = 0; i < transAudioModel.f0.size(); ++i) {
        _out_sample_index = audioModel.frame_period * i;
        if (_out_sample_index < shine.base_length) {
            _in_sample_index = shine.offset + _out_sample_index * shine.velocity;
        } else {
            _in_sample_index = shine.offset + shine.first_half_fixed_part + (_out_sample_index - shine.base_length) * shine.stretch_length;
        }
        YALL_DUMP_ << "_in_sample_index -> " + std::to_string(_in_sample_index);
        YALL_DUMP_ << "_out_sample_index -> " + std::to_string(_out_sample_index);

        _sample_sp_trans_index = _in_sample_index / audioModel.frame_period;
        _sp_trans_index = static_cast<int>(floor(_sample_sp_trans_index));
        _sample_sp_trans_index -= _sp_trans_index;

        YALL_DUMP_ << "_sp_trans_index -> " + std::to_string(_sp_trans_index);
        YALL_DUMP_ << "_sample_sp_trans_index -> " + std::to_string(_sp_trans_index + _sample_sp_trans_index);

        auto temp_f0 = audioModel.f0[_sp_trans_index];
        if (_sp_trans_index < audioModel.f0.size() - 1) {
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

        _sample_ap_trans_index = _out_sample_index * 0.001 * audioModel.fs / shine.pitch_step;
        _ap_trans_index = static_cast<int>(floor(_sample_ap_trans_index));
        _sample_ap_trans_index -= _ap_trans_index;

        if (_ap_trans_index >= shine.pitch_length) {
            _ap_trans_index = shine.pitch_length - 1;
            _sample_sp_trans_index = 0.0;
        }

        YALL_DUMP_ << "_ap_trans_index -> " + std::to_string(_ap_trans_index);
        YALL_DUMP_ << "_sample_ap_trans_index -> " + std::to_string(_ap_trans_index + _sample_ap_trans_index);

        YALL_DUMP_ << "Apply Pitch Shift With Pitch Bend";
        auto pitch_base = shine.scale_num * pow(2, (shine.pitch_bend[_ap_trans_index] * (1.0 - _sample_ap_trans_index) +
                                                    shine.pitch_bend[_ap_trans_index + 1] * _sample_ap_trans_index) / 1200.0);

        YALL_DUMP_ << "Trans F0 " + std::to_string(transAudioModel.f0[i]) + " Add " + std::to_string(pitch_base);
        transAudioModel.f0[i] = pitch_base;

        transAudioModel.f0[i] = transAudioModel.f0[i] * pow(temp_f0 / avg_freq, shine.modulation * 0.01);

        YALL_DUMP_ << "Trans SP ";
        for (int j = 0; j < audioModel.w_length; ++j) {
            if (_sp_trans_index < audioModel.f0.size() - 1) {
                transAudioModel.spectrogram[i][j] = audioModel.spectrogram[_sp_trans_index][j] * (1.0 - _sample_sp_trans_index) +
                                                    audioModel.spectrogram[_sp_trans_index + 1][j] * _sample_sp_trans_index;
            } else {
                transAudioModel.spectrogram[i][j] = audioModel.spectrogram[audioModel.f0.size() - 1][j];
            }
        }

        YALL_DUMP_ << "Trans AP";
        _ap_trans_index = _sp_trans_index;
        if (_sample_sp_trans_index > 0.5) {
            ++_ap_trans_index;
        }

        for (int j = 0; j < audioModel.w_length; ++j) {
            if (_ap_trans_index < audioModel.f0.size()) {
                transAudioModel.aperiodicity[i][j] = audioModel.aperiodicity[_ap_trans_index][j];
            } else {
                transAudioModel.aperiodicity[i][j] = audioModel.aperiodicity[audioModel.f0.size() - 1][j];
            }
        }
    }

#ifdef DEBUG_MODE
#ifdef DUMP_DATA
    std::ofstream of(shine.output_file_name + ".log", std::ios::out | std::ios::app);

    of << "AVG FREQ: " << avg_freq << "\n"
       << "Trans F0 Length: " << transAudioModel.f0.size() << "\n";
    of << "Original F0: \n";
    for (double j : audioModel.f0) {
        of << j << " ";
    }

    of << "\nTransd F0: \n";
    for (double j : transAudioModel.f0) {
        of << j << " ";
    }
    of << "\n";

    of << "\nTransd SP: \n";
    for (int j = 0; j < transAudioModel.f0.size(); ++j) {
        for (int i = 0; i < transAudioModel.w_length; ++i) {
            of << transAudioModel.spectrogram[j][i] << " ";
        }
    }
    of << "\n";

    of << "\nTransd AP: \n";
    for (int j = 0; j < transAudioModel.f0.size(); ++j) {
        for (int i = 0; i < transAudioModel.w_length; ++i) {
            of << transAudioModel.aperiodicity[j][i] << " ";
        }
    }
    of << "\n";

    of.close();
#endif
#endif
}

[[maybe_unused]] void AudioProcess::interp1(const double *x, const double *y, int x_length, const double *xi, int xi_length, double *yi) {
    auto *h = new double[x_length - 1];
    int *k = new int[xi_length];

    for (int i = 0; i < x_length - 1; ++i) {
        h[i] = x[i + 1] - x[i];
    }

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

[[maybe_unused]] void AudioProcess::histc(const double *x, int x_length, const double *edges, int edges_length, int *index) {
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


