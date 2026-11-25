/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_NEURALMODEL_H
#define LESSAMPLER_NEURALMODEL_H

#include <cstddef>
#include <filesystem>
#include <memory>
#include <span>
#include <vector>

#include "Config/NeuralVocoderSpec.h"

namespace lessampler {

    /*
     * ONNX Runtime session for a neural vocoder. The input contract is
     * described by NeuralVocoderSpec: NHVSing by default (mel/f0/uv ->
     * waveform), or a PC-NSF-HiFiGAN-style graph with configurable names,
     * shapes and mel layout.
     */
    class NeuralModel {
    public:
        ~NeuralModel();

        NeuralModel(const NeuralModel &) = delete;
        NeuralModel &operator=(const NeuralModel &) = delete;

        [[nodiscard]] const std::filesystem::path &path() const noexcept;
        [[nodiscard]] const NeuralVocoderSpec &spec() const noexcept;

        [[nodiscard]] std::vector<float> Synthesize(std::span<const float> mel, std::size_t frames,
                                                    std::span<const float> f0,
                                                    std::span<const float> uv) const;

        [[nodiscard]] static std::shared_ptr<NeuralModel> Load(const std::filesystem::path &path, int threads,
                                                               const NeuralVocoderSpec &spec);

    private:
        struct Impl;
        explicit NeuralModel(std::unique_ptr<Impl> impl);

        std::unique_ptr<Impl> impl_;
    };

    /*
     * Process-wide session cache keyed by canonical path, size, mtime, thread
     * count and the vocoder spec fingerprint.
     */
    [[nodiscard]] std::shared_ptr<NeuralModel> LoadNeuralModel(const std::filesystem::path &path, int threads,
                                                               const NeuralVocoderSpec &spec);

}// namespace lessampler

#endif//LESSAMPLER_NEURALMODEL_H
