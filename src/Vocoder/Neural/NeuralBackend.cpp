/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "NeuralBackend.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <format>
#include <memory>
#include <span>
#include <utility>
#include <vector>

#include "Core/Exception.h"
#include "Core/Log.h"
#include "Core/Timer.h"
#include "MelSpectrogram.h"
#include "NeuralModel.h"
#include "Vocoder/World/WorldModelStore.h"
#include "Vocoder/World/WorldSynthesizer.h"
#include "Vocoder/World/WorldTransform.h"

namespace lessampler {

    namespace backend {

        namespace {

            constexpr double f0_min = 40.0;
            constexpr double f0_max = 1200.0;

            class NeuralAnalysis final : public Analysis {
            public:
                NeuralAnalysis(WorldModel model, std::shared_ptr<NeuralModel> neural, double target_rms)
                    : model_(std::move(model)), neural_(std::move(neural)), target_rms_(target_rms) {}

                [[nodiscard]] VocoderKind kind() const noexcept override { return VocoderKind::neural; }

                [[nodiscard]] const WorldModel &model() const noexcept { return model_; }

                [[nodiscard]] const std::shared_ptr<NeuralModel> &neural() const noexcept { return neural_; }

                [[nodiscard]] double target_rms() const noexcept { return target_rms_; }

            private:
                WorldModel model_;
                std::shared_ptr<NeuralModel> neural_;
                double target_rms_ = 0.111;
            };

            /*
             * NHVSing wants continuous F0 with a separate unvoiced flag. WORLD
             * stores an always-positive F0 after the UTAU transform, so voicing
             * comes from the low-band aperiodicity instead. Unvoiced frames are
             * then filled by interpolation, matching NHVSing's prep_f0().
             */
            void build_f0_track(std::span<const double> source_f0, const Matrix &aperiodicity, int fs,
                                int fft_size, std::size_t frames, std::vector<float> &f0,
                                std::vector<float> &uv) {
                f0.assign(frames, 0.0f);
                uv.assign(frames, 1.0f);
                if (source_f0.empty()) {
                    return;
                }

                const auto source_at = [&](std::size_t index) {
                    return source_f0[std::min(index, source_f0.size() - 1)];
                };

                const auto low_band_bins = [&]() -> std::size_t {
                    if (aperiodicity.cols() == 0 || fs <= 0 || fft_size <= 0) {
                        return 0;
                    }
                    const auto bins = static_cast<std::size_t>(4000.0 * static_cast<double>(fft_size) /
                                                               static_cast<double>(fs));
                    return std::min<std::size_t>(aperiodicity.cols(), std::max<std::size_t>(1, bins));
                }();
                const auto ap_at = [&](std::size_t index) {
                    if (low_band_bins == 0 || aperiodicity.rows() == 0) {
                        return 0.0;
                    }
                    const auto row = std::min(index, aperiodicity.rows() - 1);
                    double sum = 0.0;
                    for (std::size_t bin = 0; bin < low_band_bins; ++bin) {
                        sum += aperiodicity[row][bin];
                    }
                    return sum / static_cast<double>(low_band_bins);
                };

                std::size_t first_voiced = frames;
                std::size_t last_voiced = 0;
                for (std::size_t i = 0; i < frames; ++i) {
                    const double value = source_at(i);
                    const bool voiced = std::isfinite(value) && value > 0.0 && ap_at(i) <= 0.5;
                    if (voiced) {
                        f0[i] = static_cast<float>(std::clamp(value, f0_min, f0_max));
                        uv[i] = 0.0f;
                        first_voiced = std::min(first_voiced, i);
                        last_voiced = i;
                    }
                }

                if (first_voiced == frames) {
                    return;
                }

                for (std::size_t i = 0; i < first_voiced; ++i) {
                    f0[i] = f0[first_voiced];
                }
                for (std::size_t i = last_voiced + 1; i < frames; ++i) {
                    f0[i] = f0[last_voiced];
                }

                std::size_t previous = first_voiced;
                for (std::size_t i = first_voiced + 1; i <= last_voiced; ++i) {
                    if (uv[i] == 0.0f) {
                        if (i > previous + 1) {
                            const auto steps = static_cast<float>(i - previous);
                            for (std::size_t j = previous + 1; j < i; ++j) {
                                const auto fraction = static_cast<float>(j - previous) / steps;
                                f0[j] = f0[previous] * (1.0f - fraction) + f0[i] * fraction;
                            }
                        }
                        previous = i;
                    }
                }
            }

