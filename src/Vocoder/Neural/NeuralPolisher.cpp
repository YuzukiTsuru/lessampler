/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Vocoder/Neural/NeuralPolisher.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

#include "Core/Exception.h"
#include "Core/Log.h"
#include "Vocoder/Common/F0Estimator.h"
#include "Vocoder/Neural/MelSpectrogram.h"
#include "Vocoder/Neural/NeuralFeatures.h"
#include "Vocoder/Neural/NeuralModel.h"

namespace lessampler {

    namespace {

        constexpr float silence_threshold = -9.2f;

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

    }// namespace

    NeuralPolishResult PolishWithNeural(std::span<const double> samples, int sample_rate,
                                        const Config &configure) {
        if (samples.empty() || sample_rate <= 0) {
            throw parameter_error("The polish input is empty");
        }
        if (configure.neural_model_path.empty()) {
            throw parameter_error("--polish requires [neural] model_path in lessampler.less");
        }

        auto model = LoadNeuralModel(configure.neural_model_path, configure.neural_threads);

        std::vector<double> wave = MelSpectrogram::Resample(samples, sample_rate, MelSpectrogram::sample_rate);
        if (wave.empty()) {
            throw parameter_error("The polish input is empty after resampling");
        }

        static const MelSpectrogram extractor;
        const auto mel = extractor.Extract(wave, MelSpectrogram::sample_rate, configure.neural_target_rms);
        const auto frames = mel.size() / static_cast<std::size_t>(MelSpectrogram::mel_bins);
        if (frames == 0) {
            throw parameter_error("The polish input produced no feature frames");
        }

        if (*std::ranges::max_element(mel) < silence_threshold) {
            return {std::vector<double>(samples.size(), 0.0), sample_rate};
        }

        const auto track = EstimateF0(wave, MelSpectrogram::sample_rate, configure);
        std::vector<float> f0;
        std::vector<float> uv;
        BuildMelF0Track(track, frames, f0, uv);

        const bool any_voiced = std::ranges::any_of(uv, [](float value) { return value == 0.0f; });
        if (!any_voiced) {
            // Nothing periodic to re-render; leave the assembled mix untouched.
            return {std::vector<double>(samples.begin(), samples.end()), sample_rate};
        }

        LOG_INFO << "Neural polish: " + std::to_string(frames) + " frames";
        const auto neural = model->Synthesize(mel, frames, f0, uv);
        if (neural.empty()) {
            throw parameter_error("The neural vocoder returned an empty waveform");
        }

        std::vector<double> output(neural.begin(), neural.end());
        const std::size_t target = wave.size();
        if (output.size() > target) {
            output.resize(target);
        } else if (output.size() < target) {
            output.resize(target, 0.0);
        }

        const double input_rms = root_mean_square(wave);
        const double output_rms = root_mean_square(output);
        if (output_rms > 1e-9 && input_rms > 1e-9) {
            const double gain = std::clamp(input_rms / output_rms, 0.05, 20.0);
            for (double &value: output) {
                value *= gain;
            }
        }

        if (sample_rate != MelSpectrogram::sample_rate) {
            output = MelSpectrogram::Resample(output, MelSpectrogram::sample_rate, sample_rate);
        }

        if (output.size() > samples.size()) {
            output.resize(samples.size());
        } else if (output.size() < samples.size()) {
            output.resize(samples.size(), 0.0);
        }

        return {std::move(output), sample_rate};
    }

}// namespace lessampler
