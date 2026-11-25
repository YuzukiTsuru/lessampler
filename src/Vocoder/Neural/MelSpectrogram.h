/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_MELSPECTROGRAM_H
#define LESSAMPLER_MELSPECTROGRAM_H

#include <span>
#include <vector>

#include "Config/NeuralVocoderSpec.h"

namespace lessampler {

    /*
     * Configurable log-mel front end for neural vocoders. The defaults are
     * NHVSing: 44.1 kHz, hop 256, 2048-point STFT, 128 Slaney bins between
     * 40 Hz and 16 kHz, natural log. Set NeuralVocoderSpec for another model.
     */
    class MelSpectrogram {
    public:
        explicit MelSpectrogram(NeuralVocoderSpec spec = {});

        [[nodiscard]] const NeuralVocoderSpec &spec() const noexcept { return spec_; }

        /*
         * Returns frames * mel_bins row-major floats. The input is resampled
         * to the spec's sample rate, normalized to target_rms using active
         * (non-silent) samples, and clipped to [-1, 1] before extraction.
         */
        [[nodiscard]] std::vector<float> Extract(std::span<const double> samples, int source_rate,
                                                 double target_rms) const;

        /*
         * Windowed-sinc resampler with a low-pass cutoff below the smaller
         * Nyquist frequency. Used to bridge the model's sample rate.
         */
        [[nodiscard]] static std::vector<double> Resample(std::span<const double> samples, int from_rate,
                                                          int to_rate);

    private:
        NeuralVocoderSpec spec_;
        std::vector<double> filterbank_;

        void build_filterbank();
    };

}// namespace lessampler

#endif//LESSAMPLER_MELSPECTROGRAM_H
