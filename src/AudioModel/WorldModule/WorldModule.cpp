/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

//
// Created by gloom on 2022/5/2.
//

#include "WorldModule.h"

#include "Utils/LOG.h"

#include <world/dio.h>
#include <world/stonemask.h>
#include <world/cheaptrick.h>
#include <world/harvest.h>
#include <world/d4c.h>

#include <utility>

WorldModule::WorldModule(double *x, int x_length, int fs, lessConfigure config) : x(x), x_length(x_length), configure(std::move(config)) {
    // Set the para
    this->worldPara.fs = fs;
    this->worldPara.frame_period = configure.audio_model_frame_period;

    YALL_DEBUG_ << "Generate F0 from PCM file.";

    if (configure.f0_mode == lessConfigure::F0_MODE::F0_MODE_DIO) {
        F0EstimationDio();
    } else if (configure.f0_mode == lessConfigure::F0_MODE::F0_MODE_HARVEST) {
        F0EstimationHarvest();
    } else {
        throw std::runtime_error("F0 Estimation Mode Error.");
    }
    YALL_DEBUG_ << "Generate Envelope from PCM file and F0.";
    SpectralEnvelopeEstimation();
    YALL_DEBUG_ << "Generate Aperiodicity.";
    AperiodicityEstimation();
}

WorldModule::~WorldModule() {
    delete[] this->worldPara.time_axis;
    delete[] this->worldPara.f0;
    for (int i = 0; i < this->worldPara.f0_length; ++i) {
        delete[] this->worldPara.spectrogram[i];
        delete[] this->worldPara.aperiodicity[i];
    }
    delete[] this->worldPara.spectrogram;
    delete[] this->worldPara.aperiodicity;
}

WorldPara WorldModule::GetModule() {
    return this->worldPara;
}

void WorldModule::F0EstimationDio() {
    DioOption option = {0};
    InitializeDioOption(&option);

    // Modification of the option
    option.frame_period = this->worldPara.frame_period;

    // Valuable option.speed represents the ratio for downsampling.
    // The signal is downsampled to fs / speed Hz.
    // If you want to obtain the accurate result, speed should be set to 1.
    option.speed = configure.f0_speed;

    // You can set the f0_floor below world::kFloorF0.
    option.f0_floor = configure.f0_dio_floor;
    option.allowed_range = configure.f0_allow_range;

    // Parameters setting and memory allocation.
    this->worldPara.f0_length = GetSamplesForDIO(this->worldPara.fs, x_length, this->worldPara.frame_period);
    this->worldPara.f0 = new double[this->worldPara.f0_length];
    this->worldPara.time_axis = new double[this->worldPara.f0_length];
    auto *refined_f0 = new double[this->worldPara.f0_length];

    Dio(x, x_length, this->worldPara.fs, &option, this->worldPara.time_axis, this->worldPara.f0);

    // StoneMask is carried out to improve the estimation performance.
    StoneMask(x, x_length, this->worldPara.fs, this->worldPara.time_axis, this->worldPara.f0, this->worldPara.f0_length, refined_f0);

    for (int i = 0; i < this->worldPara.f0_length; ++i) {
        this->worldPara.f0[i] = refined_f0[i];
    }

    delete[] refined_f0;
}

void WorldModule::F0EstimationHarvest() {
    HarvestOption option = {0};
    InitializeHarvestOption(&option);

    // You can change the frame period.
    // But the estimation is carried out with 1-ms frame shift.
    option.frame_period = this->worldPara.frame_period;

    // You can set the f0_floor below world::kFloorF0.
    option.f0_floor = configure.f0_harvest_floor;

    // Parameters setting and memory allocation.
    this->worldPara.f0_length = GetSamplesForHarvest(this->worldPara.fs, x_length, this->worldPara.frame_period);
    this->worldPara.f0 = new double[this->worldPara.f0_length];
    this->worldPara.time_axis = new double[this->worldPara.f0_length];

    Harvest(x, x_length, this->worldPara.fs, &option, this->worldPara.time_axis, this->worldPara.f0);
}

void WorldModule::SpectralEnvelopeEstimation() {
    CheapTrickOption option = {0};
    // Note (2017/01/02): fs is added as an argument.
    InitializeCheapTrickOption(this->worldPara.fs, &option);

    // Default value was modified to -0.15.
    // option.q1 = -0.15;

    // Important notice (2017/01/02)
    // You can set the fft_size.
    // Default is GetFFTSizeForCheapTrick(this->worldPara.fs, &option);
    // When fft_size changes from default value,
    // a replaced f0_floor will be used in CheapTrick().
    // The lowest F0 that WORLD can work as expected is determined
    // by the following : 3.0 * fs / fft_size.
    option.f0_floor = configure.f0_cheap_trick_floor;
    option.fft_size = [&]() {
        if (configure.custom_fft_size) {
            return configure.fft_size;
        } else {
            return GetFFTSizeForCheapTrick(this->worldPara.fs, &option);
        }
    }();

    // Parameters setting and memory allocation.
    this->worldPara.fft_size = option.fft_size;
    this->worldPara.spectrogram = new double *[this->worldPara.f0_length];
    for (int i = 0; i < this->worldPara.f0_length; ++i) {
        this->worldPara.spectrogram[i] = new double[this->worldPara.fft_size / 2 + 1];
    }

    CheapTrick(x, x_length, this->worldPara.fs, this->worldPara.time_axis, this->worldPara.f0, this->worldPara.f0_length, &option,
               this->worldPara.spectrogram);
}

void WorldModule::AperiodicityEstimation() {
    D4COption option = {0};
    InitializeD4COption(&option);

    // Comment was modified because it was confusing (2017/12/10).
    // It is used to determine the aperiodicity in whole frequency band.
    // D4C identifies whether the frame is voiced segment even if it had an F0.
    // If the estimated value falls below the threshold,
    // the aperiodicity in entire frequency band will set to 1.0.
    // If you want to use the conventional D4C, please set the threshold to 0.0.
    option.threshold = configure.ap_threshold;

    // Parameters setting and memory allocation.
    this->worldPara.aperiodicity = new double *[this->worldPara.f0_length];
    for (int i = 0; i < this->worldPara.f0_length; ++i) {
        this->worldPara.aperiodicity[i] = new double[this->worldPara.fft_size / 2 + 1];
    }

    D4C(x, x_length, this->worldPara.fs, this->worldPara.time_axis, this->worldPara.f0, this->worldPara.f0_length, this->worldPara.fft_size, &option,
        this->worldPara.aperiodicity);
}
