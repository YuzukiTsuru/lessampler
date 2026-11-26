/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "LlsmBackend.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <format>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "Audio/WavIO.h"
#include "Core/Exception.h"
#include "Core/Log.h"
#include "Core/Timer.h"
#include "Core/VoiceStyle.h"
#include "Vocoder/Common/F0Estimator.h"
#include "Vocoder/Llsm/LlsmCache.h"

namespace lessampler {

    namespace backend {

        namespace {

            constexpr int kLayer1FftSize = 2048;
            constexpr double kEpsilon = 1e-8;

            struct AOptionsDeleter {
                void operator()(llsm_aoptions *value) const { llsm_delete_aoptions(value); }
            };

            struct SOptionsDeleter {
                void operator()(llsm_soptions *value) const { llsm_delete_soptions(value); }
            };

            struct OutputDeleter {
                void operator()(llsm_output *value) const { llsm_delete_output(value); }
            };

            using ChunkPtr = LlsmChunkPtr;
            using AOptionsPtr = std::unique_ptr<llsm_aoptions, AOptionsDeleter>;
            using SOptionsPtr = std::unique_ptr<llsm_soptions, SOptionsDeleter>;
            using OutputPtr = std::unique_ptr<llsm_output, OutputDeleter>;

            double Lerp(double a, double b, double ratio) {
                return a + (b - a) * ratio;
            }

            double LerpCircular(double a, double b, double ratio) {
                const double ax = std::cos(a);
                const double ay = std::sin(a);
                const double bx = std::cos(b);
                const double by = std::sin(b);
                return std::atan2(Lerp(ay, by, ratio), Lerp(ax, bx, ratio));
            }

            double MagnitudeToDb(double value) {
                return 20.0 * std::log10(std::max(kEpsilon, value));
            }

            void InterpNoiseFrame(llsm_nmframe *dst, llsm_nmframe *src, double ratio) {
                const int npsd = std::min(dst->npsd, src->npsd);
                for (int i = 0; i < npsd; ++i) {
                    dst->psd[i] = Lerp(dst->psd[i], src->psd[i], ratio);
                }

                const int nchannel = std::min(dst->nchannel, src->nchannel);
                for (int channel = 0; channel < nchannel; ++channel) {
                    llsm_hmframe *src_envelope = src->eenv[channel];
                    llsm_hmframe *dst_envelope = dst->eenv[channel];
                    dst->edc[channel] = Lerp(dst->edc[channel], src->edc[channel], ratio);

                    const int min_harmonics = std::min(src_envelope->nhar, dst_envelope->nhar);
                    const int max_harmonics = std::max(src_envelope->nhar, dst_envelope->nhar);
                    if (dst_envelope->nhar < max_harmonics) {
                        dst_envelope->ampl = static_cast<FP_TYPE *>(
                                realloc(dst_envelope->ampl, sizeof(FP_TYPE) * static_cast<std::size_t>(max_harmonics)));
                        dst_envelope->phse = static_cast<FP_TYPE *>(
                                realloc(dst_envelope->phse, sizeof(FP_TYPE) * static_cast<std::size_t>(max_harmonics)));
                    }
                    for (int i = 0; i < min_harmonics; ++i) {
                        dst_envelope->ampl[i] = Lerp(dst_envelope->ampl[i], src_envelope->ampl[i], ratio);
                        dst_envelope->phse[i] = LerpCircular(dst_envelope->phse[i], src_envelope->phse[i], ratio);
                    }
                    if (max_harmonics == src_envelope->nhar) {
                        for (int i = min_harmonics; i < max_harmonics; ++i) {
                            dst_envelope->ampl[i] = src_envelope->ampl[i];
                            dst_envelope->phse[i] = src_envelope->phse[i];
                        }
                    }
                    dst_envelope->nhar = max_harmonics;
                }
            }

