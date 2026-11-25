/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Vocoder/World/WorldSynthesizer.h"

#include <algorithm>
#include <vector>

#include <world/synthesisrealtime.h>

#include "Core/Exception.h"
#include "Core/Log.h"

namespace lessampler {

    namespace {

        struct WorldSynthesizerGuard {
            WorldSynthesizer synthesizer{};

            ~WorldSynthesizerGuard() { DestroySynthesizer(&synthesizer); }
        };

    }// namespace

    std::vector<double> SynthesizeWorld(const WorldModel &model, int output_samples) {
        if (model.frame_period <= 0.0 || model.fft_size <= 0 || model.fs <= 0) {
            throw parameter_error("Invalid audio model parameters for synthesis");
        }
        if (output_samples <= 0) {
            return {};
        }

        const std::size_t frame_count = std::min({model.f0.size(),
                                                  model.spectrogram.rows(),
                                                  model.aperiodicity.rows()});
        if (frame_count == 0) {
            return {};
        }

        WorldSynthesizerGuard guard;
        constexpr int buffer_size = 64;
        InitializeSynthesizer(model.fs, model.frame_period, model.fft_size, buffer_size, 100,
                              &guard.synthesizer);

        std::vector<double> f0(model.f0.begin(), model.f0.begin() + static_cast<std::ptrdiff_t>(frame_count));
        auto spectrogram = model.spectrogram.mutable_row_pointers();
        auto aperiodicity = model.aperiodicity.mutable_row_pointers();
        std::vector<double> output(static_cast<std::size_t>(output_samples), 0.0);

        std::size_t offset = 0;
        const auto drain = [&]() {
            while (Synthesis2(&guard.synthesizer) != 0) {
                const std::size_t index = offset * static_cast<std::size_t>(buffer_size);
                if (index < output.size()) {
                    const std::size_t count = std::min(static_cast<std::size_t>(buffer_size), output.size() - index);
                    for (std::size_t j = 0; j < count; ++j) {
                        output[index + j] = guard.synthesizer.buffer[j];
                    }
                }
                ++offset;
            }
        };

        for (std::size_t i = 0; i < frame_count;) {
            if (AddParameters(f0.data() + i, 1, spectrogram.data() + i, aperiodicity.data() + i, &guard.synthesizer) == 1) {
                ++i;
            }

            drain();

            if (IsLocked(&guard.synthesizer) == 1) {
                LOG_WARN << "Synthesis Buffer Locked";
                break;
            }
        }

        // WORLD renders the interval of frame k-1 when frame k is added, so
        // the final frame interval is never generated. Add the last frame once
        // more and drain; without this every note ends with an exact-zero tail
        // of up to one frame period.
        if (IsLocked(&guard.synthesizer) == 0 && frame_count > 0) {
            drain();
            const auto last = frame_count - 1;
            if (AddParameters(f0.data() + last, 1, spectrogram.data() + last,
                              aperiodicity.data() + last, &guard.synthesizer) == 1) {
                drain();
            }
        }

        return output;
    }

}// namespace lessampler
