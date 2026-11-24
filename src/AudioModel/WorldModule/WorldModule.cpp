/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "WorldModule.h"

#include <algorithm>
#include <thread>
#include <utility>
#include <vector>

#include <world/cheaptrick.h>
#include <world/d4c.h>

#include "F0Estimator.h"
#include "Utils/LOG.h"
#include "Utils/Timer.h"
#include "Utils/exception.h"

WorldModule::WorldModule(std::span<const double> samples, int sample_rate, const lessConfigure &config)
    : x(samples.data()), x_length(static_cast<int>(samples.size())), configure(config) {
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

WorldPara WorldModule::TakeModule() {
    return std::move(worldPara);
}

void WorldModule::SpectralEnvelopeEstimation() {
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

void WorldModule::AperiodicityEstimation() {
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

    // D4C works one frame at a time. Split the frame range across a fixed
    // number of blocks so the partition (and therefore the random safeguard
    // sequence) does not depend on the machine's core count. Each call owns its
    // FFT and random state, and every call writes a disjoint block of rows.
    constexpr int max_workers = 8;
    const int thread_count = std::min(max_workers, frame_count);
    if (thread_count <= 1) {
        D4C(x, x_length, worldPara.fs, worldPara.time_axis.data(), worldPara.f0.data(),
            frame_count, worldPara.fft_size, &option, pointers.data());
    } else {
        const int chunk = (frame_count + thread_count - 1) / thread_count;
        std::vector<std::jthread> threads;
        threads.reserve(static_cast<std::size_t>(thread_count - 1));
        for (int start = 0; start < frame_count; start += chunk) {
            const int count = std::min(chunk, frame_count - start);
            threads.emplace_back([&, start, count]() {
                D4C(x, x_length, worldPara.fs, worldPara.time_axis.data() + start,
                    worldPara.f0.data() + start, count, worldPara.fft_size, &option,
                    pointers.data() + start);
            });
        }
    }
}