            /*
         * Layer-1 interpolation adapted from libllsm2's demo-stretch.c.
         * Both frames must be layer-1 frames.
         */
            void InterpLlsmFrame(llsm_container *dst, llsm_container *src, double ratio) {
                auto *dst_f0 = static_cast<FP_TYPE *>(llsm_container_get(dst, LLSM_FRAME_F0));
                auto *src_f0 = static_cast<FP_TYPE *>(llsm_container_get(src, LLSM_FRAME_F0));
                auto *dst_nm = static_cast<llsm_nmframe *>(llsm_container_get(dst, LLSM_FRAME_NM));
                auto *src_nm = static_cast<llsm_nmframe *>(llsm_container_get(src, LLSM_FRAME_NM));
                auto *src_rd = static_cast<FP_TYPE *>(llsm_container_get(src, LLSM_FRAME_RD));
                auto *dst_rd = static_cast<FP_TYPE *>(llsm_container_get(dst, LLSM_FRAME_RD));
                auto *dst_vsphse = static_cast<FP_TYPE *>(llsm_container_get(dst, LLSM_FRAME_VSPHSE));
                auto *src_vsphse = static_cast<FP_TYPE *>(llsm_container_get(src, LLSM_FRAME_VSPHSE));
                auto *dst_vtmagn = static_cast<FP_TYPE *>(llsm_container_get(dst, LLSM_FRAME_VTMAGN));
                auto *src_vtmagn = static_cast<FP_TYPE *>(llsm_container_get(src, LLSM_FRAME_VTMAGN));

                const bool has_dst_f0 = dst_f0 != nullptr;
                const bool has_src_f0 = src_f0 != nullptr;
                const double dst_f0_value = has_dst_f0 ? dst_f0[0] : 0.0;
                const double src_f0_value = has_src_f0 ? src_f0[0] : 0.0;
                const bool both_voiced = dst_f0_value > 0.0 && src_f0_value > 0.0;
                llsm_container *voiced = nullptr;
                if (both_voiced) {
                    voiced = dst;
                } else if (src_f0_value > 0.0) {
                    voiced = src;
                } else if (dst_f0_value > 0.0) {
                    voiced = dst;
                }

                const int dst_harmonics = dst_vsphse == nullptr ? 0 : llsm_fparray_length(dst_vsphse);
                const int src_harmonics = src_vsphse == nullptr ? 0 : llsm_fparray_length(src_vsphse);
                const int max_harmonics = std::max(dst_harmonics, src_harmonics);
                const int min_harmonics = std::min(dst_harmonics, src_harmonics);

                if (!both_voiced && voiced == src) {
                    llsm_container_attach(dst, LLSM_FRAME_F0, llsm_create_fp(src_f0_value),
                                          llsm_delete_fp, llsm_copy_fp);
                    if (src_rd != nullptr) {
                        llsm_container_attach(dst, LLSM_FRAME_RD, llsm_create_fp(src_rd[0]),
                                              llsm_delete_fp, llsm_copy_fp);
                    }
                } else if (voiced == nullptr) {
                    llsm_container_attach(dst, LLSM_FRAME_F0, llsm_create_fp(0.0),
                                          llsm_delete_fp, llsm_copy_fp);
                    llsm_container_attach(dst, LLSM_FRAME_RD, llsm_create_fp(1.0),
                                          llsm_delete_fp, llsm_copy_fp);
                }

                const int dst_spectrum_size = dst_vtmagn == nullptr ? 0 : llsm_fparray_length(dst_vtmagn);
                const int src_spectrum_size = src_vtmagn == nullptr ? 0 : llsm_fparray_length(src_vtmagn);
                const int spectrum_size = dst_spectrum_size > 0 ? dst_spectrum_size : src_spectrum_size;
                // llsm_container_attach() frees the old member. Track whichever
                // magnitude array ends up attached so the final clamp never touches
                // a pointer that has already been replaced.
                FP_TYPE *result_magnitudes = dst_vtmagn;
                int result_spectrum_size = dst_spectrum_size;

                if (both_voiced) {
                    llsm_container_attach(dst, LLSM_FRAME_F0,
                                          llsm_create_fp(Lerp(dst_f0_value, src_f0_value, ratio)),
                                          llsm_delete_fp, llsm_copy_fp);
                    if (dst_rd != nullptr && src_rd != nullptr) {
                        llsm_container_attach(dst, LLSM_FRAME_RD,
                                              llsm_create_fp(Lerp(dst_rd[0], src_rd[0], ratio)),
                                              llsm_delete_fp, llsm_copy_fp);
                    }

                    auto *phases = llsm_create_fparray(max_harmonics);
                    auto *magnitudes = llsm_create_fparray(spectrum_size);
                    for (int i = 0; i < min_harmonics; ++i) {
                        phases[i] = LerpCircular(dst_vsphse[i], src_vsphse[i], ratio);
                    }
                    for (int i = 0; i < spectrum_size; ++i) {
                        const double dst_magnitude = dst_vtmagn == nullptr ? 0.0 : dst_vtmagn[i];
                        const double src_magnitude = src_vtmagn == nullptr ? 0.0 : src_vtmagn[i];
                        magnitudes[i] = Lerp(dst_magnitude, src_magnitude, ratio);
                    }
                    if (dst_harmonics < src_harmonics) {
                        for (int i = min_harmonics; i < max_harmonics; ++i) {
                            phases[i] = src_vsphse[i];
                        }
                    }
                    llsm_container_attach(dst, LLSM_FRAME_VSPHSE, phases,
                                          llsm_delete_fparray, llsm_copy_fparray);
                    llsm_container_attach(dst, LLSM_FRAME_VTMAGN, magnitudes,
                                          llsm_delete_fparray, llsm_copy_fparray);
                    result_magnitudes = magnitudes;
                    result_spectrum_size = spectrum_size;
                } else if (voiced == src) {
                    auto *phases = src_vsphse == nullptr ? llsm_create_fparray(0) : llsm_copy_fparray(src_vsphse);
                    auto *magnitudes = src_vtmagn == nullptr ? llsm_create_fparray(0) : llsm_copy_fparray(src_vtmagn);
                    llsm_container_attach(dst, LLSM_FRAME_VSPHSE, phases,
                                          llsm_delete_fparray, llsm_copy_fparray);
                    llsm_container_attach(dst, LLSM_FRAME_VTMAGN, magnitudes,
                                          llsm_delete_fparray, llsm_copy_fparray);
                    result_magnitudes = magnitudes;
                    result_spectrum_size = src_spectrum_size;
                    const double fade = MagnitudeToDb(std::max(kEpsilon, ratio));
                    for (int i = 0; i < src_spectrum_size; ++i) {
                        magnitudes[i] += fade;
                    }
                } else if (dst_vtmagn != nullptr) {
                    const double fade = MagnitudeToDb(std::max(kEpsilon, 1.0 - ratio));
                    for (int i = 0; i < dst_spectrum_size; ++i) {
                        dst_vtmagn[i] += fade;
                    }
                }

                if (result_magnitudes != nullptr) {
                    for (int i = 0; i < result_spectrum_size; ++i) {
                        result_magnitudes[i] = std::max(-80.0f, result_magnitudes[i]);
                    }
                }

                if (dst_nm != nullptr && src_nm != nullptr) {
                    InterpNoiseFrame(dst_nm, src_nm, ratio);
                }
            }

