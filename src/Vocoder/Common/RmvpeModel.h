/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_RMVPEMODEL_H
#define LESSAMPLER_RMVPEMODEL_H

#include <cstddef>
#include <filesystem>
#include <memory>
#include <span>
#include <vector>

namespace lessampler {

    struct RmvpeOutput {
        bool direct_f0 = false;
        std::size_t frames = 0;
        std::size_t bins = 0;
        std::vector<float> salience;// time-major frames * bins when direct_f0 is false
        std::vector<float> f0;      // frames when direct_f0 is true
        std::vector<std::uint8_t> uv;// 1 = unvoiced when direct_f0 is true

        [[nodiscard]] bool Empty() const noexcept {
            return frames == 0 || (direct_f0 ? f0.empty() : salience.empty());
        }
    };

    /*
     * ONNX Runtime session for RMVPE. Two common exports are supported:
     *  - audio -> salience [1, T, 360] (or [1, 360, T]);
     *  - waveform + threshold -> f0 [T] + uv [T].
     * Input/output names are detected from the graph.
     */
    class RmvpeModel {
    public:
        ~RmvpeModel();

        RmvpeModel(const RmvpeModel &) = delete;
        RmvpeModel &operator=(const RmvpeModel &) = delete;

        [[nodiscard]] const std::filesystem::path &path() const noexcept;

        [[nodiscard]] RmvpeOutput Infer(std::span<const float> audio, float threshold) const;

        [[nodiscard]] static std::shared_ptr<RmvpeModel> Load(const std::filesystem::path &path, int threads);

    private:
        struct Impl;
        explicit RmvpeModel(std::unique_ptr<Impl> impl);

        std::unique_ptr<Impl> impl_;
    };

    /*
     * Process-wide session cache keyed by canonical path, size, mtime, thread
     * count and the RMVPE contract version.
     */
    [[nodiscard]] std::shared_ptr<RmvpeModel> LoadRmvpeModel(const std::filesystem::path &path, int threads);

}// namespace lessampler

#endif//LESSAMPLER_RMVPEMODEL_H
