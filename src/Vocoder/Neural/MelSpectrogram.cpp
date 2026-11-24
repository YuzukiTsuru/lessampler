/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "MelSpectrogram.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <vector>

#include <world/fft.h>

#include "Core/Exception.h"

namespace lessampler {

    namespace {

        constexpr std::size_t bin_count = static_cast<std::size_t>(MelSpectrogram::fft_size) / 2 + 1;

        double hz_to_mel(double frequency) {
            constexpr double f_min = 0.0;
            constexpr double f_sp = 200.0 / 3.0;
            constexpr double min_log_hz = 1000.0;
            constexpr double min_log_mel = (min_log_hz - f_min) / f_sp;
            constexpr double logstep = 1.8562979903656263 / 27.0;// log(6.4)

            if (frequency < min_log_hz) {
                return (frequency - f_min) / f_sp;
            }
            return min_log_mel + std::log(frequency / min_log_hz) / logstep;
        }

        double mel_to_hz(double mel) {
            constexpr double f_min = 0.0;
            constexpr double f_sp = 200.0 / 3.0;
            constexpr double min_log_hz = 1000.0;
            constexpr double min_log_mel = (min_log_hz - f_min) / f_sp;
            constexpr double logstep = 1.8562979903656263 / 27.0;// log(6.4)

            if (mel < min_log_mel) {
                return f_min + f_sp * mel;
            }
            return min_log_hz * std::exp(logstep * (mel - min_log_mel));
        }

        std::vector<double> reflect_pad(std::span<const double> samples, std::size_t pad) {
            std::vector<double> padded(samples.size() + 2 * pad, 0.0);
            const auto length = samples.size();
            if (length == 0) {
                return padded;
            }
            if (length == 1) {
                std::fill(padded.begin(), padded.end(), samples.front());
                return padded;
            }

            std::ranges::copy(samples, padded.begin() + static_cast<std::ptrdiff_t>(pad));
            for (std::size_t i = 0; i < pad; ++i) {
                // numpy's reflect mode does not repeat the edge sample.
                const std::size_t source = std::min(length - 1, i + 1);
                padded[pad - 1 - i] = samples[source];
                const std::size_t tail = std::min(length - 1, i + 1);
                padded[pad + length + i] = samples[length - 1 - tail];
            }
            return padded;
        }

        double active_rms(const std::vector<double> &samples) {
            constexpr double threshold = 0.01;// -40 dB
            std::size_t active = 0;
            double sum = 0.0;
            for (double value: samples) {
                if (std::abs(value) > threshold) {
                    sum += value * value;
                    ++active;
                }
            }

            const auto minimum_active = std::max<std::size_t>(1, samples.size() / 100);
            if (active >= minimum_active) {
                return std::sqrt(sum / static_cast<double>(active));
            }

            double total = 0.0;
            for (double value: samples) {
                total += value * value;
            }
            return samples.empty() ? 0.0 : std::sqrt(total / static_cast<double>(samples.size()));
        }

    }// namespace

    MelSpectrogram::MelSpectrogram() {
        build_filterbank();
    }

    void MelSpectrogram::build_filterbank() {
        filterbank_.assign(static_cast<std::size_t>(mel_bins) * bin_count, 0.0);

        const auto mel_points = [](double start, double stop, int count) {
            std::vector<double> points(static_cast<std::size_t>(count));
            for (int i = 0; i < count; ++i) {
                const double fraction = count == 1 ? 0.0 : static_cast<double>(i) / static_cast<double>(count - 1);
                points[static_cast<std::size_t>(i)] = mel_to_hz(start + (stop - start) * fraction);
            }
            return points;
        };

        const auto mel_frequencies = mel_points(hz_to_mel(mel_min), hz_to_mel(mel_max), mel_bins + 2);
        std::vector<double> fft_frequencies(bin_count);
        for (std::size_t i = 0; i < bin_count; ++i) {
            fft_frequencies[i] = static_cast<double>(i) * sample_rate / fft_size;
        }

        for (int m = 0; m < mel_bins; ++m) {
            const double lower_hz = mel_frequencies[static_cast<std::size_t>(m)];
            const double center_hz = mel_frequencies[static_cast<std::size_t>(m) + 1];
            const double upper_hz = mel_frequencies[static_cast<std::size_t>(m) + 2];
            const double lower_width = center_hz - lower_hz;
            const double upper_width = upper_hz - center_hz;
            const double normalization = 2.0 / (upper_hz - lower_hz);

            for (std::size_t k = 0; k < bin_count; ++k) {
                const double frequency = fft_frequencies[k];
                const double rising = (frequency - lower_hz) / lower_width;
                const double falling = (upper_hz - frequency) / upper_width;
                const double weight = std::max(0.0, std::min(rising, falling));
                filterbank_[static_cast<std::size_t>(m) * bin_count + k] = weight * normalization;
            }
        }
    }

