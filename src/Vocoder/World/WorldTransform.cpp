/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "WorldTransform.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <fstream>
#include <utility>
#include <vector>

#include "Core/Exception.h"
#include "Core/Log.h"

namespace lessampler {

    WorldTransform::WorldTransform(const WorldModel &audioModel, RenderParams _shine) : shine(std::move(_shine)) {
        std::vector<double> f0 = audioModel.f0;
        LOG_DEBUG << "Equalizing Pitch...";
        PitchEqualizing(f0);
        LOG_DEBUG << "Time Stretch...";
        TimeStretch(audioModel, f0);
    }

    const WorldModel &WorldTransform::GetTransAudioModel() const {
        return transAudioModel;
    }

    void WorldTransform::PitchEqualizing(std::span<double> f0) const {
        const double freq_avg = GetAvgFreq(f0);
        LOG_DEBUG << std::format("The average frequency is {}", freq_avg);

        for (double &value: f0) {
            if (value == 0.0) {
                continue;
            }
            if (freq_avg == 0.0) {
                value = shine.scale_num;
            } else {
                value = ((value - freq_avg) * shine.modulation / 100.0 + freq_avg) * (shine.scale_num / freq_avg);
            }
        }
    }

    double WorldTransform::GetAvgFreq(std::span<const double> f0) const {
        double freq_avg = 0.0;
        double base_time_percent = 0.0;

        for (std::size_t i = 0; i < f0.size(); ++i) {
            const double time_percent = f0[i];
            if (time_percent >= 1000.0 || time_percent <= 55.0) {
                continue;
            }

            double r = 1.0;
            for (int j = 0; j <= 5; ++j) {
                if (i > static_cast<std::size_t>(j)) {
                    const double q = f0[i - static_cast<std::size_t>(j) - 1] - time_percent;
                    r *= time_percent / (time_percent + q * q);
                } else {
                    r *= 1.0 / (1.0 + time_percent);
                }
            }
            freq_avg += time_percent * r;
            base_time_percent += r;
        }

        if (base_time_percent > 0.0) {
            freq_avg /= base_time_percent;
        }
        return freq_avg;
    }

