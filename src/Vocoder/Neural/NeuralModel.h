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

namespace lessampler {

    /*
     * ONNX Runtime session for an NHVSing-compatible vocoder graph:
     * mel [1,T,128], f0 [1,1,T], uv [1,1,T] -> waveform [1,1,256*T].
     *
     * The model is loaded once per process and shared by every analysis.
     */
    class NeuralModel {
    public:
        static constexpr int model_sample_rate = 44100;
        static constexpr int hop_size = 256;
        static constexpr int mel_bins = 128;

        ~NeuralModel();

        NeuralModel(const NeuralModel &) = delete;
        NeuralModel &operator=(const NeuralModel &) = delete;

        [[nodiscard]] const std::filesystem::path &path() const noexcept;

        [[nodiscard]] std::vector<float> Synthesize(std::span<const float> mel, std::size_t frames,
                                                    std::span<const float> f0,
                                                    std::span<const float> uv) const;

        [[nodiscard]] static std::shared_ptr<NeuralModel> Load(const std::filesystem::path &path, int threads);

    private:
        struct Impl;
        explicit NeuralModel(std::unique_ptr<Impl> impl);

        std::unique_ptr<Impl> impl_;
    };

    /*
     * Process-wide session cache keyed by canonical path, size, mtime, and
     * thread count.
     */
    [[nodiscard]] std::shared_ptr<NeuralModel> LoadNeuralModel(const std::filesystem::path &path, int threads);

}// namespace lessampler

#endif//LESSAMPLER_NEURALMODEL_H