            /*
             * Clear and cry for LLSM: shift the vocal-tract magnitude response
             * and the noise model. Both quantities are stored in dB, so the
             * weight-derived gains are simply added or subtracted.
             */
            void ApplyStyleToLlsmFrame(llsm_container *frame, const StyleWeights &weights) {
                if (weights.clear == 0.0 && weights.cry == 0.0) {
                    return;
                }
                const double harmonic_db = 0.5 * weights.clear + 1.0 * weights.cry;
                const double noise_db = -6.0 * weights.clear + 0.5 * weights.cry;

                if (auto *vtmagn = static_cast<FP_TYPE *>(llsm_container_get(frame, LLSM_FRAME_VTMAGN))) {
                    const int count = llsm_fparray_length(vtmagn);
                    for (int i = 0; i < count; ++i) {
                        vtmagn[i] += static_cast<FP_TYPE>(harmonic_db);
                    }
                }

                auto *nm = static_cast<llsm_nmframe *>(llsm_container_get(frame, LLSM_FRAME_NM));
                if (nm == nullptr) {
                    return;
                }
                for (int i = 0; i < nm->npsd; ++i) {
                    nm->psd[i] += static_cast<FP_TYPE>(noise_db);
                }
                const double noise_gain = std::pow(10.0, noise_db / 20.0);
                for (int channel = 0; channel < nm->nchannel; ++channel) {
                    nm->edc[channel] = static_cast<FP_TYPE>(nm->edc[channel] * noise_gain);
                    if (nm->eenv[channel] == nullptr) {
                        continue;
                    }
                    for (int i = 0; i < nm->eenv[channel]->nhar; ++i) {
                        nm->eenv[channel]->ampl[i] =
                                static_cast<FP_TYPE>(nm->eenv[channel]->ampl[i] * noise_gain);
                    }
                }
            }

