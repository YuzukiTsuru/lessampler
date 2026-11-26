/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Audio/Dsp/Fft.h"

#include <numbers>
#include <stdexcept>
#include <string>

namespace lessampler {

    Fft::Fft(std::size_t size) : size_(size) {
        if (size_ < 2 || (size_ & (size_ - 1)) != 0) {
            throw std::invalid_argument("FFT size must be a power of two");
        }

        std::size_t bits = 0;
        while ((std::size_t{1} << bits) < size_) {
            ++bits;
        }
        bit_reverse_.resize(size_);
        for (std::size_t i = 0; i < size_; ++i) {
            std::size_t value = i;
            std::size_t reversed = 0;
            for (std::size_t bit = 0; bit < bits; ++bit) {
                reversed = (reversed << 1) | (value & 1);
                value >>= 1;
            }
            bit_reverse_[i] = reversed;
        }

        twiddle_.resize(size_ / 2);
        for (std::size_t i = 0; i < twiddle_.size(); ++i) {
            const double angle = -2.0 * std::numbers::pi * static_cast<double>(i) / static_cast<double>(size_);
            twiddle_[i] = {std::cos(angle), std::sin(angle)};
        }
    }

    void Fft::Forward(std::span<std::complex<double>> data) const {
        Transform(data, false);
    }

    void Fft::Inverse(std::span<std::complex<double>> data) const {
        Transform(data, true);
        const double scale = 1.0 / static_cast<double>(size_);
        for (auto &value: data) {
            value *= scale;
        }
    }

    void Fft::Transform(std::span<std::complex<double>> data, bool inverse) const {
        if (data.size() != size_) {
            throw std::invalid_argument("FFT data size does not match the plan");
        }

        for (std::size_t i = 0; i < size_; ++i) {
            if (i < bit_reverse_[i]) {
                std::swap(data[i], data[bit_reverse_[i]]);
            }
        }

        for (std::size_t length = 2; length <= size_; length <<= 1) {
            const std::size_t half = length / 2;
            const std::size_t step = size_ / length;
            for (std::size_t start = 0; start < size_; start += length) {
                for (std::size_t i = 0; i < half; ++i) {
                    const auto twiddle = inverse ? std::conj(twiddle_[i * step]) : twiddle_[i * step];
                    const auto even = data[start + i];
                    const auto odd = data[start + i + half] * twiddle;
                    data[start + i] = even + odd;
                    data[start + i + half] = even - odd;
                }
            }
        }
    }

}// namespace lessampler
