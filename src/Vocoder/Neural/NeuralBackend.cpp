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

#include "Audio/Preprocess.h"
#include "Core/Exception.h"
#include "Core/Log.h"
#include "Core/Timer.h"
#include "MelSpectrogram.h"
#include "NeuralModel.h"
#include "Vocoder/Common/F0Estimator.h"
#include "Vocoder/World/WorldModelStore.h"
#include "Vocoder/World/WorldSynthesizer.h"
#include "Vocoder/World/WorldTransform.h"

namespace lessampler {

    namespace backend {

        namespace {

            class NeuralAnalysis final : public Analysis {
            public:
                NeuralAnalysis(WorldModel model, std::shared_ptr<NeuralModel> neural, double target_rms,
                               NeuralVocoderSpec spec, NeuralFrontendKind frontend,
                               int source_hop, std::vector<float> source_mel,
                               std::size_t source_mel_frames, std::vector<double> source_samples)
                    : model_(std::move(model)), neural_(std::move(neural)), target_rms_(target_rms),
                      spec_(std::move(spec)), frontend_(frontend), source_hop_(source_hop),
                      source_mel_(std::move(source_mel)), source_mel_frames_(source_mel_frames),
                      source_samples_(std::move(source_samples)) {}

                [[nodiscard]] VocoderKind kind() const noexcept override { return VocoderKind::neural; }

                [[nodiscard]] const WorldModel &model() const noexcept { return model_; }

                [[nodiscard]] const std::shared_ptr<NeuralModel> &neural() const noexcept { return neural_; }

                [[nodiscard]] double target_rms() const noexcept { return target_rms_; }

                [[nodiscard]] const NeuralVocoderSpec &spec() const noexcept { return spec_; }

                [[nodiscard]] NeuralFrontendKind frontend() const noexcept { return frontend_; }

                [[nodiscard]] std::span<const float> source_mel() const noexcept { return source_mel_; }

                [[nodiscard]] std::size_t source_mel_frames() const noexcept { return source_mel_frames_; }

                [[nodiscard]] int source_hop() const noexcept { return source_hop_; }

                [[nodiscard]] std::span<const double> source_samples() const noexcept { return source_samples_; }

            private:
                WorldModel model_;
                std::shared_ptr<NeuralModel> neural_;
                double target_rms_ = 0.111;
                NeuralVocoderSpec spec_;
                NeuralFrontendKind frontend_ = NeuralFrontendKind::world;
                int source_hop_ = 128;
                std::vector<float> source_mel_;
                std::size_t source_mel_frames_ = 0;
                std::vector<double> source_samples_;
            };

