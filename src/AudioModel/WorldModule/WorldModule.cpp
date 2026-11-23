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

#include <utility>
#include <vector>

#include <world/cheaptrick.h>
#include <world/d4c.h>
#include <world/dio.h>
#include <world/harvest.h>
#include <world/stonemask.h>

#include "Utils/LOG.h"
#include "Utils/Timer.h"
#include "Utils/exception.h"

WorldModule::WorldModule(std::span<const double> samples, int sample_rate, const lessConfigure &config)
    : x(samples.data()), x_length(static_cast<int>(samples.size())), configure(config) {
    if (x == nullptr || x_length <= 0) {
        throw parameter_error("Empty audio input");
    }

    worldPara.fs = sample_rate;
    worldPara.frame_period = configure.audio_model_frame_period;

    YALL_DEBUG_ << "Generate F0 from PCM file.";

    Timer timer;

    if (configure.f0_mode == lessConfigure::F0_MODE::F0_MODE_DIO) {
        timer.SetTimer();
        F0EstimationDio();
        YALL_EVAL_ << timer.GetTimer("DIO F0 Estimation: ");
    } else if (configure.f0_mode == lessConfigure::F0_MODE::F0_MODE_HARVEST) {
        timer.SetTimer();
        F0EstimationHarvest();
        YALL_EVAL_ << timer.GetTimer("HARVEST F0 Estimation: ");
    } else {
        throw parameter_error("F0 estimation mode is unknown");
    }

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

void WorldModule::F0EstimationDio() {
    DioOption option{};
    InitializeDioOption(&option);

    option.frame_period = worldPara.frame_period;
    option.speed = configure.f0_speed;
    option.f0_floor = configure.f0_dio_floor;
    option.allowed_range = configure.f0_allow_range;

    const int f0_length = GetSamplesForDIO(worldPara.fs, x_length, worldPara.frame_period);
    worldPara.f0.resize(static_cast<std::size_t>(f0_length));
    worldPara.time_axis.resize(static_cast<std::size_t>(f0_length));
    std::vector<double> refined_f0(static_cast<std::size_t>(f0_length));

    Dio(x, x_length, worldPara.fs, &option, worldPara.time_axis.data(), worldPara.f0.data());
    StoneMask(x, x_length, worldPara.fs, worldPara.time_axis.data(), worldPara.f0.data(), f0_length,
              refined_f0.data());

    worldPara.f0 = std::move(refined_f0);
}

void WorldModule::F0EstimationHarvest() {
    HarvestOption option{};
    InitializeHarvestOption(&option);

    option.frame_period = worldPara.frame_period;
    option.f0_floor = configure.f0_harvest_floor;

    const int f0_length = GetSamplesForHarvest(worldPara.fs, x_length, worldPara.frame_period);
    worldPara.f0.resize(static_cast<std::size_t>(f0_length));
    worldPara.time_axis.resize(static_cast<std::size_t>(f0_length));

    Harvest(x, x_length, worldPara.fs, &option, worldPara.time_axis.data(), worldPara.f0.data());
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
    worldPara.aperiodicity.assign(worldPara.f0.size(), static_cast<std::size_t>(w_length), 0.0);

    auto pointers = worldPara.aperiodicity.row_pointers();
    D4C(x, x_length, worldPara.fs, worldPara.time_axis.data(), worldPara.f0.data(),
        static_cast<int>(worldPara.f0.size()), worldPara.fft_size, &option, pointers.data());
}
