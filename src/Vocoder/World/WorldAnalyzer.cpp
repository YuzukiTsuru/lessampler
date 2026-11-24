/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "WorldAnalyzer.h"

#include <algorithm>
#include <atomic>
#include <thread>
#include <utility>
#include <vector>

#include <world/cheaptrick.h>
#include <world/d4c.h>

#include "Core/Exception.h"
#include "Core/Log.h"
#include "Core/Timer.h"
#include "Vocoder/Common/F0Estimator.h"

namespace lessampler {

    WorldAnalyzer::WorldAnalyzer(std::span<const double> samples, int sample_rate, const Config &config,
                                 int max_workers)
        : x(samples.data()), x_length(static_cast<int>(samples.size())), configure(config), max_workers_(max_workers) {
        if (x == nullptr || x_length <= 0) {
            throw parameter_error("Empty audio input");
        }

        worldPara.fs = sample_rate;

        YALL_DEBUG_ << "Generate F0 from PCM file.";

        Timer timer;
        timer.SetTimer();
        auto track = EstimateF0(samples, sample_rate, configure);
        worldPara.frame_period = track.frame_period;
        worldPara.f0 = std::move(track.f0);
        worldPara.time_axis = std::move(track.time_axis);
        YALL_EVAL_ << timer.GetTimer("F0 Estimation: ");

        YALL_DEBUG_ << "Generate Envelope from PCM file and F0.";
        timer.SetTimer();
        SpectralEnvelopeEstimation();
        YALL_EVAL_ << timer.GetTimer("Spectral Envelope Estimation: ");

        YALL_DEBUG_ << "Generate Aperiodicity.";
        timer.SetTimer();
        AperiodicityEstimation();
        YALL_EVAL_ << timer.GetTimer("Aperiodicity Estimation: ");
    }

    WorldPara WorldAnalyzer::TakeModule() {
        return std::move(worldPara);
    }

    void WorldAnalyzer::SpectralEnvelopeEstimation() {
        CheapTrickOption option{};
        InitializeCheapTrickOption(worldPara.fs, &option);

        option.f0_floor = configure.f0_cheap_trick_floor;
        option.fft_size = [&]() {
            if (configure.custom_fft_size) {
                return configure.fft_size;
            }
            return GetFFTSizeForCheapTrick(worldPara.fs, &option);
        }();

        worldPara.fft_size = option.fft_size;
        const int w_length = worldPara.fft_size / 2 + 1;
        worldPara.spectrogram.assign(worldPara.f0.size(), static_cast<std::size_t>(w_length), 0.0);

        auto pointers = worldPara.spectrogram.row_pointers();
        CheapTrick(x, x_length, worldPara.fs, worldPara.time_axis.data(), worldPara.f0.data(),
                   static_cast<int>(worldPara.f0.size()), &option, pointers.data());
    }

    void WorldAnalyzer::AperiodicityEstimation() {
        D4COption option{};
        InitializeD4COption(&option);
        option.threshold = configure.ap_threshold;

        const int w_length = worldPara.fft_size / 2 + 1;
        const auto frame_count = static_cast<int>(worldPara.f0.size());
        worldPara.aperiodicity.assign(worldPara.f0.size(), static_cast<std::size_t>(w_length), 0.0);

        auto pointers = worldPara.aperiodicity.row_pointers();
        if (frame_count <= 0) {
            return;
        }

        // D4C works one frame at a time. Use a fixed eight-block partition so the
        // random-safeguard sequence is independent of the worker count, then let a
        // bounded number of threads drain those blocks.
        constexpr int fixed_chunks = 8;
        const int chunk_count = std::min(fixed_chunks, frame_count);
        const int chunk = (frame_count + chunk_count - 1) / chunk_count;
        const int workers = max_workers_ > 0 ? std::min(max_workers_, chunk_count) : chunk_count;
        const auto process_chunk = [&](int index) {
            const int start = index * chunk;
            if (start >= frame_count) {
                return;
            }
            const int count = std::min(chunk, frame_count - start);
            D4C(x, x_length, worldPara.fs, worldPara.time_axis.data() + start,
                worldPara.f0.data() + start, count, worldPara.fft_size, &option,
                pointers.data() + start);
        };

        if (workers <= 1) {
            for (int index = 0; index < chunk_count; ++index) {
                process_chunk(index);
            }
            return;
        }

        std::atomic<int> next{0};
        std::vector<std::jthread> threads;
        threads.reserve(static_cast<std::size_t>(workers - 1));
        const auto worker = [&]() {
            while (true) {
                const int index = next.fetch_add(1, std::memory_order_relaxed);
                if (index >= chunk_count) {
                    return;
                }
                process_chunk(index);
            }
        };
        for (int i = 1; i < workers; ++i) {
            threads.emplace_back(worker);
        }
        worker();
        threads.clear();
    }

}// namespace lessampler
