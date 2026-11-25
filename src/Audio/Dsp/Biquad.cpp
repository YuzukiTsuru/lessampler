/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Audio/Dsp/Biquad.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace lessampler {

    void Biquad::SetCoefficients(double b0, double b1, double b2, double a0, double a1, double a2) {
        if (std::abs(a0) < 1e-15) {
            b0_ = 1.0;
            b1_ = b2_ = a1_ = a2_ = 0.0;
            return;
        }
        b0_ = b0 / a0;
        b1_ = b1 / a0;
        b2_ = b2 / a0;
        a1_ = a1 / a0;
        a2_ = a2 / a0;
    }

    void Biquad::SetHighPass(int sample_rate, double frequency_hz, double q) {
        if (sample_rate <= 0 || frequency_hz <= 0.0 || frequency_hz >= static_cast<double>(sample_rate) / 2.0) {
            SetCoefficients(1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
            return;
        }
        const double omega = 2.0 * std::numbers::pi * frequency_hz / static_cast<double>(sample_rate);
        const double cosine = std::cos(omega);
        const double alpha = std::sin(omega) / (2.0 * std::max(q, 0.01));
        const double a0 = 1.0 + alpha;
        SetCoefficients((1.0 + cosine) / 2.0, -(1.0 + cosine), (1.0 + cosine) / 2.0,
                        a0, -2.0 * cosine, 1.0 - alpha);
    }

    void Biquad::SetLowShelf(int sample_rate, double frequency_hz, double gain_db) {
        if (sample_rate <= 0 || frequency_hz <= 0.0 || frequency_hz >= static_cast<double>(sample_rate) / 2.0 ||
            std::abs(gain_db) < 1e-9) {
            SetCoefficients(1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
            return;
        }
        const double a = std::pow(10.0, gain_db / 40.0);
        const double omega = 2.0 * std::numbers::pi * frequency_hz / static_cast<double>(sample_rate);
        const double cosine = std::cos(omega);
        const double sine = std::sin(omega);
        const double alpha = sine / 2.0 * std::sqrt(2.0);
        const double two_sqrt_a_alpha = 2.0 * std::sqrt(a) * alpha;
        const double a0 = (a + 1.0) + (a - 1.0) * cosine + two_sqrt_a_alpha;
        SetCoefficients(
                a * ((a + 1.0) - (a - 1.0) * cosine + two_sqrt_a_alpha),
                2.0 * a * ((a - 1.0) - (a + 1.0) * cosine),
                a * ((a + 1.0) - (a - 1.0) * cosine - two_sqrt_a_alpha),
                a0,
                -2.0 * ((a - 1.0) + (a + 1.0) * cosine),
                (a + 1.0) + (a - 1.0) * cosine - two_sqrt_a_alpha);
    }

    void Biquad::SetPeaking(int sample_rate, double frequency_hz, double gain_db, double q) {
        if (sample_rate <= 0 || frequency_hz <= 0.0 || frequency_hz >= static_cast<double>(sample_rate) / 2.0 ||
            std::abs(gain_db) < 1e-9) {
            SetCoefficients(1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
            return;
        }
        const double a = std::pow(10.0, gain_db / 40.0);
        const double omega = 2.0 * std::numbers::pi * frequency_hz / static_cast<double>(sample_rate);
        const double alpha = std::sin(omega) / (2.0 * std::max(q, 0.01));
        const double a0 = 1.0 + alpha / a;
        SetCoefficients(1.0 + alpha * a, -2.0 * std::cos(omega), 1.0 - alpha * a,
                        a0, -2.0 * std::cos(omega), 1.0 - alpha / a);
    }

    void Biquad::Reset() {
        z1_ = 0.0;
        z2_ = 0.0;
    }

    double Biquad::Process(double input) {
        const double output = b0_ * input + z1_;
        z1_ = b1_ * input - a1_ * output + z2_;
        z2_ = b2_ * input - a2_ * output;
        return output;
    }

}// namespace lessampler
