/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Audio/Preprocess.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <vector>

#include "Audio/Dsp/Biquad.h"
#include "Audio/Dsp/Fft.h"

namespace lessampler {

    namespace {

        constexpr std::size_t kFftSize = 1024;
        constexpr std::size_t kHopSize = 256;
        constexpr std::size_t kMaxProfileFrames = 512;
        constexpr double kEpsilon = 1e-12;

        double Median(std::vector<double> &values) {
            if (values.empty()) {
                return 0.0;
            }
            const auto middle = values.begin() + static_cast<std::ptrdiff_t>(values.size() / 2);
            std::nth_element(values.begin(), middle, values.end());
            double result = *middle;
            if (values.size() % 2 == 0) {
                const auto lower = std::max_element(values.begin(), middle);
                result = 0.5 * (result + *lower);
            }
            return result;
        }

        void RepairClicks(std::vector<double> &samples, int sample_rate, double threshold) {
            if (samples.size() < 8 || sample_rate <= 0 || threshold <= 0.0) {
                return;
            }

            const std::size_t count = samples.size();
            std::vector<double> residual(count, 0.0);
            std::vector<double> magnitudes;
            magnitudes.reserve(count - 2);
            for (std::size_t i = 1; i + 1 < count; ++i) {
                residual[i] = samples[i + 1] - 2.0 * samples[i] + samples[i - 1];
                magnitudes.push_back(std::abs(residual[i]));
            }
            if (magnitudes.empty()) {
                return;
            }

            std::vector<double> work = magnitudes;
            const double median = Median(work);
            for (double &value: work) {
                value = std::abs(value - median);
            }
            double scale = 1.4826 * Median(work);
            if (!(scale > 1e-9)) {
                // A constant or silent buffer has no usable MAD. Keep a tiny
                // floor so an isolated spike is still detected.
                scale = 1e-6;
            }

            const double limit = threshold * scale;
            std::vector<bool> bad(count, false);
            for (std::size_t i = 1; i + 1 < count; ++i) {
                bad[i] = std::abs(residual[i]) > limit;
            }

            const auto max_click = static_cast<std::size_t>(
                    std::max(1.0, std::round(0.003 * static_cast<double>(sample_rate))));
            std::size_t i = 1;
            while (i + 1 < count) {
                if (!bad[i]) {
                    ++i;
                    continue;
                }
                const std::size_t start = i;
                while (i + 1 < count && bad[i + 1]) {
                    ++i;
                }
                const std::size_t end = i;
                if (start > 0 && end + 1 < count && end - start + 1 <= max_click) {
                    const double left = samples[start - 1];
                    const double right = samples[end + 1];
                    const double span = static_cast<double>(end + 1 - (start - 1));
                    for (std::size_t k = start; k <= end; ++k) {
                        const double position = static_cast<double>(k - (start - 1)) / span;
                        samples[k] = left + (right - left) * position;
                    }
                }
                ++i;
            }
        }

        void FillFrame(std::span<const double> samples,
                       std::int64_t start,
                       std::span<std::complex<double>> spectrum,
                       std::span<const double> window) {
            for (std::size_t i = 0; i < spectrum.size(); ++i) {
                const auto index = start + static_cast<std::int64_t>(i);
                if (index >= 0 && static_cast<std::size_t>(index) < samples.size()) {
                    spectrum[i] = {samples[static_cast<std::size_t>(index)] * window[i], 0.0};
                } else {
                    spectrum[i] = {0.0, 0.0};
                }
            }
        }

