/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Synthesis.h"

#include <algorithm>
#include <vector>

#include <world/synthesisrealtime.h>

#include "Utils/LOG.h"
#include "Utils/exception.h"

namespace {

    struct SynthesizerGuard {
        WorldSynthesizer synthesizer{};

        ~SynthesizerGuard() { DestroySynthesizer(&synthesizer); }
    };

}// namespace

Synthesis::Synthesis(const lessAudioModel &audioModel, int x_length)
    : x(static_cast<std::size_t>(std::max(0, x_length)), 0.0) {
    YALL_DEBUG_ << "Allocate Out Memory, Length: " + std::to_string(x_length);
    SynthesisWav(audioModel);
}

std::vector<double> &Synthesis::GetWavData() noexcept {
    return x;
}

const std::vector<double> &Synthesis::GetWavData() const noexcept {
    return x;
}

void Synthesis::SynthesisWav(const lessAudioModel &audioModel) {
    if (audioModel.frame_period <= 0.0 || audioModel.fft_size <= 0 || audioModel.fs <= 0) {
        throw parameter_error("Invalid audio model parameters for synthesis");
    }

    const std::size_t frame_count = std::min({audioModel.f0.size(),
                                              audioModel.spectrogram.rows(),
                                              audioModel.aperiodicity.rows()});
    if (frame_count == 0) {
        return;
    }

    SynthesizerGuard guard;
    constexpr int buffer_size = 64;
    InitializeSynthesizer(audioModel.fs, audioModel.frame_period, audioModel.fft_size, buffer_size, 100,
                          &guard.synthesizer);

    std::vector<double> f0(audioModel.f0.begin(), audioModel.f0.begin() + static_cast<std::ptrdiff_t>(frame_count));
    auto spectrogram = audioModel.spectrogram.mutable_row_pointers();
    auto aperiodicity = audioModel.aperiodicity.mutable_row_pointers();

    std::size_t offset = 0;
    for (std::size_t i = 0; i < frame_count;) {
        if (AddParameters(f0.data() + i, 1, spectrogram.data() + i, aperiodicity.data() + i, &guard.synthesizer) == 1) {
            ++i;
        }

        while (Synthesis2(&guard.synthesizer) != 0) {
            const std::size_t index = offset * static_cast<std::size_t>(buffer_size);
            if (index >= x.size()) {
                break;
            }
            const std::size_t count = std::min(static_cast<std::size_t>(buffer_size), x.size() - index);
            for (std::size_t j = 0; j < count; ++j) {
                x[index + j] = guard.synthesizer.buffer[j];
            }
            ++offset;
        }

        if (IsLocked(&guard.synthesizer) == 1) {
            YALL_WARN_ << "Synthesis Buffer Locked";
            break;
        }
    }
}
