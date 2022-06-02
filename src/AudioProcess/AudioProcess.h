
// Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

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
    void PicthEqualizing();

    [[nodiscard]] double GetAvgFreq() const;

    void TimeStretch();

private:
    static void interp1(const double *x, const double *y, int x_length, const double *xi, int xi_length, double *yi);

    static void histc(const double *x, int x_length, const double *edges, int edges_length, int *index);
};


#endif //LESSAMPLER_AUDIOPROCESS_H
