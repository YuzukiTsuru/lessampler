/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "WorldBackend.h"

#include <format>
#include <memory>
#include <utility>

#include "Core/Log.h"
#include "Core/Timer.h"
#include "Vocoder/World/WorldModelStore.h"
#include "Vocoder/World/WorldSynthesizer.h"
#include "Vocoder/World/WorldTransform.h"

namespace lessampler {

    namespace backend {

        namespace {

            class WorldAnalysis final : public Analysis {
            public:
                explicit WorldAnalysis(WorldModel model) : model_(std::move(model)) {}

                [[nodiscard]] VocoderKind kind() const noexcept override { return VocoderKind::world; }

                [[nodiscard]] const WorldModel &model() const noexcept { return model_; }

            private:
                WorldModel model_;
            };

            class WorldBackend final : public VocoderBackend {
            public:
                [[nodiscard]] VocoderKind kind() const noexcept override { return VocoderKind::world; }

                [[nodiscard]] std::string_view name() const noexcept override { return "world"; }

                [[nodiscard]] std::uint32_t analysis_version() const noexcept override { return 1; }

                [[nodiscard]] AnalysisPtr Prepare(const AnalysisRequest &request) override {
                    Timer timer;
                    timer.SetTimer();
                    auto model = LoadOrBuildWorldModel(request.source, request.configure, request.max_workers);
                    YALL_EVAL_ << timer.GetTimer("WORLD Model Prepare: ");

                    YALL_INFO_ << std::format("Audio Sample Rate: {}", model.fs);
                    YALL_INFO_ << std::format("Model Frame Period: {}", model.frame_period);
                    YALL_INFO_ << std::format("Model FFT Size: {}", model.fft_size);

                    return std::make_shared<WorldAnalysis>(std::move(model));
                }

                [[nodiscard]] RenderResult Synthesize(const SynthesisRequest &request) override {
                    const auto &analysis = static_cast<const WorldAnalysis &>(request.analysis);

                    Timer timer;
                    timer.SetTimer();
                    WorldTransform transform(analysis.model(), request.params);
                    const auto &transformed = transform.GetTransAudioModel();
                    YALL_EVAL_ << timer.GetTimer("WORLD Transform: ");

                    timer.SetTimer();
                    auto output = SynthesizeWorld(transformed, request.params.output_samples);
                    YALL_EVAL_ << timer.GetTimer("WORLD Synthesis: ");

                    return {std::move(output), transformed.fs};
                }
            };

        }// namespace

        std::unique_ptr<VocoderBackend> CreateWorldBackend() {
            return std::make_unique<WorldBackend>();
        }

    }// namespace backend

}// namespace lessampler
