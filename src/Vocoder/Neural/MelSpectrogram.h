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

namespace lessampler {

    /*
     * Feature front end for NHVSing-compatible neural vocoders.
     *
     * The ONNX model expects 44.1 kHz, hop 256, 2048-point STFT, and 128
     * Slaney-normalized mel bins between 40 Hz and 16 kHz, taking the natural
     * log of the magnitude mel. This mirrors NHVSing's preprocessing exactly.
     */
    class MelSpectrogram {
    public:
        static constexpr int sample_rate = 44100;
        static constexpr int hop_size = 256;
        static constexpr int fft_size = 2048;
        static constexpr int mel_bins = 128;
        static constexpr double mel_min = 40.0;
        static constexpr double mel_max = 16000.0;

        MelSpectrogram();

        /*
         * Returns frames * mel_bins row-major floats. The input is resampled
         * to 44.1 kHz, normalized to target_rms using active (non-silent)
         * samples, and clipped to [-1, 1] before feature extraction.
         */
        [[nodiscard]] std::vector<float> Extract(std::span<const double> samples, int source_rate,
                                                 double target_rms) const;

        /*
         * Windowed-sinc resampler with a low-pass cutoff below the smaller
         * Nyquist frequency. Used to bridge the model's fixed 44.1 kHz rate.
         */
        [[nodiscard]] static std::vector<double> Resample(std::span<const double> samples, int from_rate,
                                                          int to_rate);

    private:
        std::vector<double> filterbank_;

        void build_filterbank();
    };

}// namespace lessampler

#endif//LESSAMPLER_MELSPECTROGRAM_H