            double GetAverageF0(std::span<const double> f0) {
                double sum = 0.0;
                double weight = 0.0;
                for (std::size_t i = 0; i < f0.size(); ++i) {
                    const double value = f0[i];
                    if (value >= 1000.0 || value <= 55.0) {
                        continue;
                    }
                    double confidence = 1.0;
                    for (int j = 0; j <= 5; ++j) {
                        if (i > static_cast<std::size_t>(j)) {
                            const double delta = f0[i - static_cast<std::size_t>(j) - 1] - value;
                            confidence *= value / (value + delta * delta);
                        } else {
                            confidence *= 1.0 / (1.0 + value);
                        }
                    }
                    sum += value * confidence;
                    weight += confidence;
                }
                return weight > 0.0 ? sum / weight : 0.0;
            }

            void EqualizePitch(std::span<double> f0, const RenderParams &shine) {
                const double average = GetAverageF0(f0);
                for (double &value: f0) {
                    if (value == 0.0) {
                        continue;
                    }
                    if (average == 0.0) {
                        value = shine.scale_num;
                    } else {
                        value = ((value - average) * shine.modulation / 100.0 + average) * (shine.scale_num / average);
                    }
                }
            }

            class LlsmAnalysis final : public Analysis {
            public:
                LlsmAnalysis(LlsmChunkPtr chunk, std::vector<double> f0, double frame_period, int fs)
                    : chunk_(std::move(chunk)), f0_(std::move(f0)), frame_period_(frame_period), fs_(fs) {}

                [[nodiscard]] VocoderKind kind() const noexcept override { return VocoderKind::llsm; }

                [[nodiscard]] llsm_chunk *chunk() const noexcept { return chunk_.get(); }

                [[nodiscard]] const std::vector<double> &f0() const noexcept { return f0_; }

                [[nodiscard]] double frame_period() const noexcept { return frame_period_; }

                [[nodiscard]] int fs() const noexcept { return fs_; }

            private:
                LlsmChunkPtr chunk_;
                std::vector<double> f0_;
                double frame_period_ = 0.0;
                int fs_ = 0;
            };

            bool ReadCachedF0(llsm_chunk *chunk, std::vector<double> &f0) {
                const auto *nfrm = static_cast<const int *>(llsm_container_get(chunk->conf, LLSM_CONF_NFRM));
                if (nfrm == nullptr || *nfrm <= 0) {
                    return false;
                }
                f0.resize(static_cast<std::size_t>(*nfrm));
                for (std::size_t i = 0; i < f0.size(); ++i) {
                    const auto *frame_f0 = static_cast<const FP_TYPE *>(
                            llsm_container_get(chunk->frames[i], LLSM_FRAME_F0));
                    if (frame_f0 == nullptr) {
                        f0.clear();
                        return false;
                    }
                    f0[i] = frame_f0[0];
                }
                return true;
            }