    void WorldTransform::TimeStretch(const WorldModel &audioModel, std::span<const double> f0) {
        if (shine.required_frame <= 0) {
            throw parameter_error("The target audio frame length is 0");
        }
        if (shine.pitch_step <= 0) {
            throw parameter_error("The pitch step must be greater than zero");
        }
        if (f0.empty() || audioModel.w_length <= 0) {
            throw parameter_error("The audio model is empty");
        }
        if (audioModel.spectrogram.rows() != f0.size() || audioModel.aperiodicity.rows() != f0.size()) {
            throw parameter_error("The audio model frames are inconsistent");
        }
        if (shine.pitch_bend.size() < 2) {
            throw parameter_error("The pitch bend data is too short");
        }

        const auto input_frames = f0.size();
        const auto last_input_frame = input_frames - 1;
        const auto last_ap_frame = audioModel.aperiodicity.rows() - 1;
        const auto max_pitch_index = shine.pitch_bend.size() - 2;
        const auto output_frames = static_cast<std::size_t>(shine.required_frame);
        const auto w_length = static_cast<std::size_t>(audioModel.w_length);

        if (audioModel.spectrogram.cols() < w_length || audioModel.aperiodicity.cols() < w_length) {
            throw parameter_error("The audio model rows are shorter than w_length");
        }

        transAudioModel.f0.assign(output_frames, 0.0);
        transAudioModel.spectrogram.assign(output_frames, w_length, 0.0);
        transAudioModel.aperiodicity.assign(output_frames, w_length, 0.0);
        transAudioModel.x_length = audioModel.x_length;
        transAudioModel.w_length = audioModel.w_length;
        transAudioModel.fs = audioModel.fs;
        transAudioModel.frame_period = audioModel.frame_period;
        transAudioModel.fft_size = audioModel.fft_size;

        double avg_freq = GetAvgFreq(f0);
        if (!(avg_freq > 0.0)) {
            avg_freq = shine.scale_num > 0.0 ? shine.scale_num : 1.0;
        }
        LOG_DEBUG << std::format("Get Stretch Paras, AVG: {}", avg_freq);

        for (std::size_t i = 0; i < output_frames; ++i) {
            const double out_sample_index = audioModel.frame_period * static_cast<double>(i);
            double in_sample_index;
            if (out_sample_index < shine.base_length) {
                in_sample_index = shine.offset + out_sample_index * shine.velocity;
            } else {
                in_sample_index = shine.offset + shine.first_half_fixed_part + (out_sample_index - shine.base_length) * shine.stretch_length;
            }

            double sample_sp_trans_index = in_sample_index / audioModel.frame_period;
            std::size_t sp_trans_index = 0;
            double sp_fraction = 0.0;
            if (sample_sp_trans_index > 0.0) {
                sp_trans_index = static_cast<std::size_t>(std::floor(sample_sp_trans_index));
                if (sp_trans_index > last_input_frame) {
                    sp_trans_index = last_input_frame;
                }
                sp_fraction = sample_sp_trans_index - static_cast<double>(sp_trans_index);
            }
            const std::size_t sp_next_index = std::min(sp_trans_index + 1, last_input_frame);

            double temp_f0 = f0[sp_trans_index];
            if (sp_trans_index < last_input_frame) {
                double next_f0 = f0[sp_next_index];
                if (temp_f0 != 0.0 || next_f0 != 0.0) {
                    if (temp_f0 == 0.0) {
                        temp_f0 = avg_freq;
                    }
                    if (next_f0 == 0.0) {
                        next_f0 = avg_freq;
                    }
                    temp_f0 = temp_f0 * (1.0 - sp_fraction) + next_f0 * sp_fraction;
                }
            }

            double pitch_position = out_sample_index * 0.001 * audioModel.fs / shine.pitch_step;
            std::size_t pitch_index = 0;
            double pitch_fraction = 0.0;
            if (pitch_position > 0.0) {
                pitch_index = static_cast<std::size_t>(std::floor(pitch_position));
                if (pitch_index > max_pitch_index) {
                    pitch_index = max_pitch_index;
                }
                pitch_fraction = pitch_position - static_cast<double>(pitch_index);
                if (pitch_fraction > 1.0) {
                    pitch_fraction = 1.0;
                }
            }

            const double pitch_bend = shine.pitch_bend[pitch_index] * (1.0 - pitch_fraction) + shine.pitch_bend[pitch_index + 1] * pitch_fraction;
            const double pitch_base = shine.scale_num * std::pow(2.0, pitch_bend / 1200.0);
            transAudioModel.f0[i] = pitch_base * std::pow(temp_f0 / avg_freq, shine.modulation * 0.01);

            auto output_row = transAudioModel.spectrogram[i];
            const auto current_row = audioModel.spectrogram[sp_trans_index];
            const auto next_row = audioModel.spectrogram[sp_next_index];
            for (std::size_t j = 0; j < w_length; ++j) {
                output_row[j] = current_row[j] * (1.0 - sp_fraction) + next_row[j] * sp_fraction;
            }

            std::size_t ap_index = sp_trans_index + (sp_fraction > 0.5 ? 1U : 0U);
            if (ap_index > last_ap_frame) {
                ap_index = last_ap_frame;
            }
            std::ranges::copy(audioModel.aperiodicity[ap_index], transAudioModel.aperiodicity[i].begin());
        }

#if defined(DEBUG_MODE) && defined(LESSAMPLER_DUMP_DATA)
        std::ofstream out(shine.output_file_name + ".log", std::ios::out | std::ios::app);
        out << std::format("AVG FREQ: {}\nTrans F0 Length: {}\n", avg_freq, transAudioModel.f0.size());
        out << "Original F0: \n";
        for (double value: f0) {
            out << value << " ";
        }
        out << "\nTransd F0: \n";
        for (double value: transAudioModel.f0) {
            out << value << " ";
        }
        out << "\n";
#endif
    }

}// namespace lessampler