        void Denoise(std::vector<double> &samples, double attenuation_db) {
            if (samples.size() < 16 || attenuation_db <= 0.0) {
                return;
            }

            const Fft fft(kFftSize);
            const std::size_t bins = kFftSize / 2 + 1;
            const std::size_t frames = (samples.size() + kHopSize - 1) / kHopSize + 1;
            const std::size_t stride = std::max<std::size_t>(
                    1, (frames + kMaxProfileFrames - 1) / kMaxProfileFrames);

            std::vector<double> window(kFftSize);
            for (std::size_t i = 0; i < kFftSize; ++i) {
                window[i] = 0.5 - 0.5 * std::cos(2.0 * std::numbers::pi * static_cast<double>(i) /
                                                 static_cast<double>(kFftSize - 1));
            }

            std::vector<std::complex<double>> spectrum(kFftSize);
            std::vector<std::vector<double>> profile(bins);
            for (std::size_t frame = 0; frame < frames; frame += stride) {
                const auto start = static_cast<std::int64_t>(frame * kHopSize) - static_cast<std::int64_t>(kFftSize / 2);
                FillFrame(samples, start, spectrum, window);
                fft.Forward(spectrum);
                for (std::size_t bin = 0; bin < bins; ++bin) {
                    profile[bin].push_back(std::abs(spectrum[bin]));
                }
            }

            std::vector<double> noise(bins, 0.0);
            for (std::size_t bin = 0; bin < bins; ++bin) {
                auto &values = profile[bin];
                if (values.empty()) {
                    continue;
                }
                const std::size_t index = std::min(values.size() - 1, values.size() * 20 / 100);
                const auto middle = values.begin() + static_cast<std::ptrdiff_t>(index);
                std::nth_element(values.begin(), middle, values.end());
                noise[bin] = *middle;
            }
            for (std::size_t bin = 1; bin + 1 < bins; ++bin) {
                noise[bin] = 0.25 * noise[bin - 1] + 0.5 * noise[bin] + 0.25 * noise[bin + 1];
            }

            const double floor_gain = std::pow(10.0, -std::min(60.0, attenuation_db) / 20.0);
            std::vector<double> gains(bins, 1.0);
            std::vector<double> smoothed(bins, 1.0);
            std::vector<double> previous(bins, 1.0);
            std::vector<double> output(samples.size(), 0.0);
            std::vector<double> normalization(samples.size(), 0.0);

            for (std::size_t frame = 0; frame < frames; ++frame) {
                const auto start = static_cast<std::int64_t>(frame * kHopSize) - static_cast<std::int64_t>(kFftSize / 2);
                FillFrame(samples, start, spectrum, window);
                fft.Forward(spectrum);

                for (std::size_t bin = 0; bin < bins; ++bin) {
                    const double magnitude = std::abs(spectrum[bin]);
                    const double ratio = noise[bin] / std::max(magnitude, kEpsilon);
                    gains[bin] = std::max(floor_gain, 1.0 - ratio);
                }

                smoothed.front() = gains.front();
                smoothed.back() = gains.back();
                for (std::size_t bin = 1; bin + 1 < bins; ++bin) {
                    smoothed[bin] = 0.25 * gains[bin - 1] + 0.5 * gains[bin] + 0.25 * gains[bin + 1];
                }

                for (std::size_t bin = 0; bin < bins; ++bin) {
                    gains[bin] = 0.6 * previous[bin] + 0.4 * smoothed[bin];
                    previous[bin] = gains[bin];
                }

                for (std::size_t bin = 0; bin < kFftSize; ++bin) {
                    const std::size_t mirror = std::min(bin, kFftSize - bin);
                    spectrum[bin] *= gains[mirror];
                }
                fft.Inverse(spectrum);

                for (std::size_t i = 0; i < kFftSize; ++i) {
                    const auto index = start + static_cast<std::int64_t>(i);
                    if (index < 0 || static_cast<std::size_t>(index) >= samples.size()) {
                        continue;
                    }
                    const auto position = static_cast<std::size_t>(index);
                    output[position] += spectrum[i].real() * window[i];
                    normalization[position] += window[i] * window[i];
                }
            }

            for (std::size_t i = 0; i < samples.size(); ++i) {
                if (normalization[i] > kEpsilon) {
                    samples[i] = output[i] / normalization[i];
                } else {
                    samples[i] = 0.0;
                }
            }
        }

    }// namespace

    std::vector<double> PreprocessAudio(std::span<const double> samples,
                                        int sample_rate,
                                        const PreprocessOptions &options) {
        std::vector<double> result(samples.begin(), samples.end());
        if (!options.enabled || result.empty() || sample_rate <= 0) {
            return result;
        }

        if (options.highpass_hz > 0.0) {
            Biquad filter;
            filter.SetHighPass(sample_rate, options.highpass_hz);
            for (double &value: result) {
                value = filter.Process(value);
            }
        }
        if (options.declick) {
            RepairClicks(result, sample_rate, options.declick_threshold);
        }
        if (options.denoise) {
            Denoise(result, options.denoise_db);
        }
        return result;
    }

}// namespace lessampler