            class NeuralBackend final : public VocoderBackend {
            public:
                [[nodiscard]] VocoderKind kind() const noexcept override { return VocoderKind::neural; }

                [[nodiscard]] std::string_view name() const noexcept override { return "neural"; }

                [[nodiscard]] std::uint32_t analysis_version() const noexcept override { return 1; }

                [[nodiscard]] AnalysisPtr Prepare(const AnalysisRequest &request) override {
                    if (request.configure.neural_model_path.empty()) {
                        throw parameter_error("The neural backend requires [neural] model_path");
                    }

                    auto neural = LoadNeuralModel(request.configure.neural_model_path,
                                                  request.configure.neural_threads);

                    Timer timer;
                    timer.SetTimer();
                    auto model = LoadOrBuildWorldModel(request.source, request.configure, request.max_workers);
                    LOG_EVAL << timer.GetTimer("Neural Model Prepare: ");

                    LOG_INFO << std::format("Audio Sample Rate: {}", model.fs);
                    LOG_INFO << std::format("Model Frame Period: {}", model.frame_period);
                    LOG_INFO << std::format("Model FFT Size: {}", model.fft_size);

                    return std::make_shared<NeuralAnalysis>(std::move(model), std::move(neural),
                                                            request.configure.neural_target_rms);
                }

                [[nodiscard]] RenderResult Synthesize(const SynthesisRequest &request) override {
                    const auto &analysis = static_cast<const NeuralAnalysis &>(request.analysis);
                    if (request.params.output_samples <= 0) {
                        throw parameter_error("The output sample count must be greater than zero");
                    }

                    Timer timer;
                    timer.SetTimer();
                    WorldTransform transform(analysis.model(), request.params);
                    const auto &transformed = transform.GetTransAudioModel();
                    LOG_EVAL << timer.GetTimer("Neural WORLD Transform: ");

                    // WORLD provides the time/pitch-transformed spectral
                    // envelope. Rendering it also gives the neural front end a
                    // waveform with the exact feature statistics it expects.
                    timer.SetTimer();
                    const auto world_audio = SynthesizeWorld(transformed, request.params.output_samples);
                    LOG_EVAL << timer.GetTimer("Neural WORLD Synthesis: ");

                    timer.SetTimer();
                    static const MelSpectrogram extractor;
                    const auto mel = extractor.Extract(world_audio, transformed.fs, analysis.target_rms());
                    const auto frames = mel.size() / static_cast<std::size_t>(MelSpectrogram::mel_bins);
                    LOG_EVAL << timer.GetTimer("Neural Mel Extraction: ");

                    if (frames == 0) {
                        throw parameter_error("Neural vocoder produced no feature frames");
                    }

                    // NHVSing's preprocessing treats a segment whose loudest
                    // mel bin stays below -9.2 as silence. Return exact zeros
                    // instead of letting AutoAMP amplify model noise.
                    if (*std::ranges::max_element(mel) < -9.2f) {
                        return {std::vector<double>(static_cast<std::size_t>(request.params.output_samples), 0.0),
                                transformed.fs};
                    }

                    std::vector<float> f0;
                    std::vector<float> uv;
                    build_f0_track(transformed.f0, transformed.aperiodicity, transformed.fs,
                                   transformed.fft_size, frames, f0, uv);

                    timer.SetTimer();
                    const auto neural_audio = analysis.neural()->Synthesize(mel, frames, f0, uv);
                    LOG_EVAL << timer.GetTimer("Neural Synthesis: ");

                    if (neural_audio.empty()) {
                        throw parameter_error("Neural vocoder returned an empty waveform");
                    }

                    timer.SetTimer();
                    std::vector<double> samples;
                    if (transformed.fs == NeuralModel::model_sample_rate) {
                        samples.assign(neural_audio.begin(), neural_audio.end());
                    } else {
                        const std::vector<double> input(neural_audio.begin(), neural_audio.end());
                        samples = MelSpectrogram::Resample(input, NeuralModel::model_sample_rate, transformed.fs);
                    }

                    const auto target_samples = static_cast<std::size_t>(request.params.output_samples);
                    if (samples.size() > target_samples) {
                        samples.resize(target_samples);
                    } else if (samples.size() < target_samples) {
                        samples.resize(target_samples, 0.0);
                    }
                    LOG_EVAL << timer.GetTimer("Neural Resample: ");

                    return {std::move(samples), transformed.fs};
                }
            };

        }// namespace

        std::unique_ptr<VocoderBackend> CreateNeuralBackend() {
            return std::make_unique<NeuralBackend>();
        }

    }// namespace backend

}// namespace lessampler
