/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_DSP_FFT_H
#define LESSAMPLER_DSP_FFT_H

#include <complex>
#include <cstddef>
#include <span>
#include <vector>

namespace lessampler {

    /*
     * Small in-place radix-2 FFT used by the source denoiser. The size must be
     * a power of two. Twiddles and the bit-reversal permutation are computed
     * once per instance.
     */
    class Fft {
    public:
        explicit Fft(std::size_t size);

        [[nodiscard]] std::size_t size() const noexcept { return size_; }

        void Forward(std::span<std::complex<double>> data) const;
        void Inverse(std::span<std::complex<double>> data) const;

    private:
        void Transform(std::span<std::complex<double>> data, bool inverse) const;

        std::size_t size_ = 0;
        std::vector<std::size_t> bit_reverse_;
        std::vector<std::complex<double>> twiddle_;
    };

}// namespace lessampler

#endif//LESSAMPLER_DSP_FFT_H