            class LlsmBackend final : public VocoderBackend {
            public:
                [[nodiscard]] VocoderKind kind() const noexcept override { return VocoderKind::llsm; }

                [[nodiscard]] std::string_view name() const noexcept override { return "llsm"; }

                [[nodiscard]] std::uint32_t analysis_version() const noexcept override { return 2; }

                [[nodiscard]] AnalysisPtr Prepare(const AnalysisRequest &request) override {
                    static_assert(std::is_same_v<FP_TYPE, float>);
                    const auto configure = ResolveF0EstimatorConfig(request.configure, kind());
                    const int fs = request.source.info().sample_rate;
                    double frame_period = configure.audio_model_frame_period;

                    Timer timer;
                    timer.SetTimer();
                    ChunkPtr source = LoadLlsmCache(request.source.path(), configure);
                    std::vector<double> f0;
                    if (source && !ReadCachedF0(source.get(), f0)) {
                        source.reset();
                        f0.clear();
                    }

                    if (source) {
                        LOG_EVAL << timer.GetTimer("LLSM Cache Load: ");
                        return std::make_shared<LlsmAnalysis>(std::move(source), std::move(f0), frame_period, fs);
                    }

                    const auto samples = request.source.samples();
                    if (samples.empty()) {
                        throw parameter_error("Empty audio input");
                    }

                    timer.SetTimer();
                    auto estimator = CreateF0Estimator(configure.f0_estimator, configure);
                    auto track = estimator->Estimate(samples, fs, configure);
                    if (track.f0.empty()) {
                        throw parameter_error("LLSM F0 estimation produced no frames");
                    }
                    frame_period = track.frame_period;
                    LOG_INFO << timer.GetTimer("LLSM F0 Estimation: ");

                    AOptionsPtr analysis_options(llsm_create_aoptions());
                    if (!analysis_options) {
                        throw std::runtime_error("Cannot create LLSM analysis options");
                    }
                    analysis_options->thop = frame_period / 1000.0;
                    analysis_options->npsd = 128;
                    analysis_options->maxnhar = 400;
                    analysis_options->maxnhar_e = 5;
                    analysis_options->hm_method = LLSM_AOPTION_HMPP;
                    // The selected F0 estimator already applied any refinement
                    // (llsm_refine_f0 when estimator = llsm); do not refine twice.
                    analysis_options->f0_refine = 0;

                    timer.SetTimer();
                    std::vector<FP_TYPE> analysis_samples(samples.size());
                    std::ranges::transform(samples, analysis_samples.begin(),
                                           [](double value) { return static_cast<FP_TYPE>(value); });
                    std::vector<FP_TYPE> analysis_f0(track.f0.size());
                    std::ranges::transform(track.f0, analysis_f0.begin(),
                                           [](double value) { return static_cast<FP_TYPE>(value); });
                    LOG_EVAL << timer.GetTimer("LLSM Conversion: ");

                    timer.SetTimer();
                    source.reset(llsm_analyze(analysis_options.get(),
                                              analysis_samples.data(),
                                              static_cast<int>(analysis_samples.size()), fs,
                                              analysis_f0.data(), static_cast<int>(analysis_f0.size()), nullptr));
                    if (!source) {
                        throw std::runtime_error("LLSM analysis failed");
                    }
                    LOG_INFO << timer.GetTimer("LLSM Analysis: ");

                    // llsm_analyze() does not retain either buffer; release the
                    // float copies before the transform allocates its chunks.
                    std::vector<FP_TYPE>().swap(analysis_samples);
                    std::vector<FP_TYPE>().swap(analysis_f0);

                    timer.SetTimer();
                    llsm_chunk_tolayer1(source.get(), kLayer1FftSize);
                    LOG_EVAL << timer.GetTimer("LLSM Layer 1: ");

                    timer.SetTimer();
                    llsm_chunk_phasepropagate(source.get(), -1);
                    LOG_EVAL << timer.GetTimer("LLSM Source Phase Propagation: ");

                    f0 = std::move(track.f0);
                    timer.SetTimer();
                    try {
                        SaveLlsmCache(request.source.path(), configure, source.get());
                        LOG_EVAL << timer.GetTimer("LLSM Cache Save: ");
                    } catch (const std::exception &error) {
                        LOG_DEBUG << std::format("Cannot save LLSM cache: {}", error.what());
                    }

                    return std::make_shared<LlsmAnalysis>(std::move(source), std::move(f0), frame_period, fs);
                }

