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
        source_average_f0_ = GetAvgFreq(f0);
        LOG_DEBUG << "Equalizing Pitch...";
        PitchEqualizing(f0);
        LOG_DEBUG << "Time Stretch...";
        TimeStretch(audioModel, f0);
    }

    const WorldModel &WorldTransform::GetTransAudioModel() const {
        return transAudioModel;
    }

    void WorldTransform::PitchEqualizing(std::span<double> f0) const {
        const double freq_avg = source_average_f0_;
        LOG_DEBUG << std::format("The average frequency is {}", freq_avg);

        for (double &value: f0) {
            if (value == 0.0) {
                continue;
            }
            if (freq_avg == 0.0) {
                value = shine.scale_num;
            } else {
                // Map the source contour to the note pitch. Modulation is
                // applied exactly once in TimeStretch.
                value *= shine.scale_num / freq_avg;
            }
        }
    }

    double WorldTransform::GetAvgFreq(std::span<const double> f0) const {
        double freq_avg = 0.0;
        double base_time_percent = 0.0;

        for (std::size_t i = 0; i < f0.size(); ++i) {
            const double time_percent = f0[i];
            if (time_percent >= 1200.0 || time_percent <= 55.0) {
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

        // A first rule-based variance layer. High notes that were shifted up
        // from a lower sample get more harmonic energy, less noise and a
        // little low-harmonic tension; noisy samples get their aperiodic part
        // reduced. The natural case leaves the model bit-exact.
        const double pitch_ratio = source_average_f0_ > 0.0 && shine.scale_num > 0.0
                                           ? shine.scale_num / source_average_f0_
                                           : 1.0;
        const bool high_note = pitch_ratio > std::pow(2.0, 5.0 / 12.0);
        double voicing_db = high_note ? 1.5 : 0.0;
        double breathiness_db = high_note ? -1.5 : 0.0;
        const double tension = high_note ? 0.15 : 0.0;
        double ap_sum = 0.0;
        std::size_t ap_count = 0;
        for (std::size_t i = 0; i < audioModel.f0.size(); ++i) {
            if (audioModel.f0[i] <= 0.0) {
                continue;
            }
            for (double value: audioModel.aperiodicity[i]) {
                ap_sum += value;
                ++ap_count;
            }
        }
        const double mean_ap = ap_count > 0 ? ap_sum / static_cast<double>(ap_count) : 0.0;
        if (mean_ap > 0.6) {
            breathiness_db -= 2.0;
        }
        const double voicing_gain = std::pow(10.0, voicing_db / 20.0);
        const double breathiness_gain = std::pow(10.0, breathiness_db / 20.0);
        const bool style_active = high_note || mean_ap > 0.6;
        const double note_duration_ms = static_cast<double>(output_frames) * audioModel.frame_period;

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
            const double modulation_reference = shine.scale_num > 0.0 ? shine.scale_num : avg_freq;
            if (temp_f0 <= 0.0 || !(modulation_reference > 0.0)) {
                // Unvoiced source frame. pow(0, 0) would otherwise turn it
                // into a target-pitch voiced frame when modulation is zero.
                transAudioModel.f0[i] = 0.0;
            } else {
                transAudioModel.f0[i] = pitch_base *
                                        std::pow(temp_f0 / modulation_reference, shine.modulation * 0.01);
            }

            auto output_row = transAudioModel.spectrogram[i];
            const auto current_row = audioModel.spectrogram[sp_trans_index];
            const auto next_row = audioModel.spectrogram[sp_next_index];
            for (std::size_t j = 0; j < w_length; ++j) {
                output_row[j] = current_row[j] * (1.0 - sp_fraction) + next_row[j] * sp_fraction;
            }

            // Interpolate aperiodicity like the spectral envelope. Copying the
            // nearest frame makes the noise texture jump during time stretch.
            auto output_ap = transAudioModel.aperiodicity[i];
            const auto current_ap = audioModel.aperiodicity[sp_trans_index];
            const auto next_ap = audioModel.aperiodicity[sp_next_index];
            for (std::size_t j = 0; j < w_length; ++j) {
                output_ap[j] = current_ap[j] * (1.0 - sp_fraction) + next_ap[j] * sp_fraction;
            }

            // Clear and cry are spectral voice colors layered on top of the
            // rule-based variance above. Growl is a post-synthesis shaper, so
            // a growl-only note leaves the WORLD model untouched.
            const StyleWeights weights = StyleAt(shine.style, out_sample_index, note_duration_ms);
            const bool spectral_style = weights.clear > 0.0 || weights.cry > 0.0;
            if (style_active || spectral_style) {
                const double frame_voicing_gain = spectral_style
                                                          ? voicing_gain * std::pow(10.0, (0.5 * weights.clear + 1.0 * weights.cry) / 20.0)
                                                          : voicing_gain;
                const double frame_breathiness_gain = spectral_style
                                                              ? breathiness_gain * std::pow(10.0, (-6.0 * weights.clear + 0.5 * weights.cry) / 20.0)
                                                              : breathiness_gain;
                const double frame_tension = spectral_style
                                                     ? tension * (1.0 - 0.5 * weights.clear) + 0.25 * weights.cry
                                                     : tension;
                for (std::size_t j = 0; j < w_length; ++j) {
                    const double aperiodicity = std::clamp(output_ap[j], 0.0, 0.999999);
                    double harmonic = output_row[j] * (1.0 - aperiodicity) * frame_voicing_gain;
                    double noise = output_row[j] * aperiodicity * frame_breathiness_gain;
                    if (frame_tension > 0.0 && audioModel.fft_size > 0) {
                        const double frequency = static_cast<double>(j) * static_cast<double>(audioModel.fs) /
                                                 static_cast<double>(audioModel.fft_size);
                        harmonic *= 1.0 + frame_tension * std::max(0.0, 1.0 - frequency / 2000.0);
                    }
                    const double power = harmonic + noise;
                    output_row[j] = power;
                    output_ap[j] = power > 1e-12 ? std::clamp(noise / power, 0.0, 0.999) : 0.0;
                }
            }
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