    std::vector<double> MelSpectrogram::Resample(std::span<const double> samples, int from_rate, int to_rate) {
        if (from_rate <= 0 || to_rate <= 0) {
            throw parameter_error("Invalid sample rate for resampling");
        }
        if (samples.empty() || from_rate == to_rate) {
            return std::vector<double>(samples.begin(), samples.end());
        }

        const double ratio = static_cast<double>(to_rate) / static_cast<double>(from_rate);
        const auto output_length = static_cast<std::size_t>(std::llround(static_cast<double>(samples.size()) * ratio));
        std::vector<double> output(output_length, 0.0);

        constexpr int half_taps = 16;
        const double cutoff = std::min(1.0, ratio) * 0.97;

        for (std::size_t n = 0; n < output_length; ++n) {
            const double position = static_cast<double>(n) / ratio;
            const auto center = static_cast<std::ptrdiff_t>(std::floor(position));
            double sum = 0.0;
            double weight_sum = 0.0;
            for (int tap = -half_taps + 1; tap <= half_taps; ++tap) {
                const auto index = center + tap;
                if (index < 0 || static_cast<std::size_t>(index) >= samples.size()) {
                    continue;
                }
                const double distance = position - static_cast<double>(index);
                if (std::abs(distance) >= half_taps) {
                    continue;
                }
                const double window = 0.5 + 0.5 * std::cos(std::numbers::pi * distance / half_taps);
                const double sinc = std::abs(distance) < 1e-12
                                            ? 1.0
                                            : std::sin(std::numbers::pi * cutoff * distance) /
                                                      (std::numbers::pi * cutoff * distance);
                const double weight = cutoff * sinc * window;
                sum += samples[static_cast<std::size_t>(index)] * weight;
                weight_sum += weight;
            }
            output[n] = weight_sum == 0.0 ? 0.0 : sum / weight_sum;
        }
        return output;
    }

    std::vector<float> MelSpectrogram::Extract(std::span<const double> samples, int source_rate,
                                               double target_rms) const {
        if (source_rate <= 0) {
            throw parameter_error("Invalid source sample rate for neural features");
        }

        std::vector<double> audio = Resample(samples, source_rate, sample_rate);
        const double rms = active_rms(audio);
        if (rms > 1e-6 && target_rms > 0.0) {
            const double scale = target_rms / rms;
            for (double &value: audio) {
                value = std::clamp(value * scale, -1.0, 1.0);
            }
        }

        constexpr std::size_t pad = (fft_size - hop_size) / 2;
        auto padded = reflect_pad(audio, pad);
        if (padded.size() < static_cast<std::size_t>(fft_size)) {
            padded.resize(static_cast<std::size_t>(fft_size), 0.0);
        }

        const std::size_t frames = 1 + (padded.size() - static_cast<std::size_t>(fft_size)) /
                                               static_cast<std::size_t>(hop_size);
        std::vector<float> features(frames * static_cast<std::size_t>(mel_bins), 0.0f);

        std::vector<double> window(static_cast<std::size_t>(fft_size));
        for (int i = 0; i < fft_size; ++i) {
            window[static_cast<std::size_t>(i)] =
                    0.5 - 0.5 * std::cos(2.0 * std::numbers::pi * static_cast<double>(i) / fft_size);
        }

        std::vector<double> buffer(static_cast<std::size_t>(fft_size));
        std::vector<fft_complex> spectrum(static_cast<std::size_t>(fft_size));
        fft_plan plan = fft_plan_dft_r2c_1d(fft_size, buffer.data(), spectrum.data(), FFT_ESTIMATE);

        std::vector<double> magnitudes(bin_count);
        for (std::size_t frame = 0; frame < frames; ++frame) {
            const auto offset = frame * static_cast<std::size_t>(hop_size);
            for (std::size_t i = 0; i < static_cast<std::size_t>(fft_size); ++i) {
                buffer[i] = padded[offset + i] * window[i];
            }

            fft_execute(plan);
            for (std::size_t bin = 0; bin < bin_count; ++bin) {
                magnitudes[bin] = std::hypot(spectrum[bin][0], spectrum[bin][1]);
            }

            for (int m = 0; m < mel_bins; ++m) {
                double value = 0.0;
                const auto row = static_cast<std::size_t>(m) * bin_count;
                for (std::size_t bin = 0; bin < bin_count; ++bin) {
                    value += filterbank_[row + bin] * magnitudes[bin];
                }
                features[frame * static_cast<std::size_t>(mel_bins) + static_cast<std::size_t>(m)] =
                        static_cast<float>(std::log(std::max(1e-5, value)));
            }
        }

        fft_destroy_plan(plan);
        return features;
    }

}// namespace lessampler
