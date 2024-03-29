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
// Created by gloom on 2022/5/30.
//

#ifndef LESSAMPLER_SHINE_H
#define LESSAMPLER_SHINE_H

#include "ShinePara.h"

#include "Binding/libUTAU/libUTAU.h"
#include "AudioModel/lessAudioModel.h"

class Shine {
public:
    enum SHINE_MODE{
        UTAU,
    };
public:
    [[maybe_unused]] explicit Shine(ShinePara para);

    /*
     * Shine only for blank output
     */
    [[maybe_unused]] Shine(int argc, char *argv[], SHINE_MODE mode);

    [[maybe_unused]] Shine(int argc, char *argv[], const lessAudioModel& audioModel, SHINE_MODE mode);

    ~Shine() = default;

    [[maybe_unused]] ShinePara GetShine();

private:
    ShinePara shine_para;

private:
    void SetShine(const UTAUPara& utau_para, UTAUFlags utau_flags, const lessAudioModel& audioModel);

    void SetShine(const UTAUPara& utau_para);

    void DecodePitchBend(int fs, double frame_period, std::string pitch);
};


#endif //LESSAMPLER_SHINE_H
