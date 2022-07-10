
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
// Created by gloom on 2022/5/7.
//

#ifndef LESSAMPLER_AUDIOPROCESS_H
#define LESSAMPLER_AUDIOPROCESS_H

#include "AudioModel/lessAudioModel.h"
#include "Shine/ShinePara.h"

class AudioProcess {
public:
    AudioProcess(lessAudioModel audioModel, ShinePara shine);

    lessAudioModel GetTransAudioModel();

private:
    lessAudioModel audioModel{};
    lessAudioModel transAudioModel{};
    ShinePara shine;

private:
    void InitTransAudioModel();

    void PicthEqualizing();

    [[nodiscard]] double GetAvgFreq() const;

    void TimeStretch();

private:
    [[maybe_unused]] static void interp1(const double *x, const double *y, int x_length, const double *xi, int xi_length, double *yi);

    [[maybe_unused]] static void histc(const double *x, int x_length, const double *edges, int edges_length, int *index);
};


#endif //LESSAMPLER_AUDIOPROCESS_H
