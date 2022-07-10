/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_UTAUPARAMETERPASER_H
#define LESSAMPLER_UTAUPARAMETERPASER_H

#include <iostream>
#include "libUTAU.h"
#include "AudioModel/lessAudioModel.h"

class UTAUParameterPaser {
public:
    UTAUParameterPaser(int argc, char *argv[]);

    UTAUPara get_para();

private:
    UTAUPara utauPara;
};


#endif //LESSAMPLER_UTAUPARAMETERPASER_H