            /*
             * NHVSing wants continuous F0 with a separate unvoiced flag. WORLD
             * stores an always-positive F0 after the UTAU transform, so voicing
             * comes from the low-band aperiodicity instead. Unvoiced frames are
             * then filled by interpolation, matching NHVSing's prep_f0().
             */
            void build_f0_track(std::span<const double> source_f0, const Matrix &aperiodicity, int fs,
                                int fft_size, double frame_period_ms, const NeuralVocoderSpec &spec,
                                double f0_min, double f0_max, std::size_t frames,
                                std::vector<float> &f0, std::vector<float> &uv) {
                f0.assign(frames, 0.0f);
                uv.assign(frames, 1.0f);
                if (source_f0.empty()) {
                    return;
                }

                const auto source_index_at = [&](std::size_t mel_frame) {
                    if (!(frame_period_ms > 0.0)) {
                        return std::min(mel_frame, source_f0.size() - 1);
                    }
                    // The mel hop and the WORLD analysis frame period are not
                    // the same for hop-512 vocoders. Sample the source track by
                    // time, not by the mel frame index.
                    const double time_ms = static_cast<double>(mel_frame) * static_cast<double>(spec.hop_size) *
                                           1000.0 / static_cast<double>(spec.sample_rate);
                    const auto index = static_cast<std::size_t>(std::floor(time_ms / frame_period_ms));
                    return std::min(index, source_f0.size() - 1);
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
                    const auto source_index = source_index_at(i);
                    const double value = source_f0[source_index];
                    const bool voiced = std::isfinite(value) && value > 0.0 && ap_at(source_index) <= 0.5;
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

            double source_time_ms(const RenderParams &params, double output_ms) {
                if (output_ms < params.base_length) {
                    return params.offset + output_ms * params.velocity;
                }
                return params.offset + params.first_half_fixed_part +
                       (output_ms - params.base_length) * params.stretch_length;
            }

            double pitch_bend_at(const RenderParams &params, double output_ms, int source_rate) {
                if (params.pitch_step <= 0 || params.pitch_bend.size() < 2) {
                    return 0.0;
                }
                const double position = output_ms * 0.001 * static_cast<double>(source_rate) /
                                        static_cast<double>(params.pitch_step);
                const auto max_index = params.pitch_bend.size() - 2;
                std::size_t index = 0;
                double fraction = 0.0;
                if (position > 0.0) {
                    index = static_cast<std::size_t>(std::floor(position));
                    if (index > max_index) {
                        index = max_index;
                    }
                    fraction = std::min(1.0, position - static_cast<double>(index));
                }
                return params.pitch_bend[index] * (1.0 - fraction) +
                       params.pitch_bend[index + 1] * fraction;
            }

            double average_source_f0(std::span<const double> f0) {
                double sum = 0.0;
                std::size_t count = 0;
                for (double value: f0) {
                    if (value >= 55.0 && value <= 1200.0) {
                        sum += value;
                        ++count;
                    }
                }
                return count > 0 ? sum / static_cast<double>(count) : 0.0;
            }

            double low_band_aperiodicity(const Matrix &aperiodicity, std::size_t frame, int fs, int fft_size) {
                if (aperiodicity.rows() == 0 || aperiodicity.cols() == 0 || fs <= 0 || fft_size <= 0) {
                    return 0.0;
                }
                const auto bins = std::min<std::size_t>(
                        aperiodicity.cols(),
                        std::max<std::size_t>(1, static_cast<std::size_t>(
                                                         4000.0 * static_cast<double>(fft_size) / static_cast<double>(fs))));
                const auto row = std::min(frame, aperiodicity.rows() - 1);
                double sum = 0.0;
                for (std::size_t bin = 0; bin < bins; ++bin) {
                    sum += aperiodicity[row][bin];
                }
                return sum / static_cast<double>(bins);
            }

            /*
             * Source-mel frontend: keep the voicebank sample's spectral detail
             * and map it through the UTAU time/pitch controls instead of
             * rendering a preliminary WORLD waveform. This is the same
             * source-constrained idea used by hifisampler: the sample remains
             * the articulation source and the vocoder only re-synthesizes it
             * under the requested F0.
             */
            void build_source_features(std::span<const float> source_mel,
                                       std::size_t source_frames,
                                       const WorldModel &model,
                                       const NeuralVocoderSpec &spec,
                                       double source_hop_ms,
                                       const RenderParams &params,
                                       std::size_t frames,
                                       std::vector<float> &mel,
                                       std::vector<float> &f0,
                                       std::vector<float> &uv) {
                const auto bins = static_cast<std::size_t>(spec.mel_bins);
                mel.assign(frames * bins, 0.0f);
                f0.assign(frames, 0.0f);
                uv.assign(frames, 1.0f);
                if (source_mel.empty() || source_frames == 0) {
                    return;
                }

                const double hop_ms = static_cast<double>(spec.hop_size) * 1000.0 /
                                      static_cast<double>(spec.sample_rate);
                const double source_avg_f0 = average_source_f0(model.f0);
                const double modulation_reference = source_avg_f0 > 0.0 ? source_avg_f0 : params.scale_num;
                const auto source_mel_count = static_cast<double>(source_frames);
                const auto source_f0_count = static_cast<double>(model.f0.size());
                const double frame_period = model.frame_period > 0.0 ? model.frame_period : 5.0;

                const auto sample_mel = [&](double source_ms, std::size_t bin) {
                    const double position = source_ms / source_hop_ms - 0.5;
                    const double clamped = std::clamp(position, 0.0, source_mel_count - 1.0);
                    const auto left = static_cast<std::size_t>(std::floor(clamped));
                    const auto right = std::min(left + 1, source_frames - 1);
                    const double fraction = clamped - static_cast<double>(left);
                    return source_mel[left * bins + bin] * static_cast<float>(1.0 - fraction) +
                           source_mel[right * bins + bin] * static_cast<float>(fraction);
                };

                for (std::size_t frame = 0; frame < frames; ++frame) {
                    const double output_ms = (static_cast<double>(frame) + 0.5) * hop_ms;
                    const double input_ms = std::max(0.0, source_time_ms(params, output_ms));

                    for (std::size_t bin = 0; bin < bins; ++bin) {
                        mel[frame * bins + bin] = sample_mel(input_ms, bin);
                    }

                    std::size_t source_index = 0;
                    double source_fraction = 0.0;
                    double source_f0 = 0.0;
                    if (source_f0_count > 0.0) {
                        const double position = std::max(0.0, input_ms / frame_period);
                        const double clamped = std::min(position, source_f0_count - 1.0);
                        source_index = static_cast<std::size_t>(std::floor(clamped));
                        source_fraction = clamped - static_cast<double>(source_index);
                        const auto source_next = std::min(source_index + 1, model.f0.size() - 1);
                        source_f0 = model.f0[source_index];
                        if (source_fraction > 0.0 && source_index != source_next) {
                            const double next = model.f0[source_next];
                            if (source_f0 > 0.0 && next > 0.0) {
                                source_f0 = source_f0 * (1.0 - source_fraction) + next * source_fraction;
                            } else if (source_f0 <= 0.0) {
                                source_f0 = next;
                            }
                        }
                    }

                    const double bend = pitch_bend_at(params, output_ms, model.fs);
                    double target = params.scale_num * std::pow(2.0, bend / 1200.0);
                    if (params.modulation != 0 && modulation_reference > 0.0 && source_f0 > 0.0) {
                        target *= std::pow(source_f0 / modulation_reference,
                                           static_cast<double>(params.modulation) * 0.01);
                    }
                    f0[frame] = static_cast<float>(std::clamp(target, spec.f0_min, spec.f0_max));

                    const bool voiced = source_f0 > 0.0 &&
                                        low_band_aperiodicity(model.aperiodicity, source_index,
                                                              model.fs, model.fft_size) <= 0.5;
                    uv[frame] = voiced ? 0.0f : 1.0f;
                }
            }

            /*
             * WORLD-spectral frontend: use the already-transformed WORLD
             * spectrogram directly as the mel source. It avoids the preliminary
             * WORLD synthesis, so phase resets from that render cannot leak into
             * the neural features.
             */
            void build_world_spectral_features(const WorldModel &transformed,
                                               const NeuralVocoderSpec &spec,
                                               std::size_t frames,
                                               std::vector<float> &mel,
                                               std::vector<float> &f0,
                                               std::vector<float> &uv) {
                const auto bins = static_cast<std::size_t>(spec.mel_bins);
                mel.assign(frames * bins, 0.0f);
                f0.assign(frames, 0.0f);
                uv.assign(frames, 1.0f);
                const auto source_frames = transformed.spectrogram.rows();
                if (source_frames == 0 || transformed.w_length <= 0) {
                    return;
                }

                const MelSpectrogram extractor(spec);
                const auto source_mel = extractor.ExtractPower(
                        transformed.spectrogram.values(), source_frames,
                        static_cast<std::size_t>(transformed.w_length));
                const double hop_ms = static_cast<double>(spec.hop_size) * 1000.0 /
                                      static_cast<double>(spec.sample_rate);
                const double source_hop_ms = transformed.frame_period > 0.0 ? transformed.frame_period : 5.0;
                const auto source_count = static_cast<double>(source_frames);
                const auto f0_count = transformed.f0.size();
                const double source_f0_denom = f0_count > 0 ? static_cast<double>(f0_count) : 1.0;

                for (std::size_t frame = 0; frame < frames; ++frame) {
                    const double output_ms = (static_cast<double>(frame) + 0.5) * hop_ms;
                    const double position = std::clamp(output_ms / source_hop_ms, 0.0, source_count - 1.0);
                    const auto left = static_cast<std::size_t>(std::floor(position));
                    const auto right = std::min(left + 1, source_frames - 1);
                    const double fraction = position - static_cast<double>(left);
                    for (std::size_t bin = 0; bin < bins; ++bin) {
                        mel[frame * bins + bin] = source_mel[left * bins + bin] * static_cast<float>(1.0 - fraction) +
                                                  source_mel[right * bins + bin] * static_cast<float>(fraction);
                    }

                    std::size_t f0_left = 0;
                    std::size_t f0_right = 0;
                    double f0_fraction = 0.0;
                    if (f0_count > 0) {
                        const double f0_position = std::clamp(
                                output_ms / std::max(source_hop_ms, 1e-6), 0.0, source_f0_denom - 1.0);
                        f0_left = static_cast<std::size_t>(std::floor(f0_position));
                        f0_right = std::min(f0_left + 1, f0_count - 1);
                        f0_fraction = f0_position - static_cast<double>(f0_left);
                    }
                    const double value = f0_count > 0
                                                 ? transformed.f0[f0_left] * (1.0 - f0_fraction) +
                                                           transformed.f0[f0_right] * f0_fraction
                                                 : 0.0;
                    f0[frame] = static_cast<float>(std::clamp(value, spec.f0_min, spec.f0_max));
                    uv[frame] = value > 0.0 &&
                                                low_band_aperiodicity(transformed.aperiodicity, f0_left,
                                                                      transformed.fs, transformed.fft_size) <= 0.5
                                        ? 0.0f
                                        : 1.0f;
                }
            }

            void build_target_f0_uv(const WorldModel &model,
                                    const NeuralVocoderSpec &spec,
                                    const RenderParams &params,
                                    std::size_t frames,
                                    std::vector<float> &f0,
                                    std::vector<float> &uv) {
                f0.assign(frames, 0.0f);
                uv.assign(frames, 1.0f);
                const double hop_ms = static_cast<double>(spec.hop_size) * 1000.0 /
                                      static_cast<double>(spec.sample_rate);
                const double source_avg_f0 = average_source_f0(model.f0);
                const double modulation_reference = source_avg_f0 > 0.0 ? source_avg_f0 : params.scale_num;
                const double frame_period = model.frame_period > 0.0 ? model.frame_period : 5.0;
                const auto f0_count = static_cast<double>(model.f0.size());

                for (std::size_t frame = 0; frame < frames; ++frame) {
                    const double output_ms = (static_cast<double>(frame) + 0.5) * hop_ms;
                    const double input_ms = std::max(0.0, source_time_ms(params, output_ms));
                    std::size_t source_index = 0;
                    double source_f0 = 0.0;
                    if (f0_count > 0.0) {
                        const double clamped = std::min(std::max(0.0, input_ms / frame_period), f0_count - 1.0);
                        source_index = static_cast<std::size_t>(std::floor(clamped));
                        source_f0 = model.f0[source_index];
                    }

                    const double bend = pitch_bend_at(params, output_ms, model.fs);
                    double target = params.scale_num * std::pow(2.0, bend / 1200.0);
                    if (params.modulation != 0 && modulation_reference > 0.0 && source_f0 > 0.0) {
                        target *= std::pow(source_f0 / modulation_reference,
                                           static_cast<double>(params.modulation) * 0.01);
                    }
                    f0[frame] = static_cast<float>(std::clamp(target, spec.f0_min, spec.f0_max));
                    uv[frame] = source_f0 > 0.0 &&
                                                low_band_aperiodicity(model.aperiodicity, source_index,
                                                                      model.fs, model.fft_size) <= 0.5
                                        ? 0.0f
                                        : 1.0f;
                }
            }

            class NeuralBackend final : public VocoderBackend {
            public:
                [[nodiscard]] VocoderKind kind() const noexcept override { return VocoderKind::neural; }

                [[nodiscard]] std::string_view name() const noexcept override { return "neural"; }

                [[nodiscard]] std::uint32_t analysis_version() const noexcept override { return 3; }

                [[nodiscard]] AnalysisPtr Prepare(const AnalysisRequest &request) override {
                    const auto configure = ResolveF0EstimatorConfig(request.configure, kind());
                    if (configure.neural_model_path.empty()) {
                        throw parameter_error("The neural backend requires [neural] model_path");
                    }

                    auto neural = LoadNeuralModel(configure.neural_model_path,
                                                  configure.neural_threads,
                                                  configure.neural_spec);

                    Timer timer;
                    timer.SetTimer();
                    auto model = LoadOrBuildWorldModel(request.source, configure, request.max_workers);
                    LOG_EVAL << timer.GetTimer("Neural Model Prepare: ");

                    LOG_INFO << std::format("Audio Sample Rate: {}", model.fs);
                    LOG_INFO << std::format("Model Frame Period: {}", model.frame_period);
                    LOG_INFO << std::format("Model FFT Size: {}", model.fft_size);

                    std::vector<float> source_mel;
                    std::size_t source_mel_frames = 0;
                    std::vector<double> source_samples;
                    if (configure.neural_frontend == NeuralFrontendKind::source_wave) {
                        const auto raw = request.source.samples();
                        if (raw.empty()) {
                            throw parameter_error("Empty audio input");
                        }
                        source_samples.assign(raw.begin(), raw.end());
                        if (configure.preprocess.enabled) {
                            source_samples = PreprocessAudio(source_samples, model.fs, configure.preprocess);
                        }
                    }
                    if (configure.neural_frontend == NeuralFrontendKind::source_mel) {
                        timer.SetTimer();
                        const auto raw = request.source.samples();
                        if (raw.empty()) {
                            throw parameter_error("Empty audio input");
                        }
                        std::vector<double> source_audio(raw.begin(), raw.end());
                        if (configure.preprocess.enabled) {
                            source_audio = PreprocessAudio(source_audio, model.fs, configure.preprocess);
                        }
                        const auto peak = std::ranges::max_element(source_audio, {}, [](double value) {
                            return std::abs(value);
                        });
                        if (peak != source_audio.end() && std::abs(*peak) > 0.5) {
                            const double scale = 0.5 / std::abs(*peak);
                            for (double &value: source_audio) {
                                value *= scale;
                            }
                        }
                        auto source_spec = configure.neural_spec;
                        source_spec.hop_size = configure.neural_source_hop;
                        const MelSpectrogram extractor(source_spec);
                        source_mel = extractor.Extract(source_audio, model.fs, configure.neural_target_rms);
                        source_mel_frames = source_mel.size() / static_cast<std::size_t>(configure.neural_spec.mel_bins);
                        LOG_EVAL << timer.GetTimer("Neural Source Mel: ");
                    }

                    return std::make_shared<NeuralAnalysis>(std::move(model), std::move(neural),
                                                            configure.neural_target_rms,
                                                            configure.neural_spec,
                                                            configure.neural_frontend,
                                                            configure.neural_source_hop,
                                                            std::move(source_mel), source_mel_frames,
                                                            std::move(source_samples));
                }

                [[nodiscard]] RenderResult Synthesize(const SynthesisRequest &request) override {
                    const auto &analysis = static_cast<const NeuralAnalysis &>(request.analysis);
                    const auto &spec = analysis.spec();
                    if (request.params.output_samples <= 0) {
                        throw parameter_error("The output sample count must be greater than zero");
                    }

                    const int source_rate = analysis.model().fs;
                    Timer timer;
                    std::vector<float> mel;
                    std::vector<float> f0;
                    std::vector<float> uv;
                    std::size_t frames = 0;

                    if (analysis.frontend() == NeuralFrontendKind::source_wave) {
                        timer.SetTimer();
                        if (analysis.source_samples().empty()) {
                            throw parameter_error("Source-wave frontend has no source samples");
                        }
                        std::vector<double> source_wave(static_cast<std::size_t>(request.params.output_samples), 0.0);
                        for (std::size_t i = 0; i < source_wave.size(); ++i) {
                            const double output_ms = static_cast<double>(i) * 1000.0 /
                                                     static_cast<double>(source_rate);
                            const double input_ms = std::max(0.0, source_time_ms(request.params, output_ms));
                            const double position = input_ms * 0.001 * static_cast<double>(source_rate);
                            const double clamped = std::clamp(
                                    position, 0.0, static_cast<double>(analysis.source_samples().size() - 1));
                            const auto left = static_cast<std::size_t>(std::floor(clamped));
                            const auto right = std::min(left + 1, analysis.source_samples().size() - 1);
                            const double fraction = clamped - static_cast<double>(left);
                            source_wave[i] = analysis.source_samples()[left] * (1.0 - fraction) +
                                             analysis.source_samples()[right] * fraction;
                        }
                        const MelSpectrogram extractor(spec);
                        mel = extractor.Extract(source_wave, source_rate, analysis.target_rms());
                        frames = mel.size() / static_cast<std::size_t>(spec.mel_bins);
                        if (frames == 0) {
                            throw parameter_error("Source-wave frontend produced no feature frames");
                        }
                        build_target_f0_uv(analysis.model(), spec, request.params, frames, f0, uv);
                        LOG_EVAL << timer.GetTimer("Neural Source-Wave Transform: ");
                    } else if (analysis.frontend() == NeuralFrontendKind::source_mel) {
                        timer.SetTimer();
                        if (analysis.source_mel().empty() || analysis.source_mel_frames() == 0) {
                            throw parameter_error("Source-mel frontend has no source features");
                        }
                        const double hop_ms = static_cast<double>(spec.hop_size) * 1000.0 /
                                              static_cast<double>(spec.sample_rate);
                        const double duration_ms = static_cast<double>(request.params.output_samples) * 1000.0 /
                                                   static_cast<double>(source_rate);
                        frames = static_cast<std::size_t>(std::ceil(duration_ms / hop_ms)) + 1;
                        build_source_features(analysis.source_mel(), analysis.source_mel_frames(),
                                              analysis.model(), spec, analysis.source_hop() * 1000.0 / static_cast<double>(spec.sample_rate),
                                              request.params, frames, mel, f0, uv);
                        LOG_EVAL << timer.GetTimer("Neural Source Transform: ");
                    } else if (analysis.frontend() == NeuralFrontendKind::world_spectral) {
                        timer.SetTimer();
                        WorldTransform transform(analysis.model(), request.params);
                        const auto &transformed = transform.GetTransAudioModel();
                        LOG_EVAL << timer.GetTimer("Neural WORLD Transform: ");

                        const double hop_ms = static_cast<double>(spec.hop_size) * 1000.0 /
                                              static_cast<double>(spec.sample_rate);
                        const double duration_ms = static_cast<double>(request.params.output_samples) * 1000.0 /
                                                   static_cast<double>(source_rate);
                        frames = static_cast<std::size_t>(std::ceil(duration_ms / hop_ms)) + 1;
                        build_world_spectral_features(transformed, spec, frames, mel, f0, uv);
                        LOG_EVAL << timer.GetTimer("Neural WORLD-Spectral Transform: ");
                    } else {
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
                        const MelSpectrogram extractor(spec);
                        mel = extractor.Extract(world_audio, transformed.fs, analysis.target_rms());
                        frames = mel.size() / static_cast<std::size_t>(spec.mel_bins);
                        LOG_EVAL << timer.GetTimer("Neural Mel Extraction: ");

                        if (frames == 0) {
                            throw parameter_error("Neural vocoder produced no feature frames");
                        }

                        build_f0_track(transformed.f0, transformed.aperiodicity, transformed.fs,
                                       transformed.fft_size, transformed.frame_period, spec,
                                       spec.f0_min, spec.f0_max, frames, f0, uv);
                    }

                    if (frames == 0 || mel.empty()) {
                        throw parameter_error("Neural vocoder produced no feature frames");
                    }

                    // NHVSing's preprocessing treats a segment whose loudest
                    // mel bin stays below -9.2 as silence. Return exact zeros
                    // instead of letting AutoAMP amplify model noise.
                    if (*std::ranges::max_element(mel) < -9.2f) {
                        return {std::vector<double>(static_cast<std::size_t>(request.params.output_samples), 0.0),
                                source_rate};
                    }

                    timer.SetTimer();
                    const auto neural_audio = analysis.neural()->Synthesize(mel, frames, f0, uv);
                    LOG_EVAL << timer.GetTimer("Neural Synthesis: ");

                    if (neural_audio.empty()) {
                        throw parameter_error("Neural vocoder returned an empty waveform");
                    }

                    timer.SetTimer();
                    std::vector<double> samples;
                    if (source_rate == spec.sample_rate) {
                        samples.assign(neural_audio.begin(), neural_audio.end());
                    } else {
                        const std::vector<double> input(neural_audio.begin(), neural_audio.end());
                        samples = MelSpectrogram::Resample(input, spec.sample_rate, source_rate);
                    }

                    const auto target_samples = static_cast<std::size_t>(request.params.output_samples);
                    if (samples.size() > target_samples) {
                        samples.resize(target_samples);
                    } else if (samples.size() < target_samples) {
                        samples.resize(target_samples, 0.0);
                    }
                    LOG_EVAL << timer.GetTimer("Neural Resample: ");

                    return {std::move(samples), source_rate};
                }
            };

        }// namespace

        std::unique_ptr<VocoderBackend> CreateNeuralBackend() {
            return std::make_unique<NeuralBackend>();
        }

    }// namespace backend

}// namespace lessampler