                [[nodiscard]] RenderResult Synthesize(const SynthesisRequest &request) override {
                    const auto &analysis = static_cast<const LlsmAnalysis &>(request.analysis);
                    const auto &params = request.params;
                    if (params.output_samples <= 0 || params.required_frame <= 0) {
                        throw parameter_error("Invalid LLSM output length");
                    }
                    if (params.pitch_step <= 0 || params.pitch_bend.size() < 2) {
                        throw parameter_error("Invalid LLSM pitch parameters");
                    }

                    llsm_chunk *source = analysis.chunk();
                    std::vector<double> f0 = analysis.f0();
                    if (source == nullptr || f0.empty()) {
                        throw parameter_error("Invalid LLSM analysis");
                    }

                    const std::size_t input_frames = f0.size();
                    const double frame_period = analysis.frame_period();
                    const int fs = analysis.fs();
                    Timer timer;

                    EqualizePitch(f0, params);
                    double average_f0 = GetAverageF0(f0);
                    if (!(average_f0 > 0.0)) {
                        average_f0 = params.scale_num > 0.0 ? params.scale_num : 1.0;
                    }

                    const auto output_frames = static_cast<std::size_t>(params.required_frame);
                    llsm_container *conf = llsm_copy_container(source->conf);
                    llsm_container_attach(conf, LLSM_CONF_NFRM, llsm_create_int(params.required_frame),
                                          llsm_delete_int, llsm_copy_int);
                    ChunkPtr transformed(llsm_create_chunk(conf, 0));
                    llsm_delete_container(conf);
                    if (!transformed || transformed->frames == nullptr) {
                        throw std::runtime_error("Cannot allocate LLSM output chunk");
                    }

                    const std::size_t last_input = input_frames - 1;
                    const std::size_t max_pitch_index = params.pitch_bend.size() - 2;
                    const double note_duration_ms = static_cast<double>(output_frames) * frame_period;

                    timer.SetTimer();
                    for (std::size_t i = 0; i < output_frames; ++i) {
                        const double out_sample = frame_period * static_cast<double>(i);
                        double in_sample;
                        if (out_sample < params.base_length) {
                            in_sample = params.offset + out_sample * params.velocity;
                        } else {
                            in_sample = params.offset + params.first_half_fixed_part + (out_sample - params.base_length) * params.stretch_length;
                        }

                        const double position = in_sample / frame_period;
                        std::size_t base = 0;
                        double fraction = 0.0;
                        if (position > 0.0) {
                            base = static_cast<std::size_t>(std::floor(position));
                            if (base > last_input) {
                                base = last_input;
                            }
                            fraction = position - static_cast<double>(base);
                        }
                        const std::size_t next = std::min(base + 1, last_input);

                        transformed->frames[i] = llsm_copy_container(source->frames[base]);
                        // Always interpolate when a next frame exists, not only when
                        // the fraction is positive. This is what makes an unvoiced
                        // source frame inherit the layer-1 envelope of the next
                        // voiced frame before we assign it a non-zero target F0.
                        if (next != base) {
                            InterpLlsmFrame(transformed->frames[i], source->frames[next], fraction);
                        }

                        double temp_f0 = f0[base];
                        double next_f0 = f0[next];
                        const bool voiced = temp_f0 > 0.0 || (next != base && next_f0 > 0.0);
                        double target_f0 = 0.0;
                        if (voiced) {
                            if (temp_f0 != 0.0 || next_f0 != 0.0) {
                                if (temp_f0 == 0.0) {
                                    temp_f0 = average_f0;
                                }
                                if (next_f0 == 0.0) {
                                    next_f0 = average_f0;
                                }
                                if (next != base) {
                                    temp_f0 = temp_f0 * (1.0 - fraction) + next_f0 * fraction;
                                }
                            }
                            const double pitch_position = out_sample * 0.001 * static_cast<double>(fs) / params.pitch_step;
                            std::size_t pitch_index = 0;
                            double pitch_fraction = 0.0;
                            if (pitch_position > 0.0) {
                                pitch_index = static_cast<std::size_t>(std::floor(pitch_position));
                                if (pitch_index > max_pitch_index) {
                                    pitch_index = max_pitch_index;
                                }
                                pitch_fraction = pitch_position - static_cast<double>(pitch_index);
                                pitch_fraction = std::min(1.0, pitch_fraction);
                            }
                            const double bend = params.pitch_bend[pitch_index] * (1.0 - pitch_fraction) + params.pitch_bend[pitch_index + 1] * pitch_fraction;
                            const double pitch_base = params.scale_num * std::pow(2.0, bend / 1200.0);
                            target_f0 = pitch_base * std::pow(temp_f0 / average_f0,
                                                              params.modulation * 0.01);
                        }

                        auto *frame_f0 = static_cast<FP_TYPE *>(
                                llsm_container_get(transformed->frames[i], LLSM_FRAME_F0));
                        if (frame_f0 == nullptr) {
                            llsm_container_attach(transformed->frames[i], LLSM_FRAME_F0,
                                                  llsm_create_fp(target_f0), llsm_delete_fp, llsm_copy_fp);
                        } else {
                            frame_f0[0] = target_f0;
                        }

                        ApplyStyleToLlsmFrame(
                                transformed->frames[i],
                                StyleAt(params.style, out_sample, note_duration_ms));
                    }
                    LOG_INFO << timer.GetTimer("LLSM Transform: ");

                    timer.SetTimer();
                    llsm_chunk_tolayer0(transformed.get());
                    LOG_EVAL << timer.GetTimer("LLSM Layer 0: ");

                    timer.SetTimer();
                    llsm_chunk_phasepropagate(transformed.get(), 1);
                    LOG_EVAL << timer.GetTimer("LLSM Target Phase Propagation: ");

                    SOptionsPtr synthesis_options(llsm_create_soptions(fs));
                    if (!synthesis_options) {
                        throw std::runtime_error("Cannot create LLSM synthesis options");
                    }
                    synthesis_options->use_l1 = 0;
                    // libllsm2 generates its noise excitation with the C library
                    // rand(). Reseed so two renders of the same input and
                    // parameters are sample-identical instead of depending on how
                    // many notes the process has already rendered.
                    std::srand(1);
                    timer.SetTimer();
                    OutputPtr output(llsm_synthesize(synthesis_options.get(), transformed.get()));
                    if (!output) {
                        throw std::runtime_error("LLSM synthesis failed");
                    }
                    LOG_INFO << timer.GetTimer("LLSM Synthesis: ");

                    std::vector<double> result(static_cast<std::size_t>(params.output_samples), 0.0);
                    const auto copy_count = std::min<std::size_t>(result.size(),
                                                                  static_cast<std::size_t>(output->ny));
                    std::copy(output->y, output->y + static_cast<std::ptrdiff_t>(copy_count), result.begin());
                    return {std::move(result), fs};
                }
            };

        }// namespace

        std::unique_ptr<VocoderBackend> CreateLlsmBackend() {
            return std::make_unique<LlsmBackend>();
        }

    }// namespace backend

}// namespace lessampler
