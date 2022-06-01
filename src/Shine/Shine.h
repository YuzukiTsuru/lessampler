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
// Created by gloom on 2022/5/30.
//

#ifndef LESSAMPLER_SHINE_H
#define LESSAMPLER_SHINE_H

#include "ShinePara.h"

#include "Binding/libUTAU/libUTAU.h"
#include "AudioModel/lessAudioModel.h"

class Shine {
public:
    [[maybe_unused]] explicit Shine(ShinePara para);

    [[maybe_unused]] Shine(const UTAUPara &utau_para, UTAUFlags utau_flags, lessAudioModel audioModel);

    ~Shine() = default;

    [[maybe_unused]] ShinePara GetShine();

private:
    ShinePara shine_para;

private:
    void DecodePitchBend(int fs, double frame_period, std::string pitch);
};


#endif //LESSAMPLER_SHINE_H
