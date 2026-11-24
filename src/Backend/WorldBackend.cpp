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
#include <utility>

#include "AudioModel/Synthesis/Synthesis.h"
#include "AudioProcess/AudioProcess.h"
#include "FileIO/AudioModelIO.h"
#include "FileIO/GenerateAudioModel.h"
#include "Utils/LOG.h"

namespace lessbackend {

    namespace {

        class WorldBackend final : public VocoderBackend {
        public:
            [[nodiscard]] VocoderKind kind() const noexcept override { return VocoderKind::world; }

            [[nodiscard]] std::string_view name() const noexcept override { return "world"; }

            [[nodiscard]] RenderResult Render(const RenderRequest &request) override {
                AudioModelIO model_io(request.input_path);
                if (!model_io.CheckAudioModel(request.configure)) {
                    YALL_INFO_ << "Generating WORLD audio model: " + request.input_path.string();
                    GenerateAudioModel::GenerateModelForFile(request.input_path, request.configure);
                }

                const auto &model = model_io.ReadAudioModel(request.configure);
                YALL_INFO_ << std::format("Audio Sample Rate: {}", model.fs);
                YALL_INFO_ << std::format("Model Frame Period: {}", model.frame_period);
                YALL_INFO_ << std::format("Model FFT Size: {}", model.fft_size);

                AudioProcess process(model, request.shine);
                const auto &transformed = process.GetTransAudioModel();
                Synthesis synthesis(transformed, request.shine.output_samples);

                return {std::move(synthesis.GetWavData()), transformed.fs};
            }
        };

    }// namespace

    std::unique_ptr<VocoderBackend> CreateWorldBackend() {
        return std::make_unique<WorldBackend>();
    }

}// namespace lessbackend
