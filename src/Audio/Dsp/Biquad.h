/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_DSP_BIQUAD_H
#define LESSAMPLER_DSP_BIQUAD_H

namespace lessampler {

    /*
     * RBJ cookbook biquad, transposed direct form II. One instance per
     * channel; coefficients are shared by copying the same configured object.
     */
    class Biquad {
    public:
        void SetHighPass(int sample_rate, double frequency_hz, double q = 0.7071067811865476);
        void SetLowShelf(int sample_rate, double frequency_hz, double gain_db);
        void SetPeaking(int sample_rate, double frequency_hz, double gain_db, double q = 1.0);
        void Reset();

        [[nodiscard]] double Process(double input);

    private:
        void SetCoefficients(double b0, double b1, double b2, double a0, double a1, double a2);

        double b0_ = 1.0;
        double b1_ = 0.0;
        double b2_ = 0.0;
        double a1_ = 0.0;
        double a2_ = 0.0;
        double z1_ = 0.0;
        double z2_ = 0.0;
    };

}// namespace lessampler

#endif//LESSAMPLER_DSP_BIQUAD_H
