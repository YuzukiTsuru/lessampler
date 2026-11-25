/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Vocoder/Neural/FeatureAssembler.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <format>
#include <utility>
#include <vector>

#include "Audio/WavIO.h"
#include "Core/Exception.h"
#include "Core/Log.h"
#include "Vocoder/Common/F0Estimator.h"
#include "Vocoder/Neural/MelSpectrogram.h"
#include "Vocoder/Neural/NeuralFeatures.h"
#include "Vocoder/Neural/NeuralModel.h"

namespace lessampler {

    namespace {

        constexpr int model_rate = 44100;
        constexpr int hop = MelSpectrogram::hop_size;
        constexpr int bins = MelSpectrogram::mel_bins;
        constexpr float silence_threshold = -9.2f;

        std::int64_t milliseconds_to_samples(double milliseconds, int sample_rate) {
            if (!std::isfinite(milliseconds)) {
                return 0;
            }
            return static_cast<std::int64_t>(
                    std::llround(std::max(0.0, milliseconds) * static_cast<double>(sample_rate) / 1000.0));
        }

        double root_mean_square(std::span<const double> samples) {
            if (samples.empty()) {
                return 0.0;
            }
            double sum = 0.0;
            for (double value: samples) {
                if (std::isfinite(value)) {
                    sum += value * value;
                }
            }
            return std::sqrt(sum / static_cast<double>(samples.size()));
        }

        double active_rms(std::span<const double> samples) {
            constexpr double threshold = 0.01;
            double sum = 0.0;
            std::size_t active = 0;
            for (double value: samples) {
                if (std::isfinite(value) && std::abs(value) > threshold) {
                    sum += value * value;
                    ++active;
                }
            }
            if (active == 0) {
                return root_mean_square(samples);
            }
            return std::sqrt(sum / static_cast<double>(active));
        }

        std::vector<double> read_model_rate(const std::filesystem::path &path) {
            auto data = WavIO::ReadWav(path);
            if (data.sample_rate == model_rate) {
                return std::move(data.samples);
            }
            return MelSpectrogram::Resample(data.samples, data.sample_rate, model_rate);
        }

        std::vector<double> silence(std::size_t count) {
            return std::vector<double>(count, 0.0);
        }

    }// namespace

