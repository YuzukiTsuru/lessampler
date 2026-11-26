/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_PREPROCESSOPTIONS_H
#define LESSAMPLER_PREPROCESSOPTIONS_H

namespace lessampler {

    /*
     * Source cleanup applied after WAV decode and before analysis. The
     * defaults keep the preprocessor off so existing analyses stay
     * bit-identical and existing caches remain valid.
     */
    struct PreprocessOptions {
        bool enabled = false;
        double highpass_hz = 70.0;
        bool declick = true;
        double declick_threshold = 12.0;
        bool denoise = true;
        double denoise_db = 12.0;
    };

}// namespace lessampler

#endif//LESSAMPLER_PREPROCESSOPTIONS_H