    FeatureAssembleResult AssembleFeatureTimeline(std::span<const FeatureSegment> segments, int sample_rate,
                                                  double peak_limit, const Config &configure) {
        if (segments.empty()) {
            throw parameter_error("The feature plan has no notes");
        }
        if (sample_rate <= 0) {
            sample_rate = model_rate;
        }
        if (configure.neural_model_path.empty()) {
            throw parameter_error("Feature assembly requires [neural] model_path in lessampler.less");
        }

        double total_ms = 0.0;
        for (const auto &segment: segments) {
            total_ms = std::max(total_ms, segment.position_ms + std::max(0.0, segment.length_ms));
        }
        const auto total_samples = static_cast<std::size_t>(
                std::max<std::int64_t>(1, milliseconds_to_samples(total_ms, model_rate)));
        const auto max_frames = total_samples / static_cast<std::size_t>(hop) + 3;

        std::vector<double> sum_mel(max_frames * static_cast<std::size_t>(bins), 0.0);
        std::vector<double> log_f0(max_frames, 0.0);
        std::vector<double> voiced_weight(max_frames, 0.0);
        std::vector<std::uint8_t> active(max_frames, 0);
        std::vector<double> reference(total_samples, 0.0);
        std::size_t last_frame = 0;
        bool any_active = false;

        auto model = LoadNeuralModel(configure.neural_model_path, configure.neural_threads);
        static const MelSpectrogram extractor;

        for (const auto &segment: segments) {
            auto resampled = read_model_rate(segment.input);
            if (resampled.empty()) {
                continue;
            }

            auto skip = milliseconds_to_samples(segment.skip_ms, model_rate);
            auto length = milliseconds_to_samples(segment.length_ms, model_rate);
            if (length <= 0) {
                continue;
            }

            std::vector<double> wave(static_cast<std::size_t>(length), 0.0);
            for (std::int64_t i = 0; i < length; ++i) {
                const auto index = skip + i;
                if (index >= 0 && static_cast<std::size_t>(index) < resampled.size()) {
                    wave[static_cast<std::size_t>(i)] = resampled[static_cast<std::size_t>(index)];
                }
            }

            auto position = milliseconds_to_samples(segment.position_ms, model_rate);
            auto gains = segment.envelope.GainCurve(wave.size(), model_rate);

            // Negative positions drop the head of the segment, matching the
            // wavtool's start<0 handling.
            if (position < 0) {
                const auto drop = static_cast<std::size_t>(-position);
                if (drop >= wave.size()) {
                    continue;
                }
                wave.erase(wave.begin(), wave.begin() + static_cast<std::ptrdiff_t>(drop));
                gains.erase(gains.begin(), gains.begin() + static_cast<std::ptrdiff_t>(drop));
                position = 0;
            }

            const auto start = static_cast<std::size_t>(position);
            for (std::size_t i = 0; i < wave.size(); ++i) {
                if (start + i < reference.size()) {
                    reference[start + i] += wave[i] * gains[i];
                }
            }

            // Extract raw mel; a single global level shift is applied after
            // assembly so per-note dynamics are not flattened.
            const auto mel = extractor.Extract(wave, model_rate, 0.0);
            const auto frames = mel.size() / static_cast<std::size_t>(bins);
            if (frames == 0) {
                continue;
            }

            const auto track = EstimateF0(wave, model_rate, configure);
            std::vector<float> f0;
            std::vector<float> uv;
            BuildMelF0Track(track, frames, f0, uv);

            const auto global_start = start / static_cast<std::size_t>(hop);
            for (std::size_t k = 0; k < frames; ++k) {
                const auto global_frame = global_start + k;
                if (global_frame >= max_frames) {
                    break;
                }
                const auto center = std::min(gains.size() - 1, k * static_cast<std::size_t>(hop) + 128);
                const double gain = std::clamp(gains[center], 0.0, 2.0);
                if (gain <= 1e-9) {
                    continue;
                }

                active[global_frame] = 1;
                any_active = true;
                last_frame = std::max(last_frame, global_frame);

                for (int bin = 0; bin < bins; ++bin) {
                    const auto index = k * static_cast<std::size_t>(bins) + static_cast<std::size_t>(bin);
                    sum_mel[global_frame * static_cast<std::size_t>(bins) + static_cast<std::size_t>(bin)] +=
                            static_cast<double>(std::exp(mel[index])) * gain;
                }

                if (uv[k] == 0.0f && f0[k] > 0.0f) {
                    log_f0[global_frame] += std::log(static_cast<double>(f0[k])) * gain;
                    voiced_weight[global_frame] += gain;
                }
            }

            LOG_DEBUG << "Feature segment: " + segment.input.filename().string() + " (" +
                                 std::to_string(frames) + " frames)";
        }

        if (!any_active) {
            return {silence(static_cast<std::size_t>(milliseconds_to_samples(total_ms, sample_rate))), sample_rate};
        }

        const double reference_active = active_rms(reference);
        const double global_shift = (reference_active > 1e-9 && configure.neural_target_rms > 0.0)
                                            ? std::clamp(std::log(configure.neural_target_rms / reference_active), -3.0, 3.0)
                                            : 0.0;
        LOG_INFO << std::format("Feature level shift: {:.3f} dB", global_shift * 8.685889638);

        const auto frames = last_frame + 1;
        std::vector<float> global_mel(frames * static_cast<std::size_t>(bins), std::log(1e-5f));
        for (std::size_t frame = 0; frame < frames; ++frame) {
            for (int bin = 0; bin < bins; ++bin) {
                const auto index = frame * static_cast<std::size_t>(bins) + static_cast<std::size_t>(bin);
                if (sum_mel[index] > 0.0) {
                    global_mel[index] = static_cast<float>(std::log(std::max(sum_mel[index], 1e-5)) + global_shift);
                }
            }
        }

        std::vector<float> global_f0(frames, 0.0f);
        std::vector<float> global_uv(frames, 1.0f);
        bool any_voiced = false;
        for (std::size_t frame = 0; frame < frames; ++frame) {
            if (voiced_weight[frame] > 0.0) {
                global_f0[frame] = static_cast<float>(std::exp(log_f0[frame] / voiced_weight[frame]));
                global_uv[frame] = 0.0f;
                any_voiced = true;
            }
        }
        if (!any_voiced) {
            return {silence(static_cast<std::size_t>(milliseconds_to_samples(total_ms, sample_rate))), sample_rate};
        }
        FillUnvoicedF0(global_f0, global_uv, frames);

        if (*std::ranges::max_element(global_mel) < silence_threshold) {
            return {silence(static_cast<std::size_t>(milliseconds_to_samples(total_ms, sample_rate))), sample_rate};
        }

        LOG_INFO << "Feature assembly: " + std::to_string(frames) + " frames, " +
                            std::to_string(segments.size()) + " segments";
        const auto neural = model->Synthesize(global_mel, frames, global_f0, global_uv);
        if (neural.empty()) {
            throw parameter_error("The neural vocoder returned an empty waveform");
        }

        std::vector<double> output(neural.begin(), neural.end());
        if (output.size() > total_samples) {
            output.resize(total_samples);
        } else if (output.size() < total_samples) {
            output.resize(total_samples, 0.0);
        }

        const double reference_rms = root_mean_square(reference);
        const double output_rms = root_mean_square(output);
        LOG_INFO << std::format("Feature level: ref {:.6f}, out {:.6f}", reference_rms, output_rms);
        if (reference_rms > 1e-9 && output_rms > 1e-9) {
            const double gain = std::clamp(reference_rms / output_rms, 0.05, 20.0);
            LOG_INFO << std::format("Feature level gain: {:.6f}", gain);
            for (double &value: output) {
                value = std::clamp(value * gain, -1.0, 1.0);
            }
        }

        if (peak_limit > 0.0) {
            double peak = 0.0;
            for (double value: output) {
                if (std::isfinite(value)) {
                    peak = std::max(peak, std::abs(value));
                }
            }
            if (peak > peak_limit) {
                const double gain = peak_limit / peak;
                for (double &value: output) {
                    value *= gain;
                }
            }
        }

        if (sample_rate != model_rate) {
            output = MelSpectrogram::Resample(output, model_rate, sample_rate);
        }
        const auto target_samples = static_cast<std::size_t>(
                std::max<std::int64_t>(1, milliseconds_to_samples(total_ms, sample_rate)));
        if (output.size() > target_samples) {
            output.resize(target_samples);
        } else if (output.size() < target_samples) {
            output.resize(target_samples, 0.0);
        }

        return {std::move(output), sample_rate};
    }

}// namespace lessampler
