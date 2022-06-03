/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <sstream>

#if MSVC
#include <algorithm>
#endif

#include "Utils/exception.h"

#include "UTAUParameterPaser.h"
#include "ScaleConvert.h"

UTAUParameterPaser::UTAUParameterPaser(int argc, char *argv[]) {
    utauPara.local_name = argv[0];
    utauPara.input_file_name = argv[1];
    utauPara.output_file_name = argv[2];
    utauPara.scale_name = argv[3];
    ScaleConvert scaleConvert(utauPara.scale_name);
    utauPara.scale_num = scaleConvert.GetScaleNum();

    // velocity
    if (argc > 4) {
        std::istringstream sstream(argv[4]);
        sstream >> utauPara.time_percent;
        utauPara.velocity = pow(2, utauPara.time_percent / 100.0 - 1.0);
        sstream.clear();
    }

    if (argc > 5) {
        std::istringstream sstream(argv[5]);
        sstream >> utauPara.flags;
        sstream.clear();
    }

    // offset
    if (argc > 6) {
        std::istringstream sstream(argv[6]);
        sstream >> utauPara.offset;
        sstream.clear();
    }

    // require length
    if (argc > 7) {
        std::istringstream sstream(argv[7]);
        sstream >> utauPara.required_length;
        sstream.clear();
    }

    // fixed length
    if (argc > 8) {
        std::istringstream sstream(argv[8]);
        sstream >> utauPara.first_half_fixed_part;
        sstream.clear();
    }

    // blank length
    if (argc > 9) {
        std::istringstream sstream(argv[9]);
        sstream >> utauPara.last_unused_part;
        sstream.clear();
    }

    // volume
    if (argc > 10) {
        std::istringstream sstream(argv[10]);
        sstream >> utauPara.volumes;
        utauPara.volumes *= 0.01;
        sstream.clear();
    }

    // modulation
    if (argc > 11) {
        std::istringstream sstream(argv[11]);
        sstream >> utauPara.modulation;
        sstream.clear();
    }

    // tempo
    if (argc > 12) {
        std::istringstream sstream(argv[12]);
        sstream >> utauPara.tempo;
        utauPara.tempo_num = std::stoi(utauPara.tempo.substr(1, utauPara.tempo.length()));
        sstream.clear();
    }

    // pitchbend
    if (argc > 13) {
        utauPara.is_custom_pitch = true;
        std::istringstream sstream(argv[13]);
        sstream >> utauPara.pitch;
    }
}

UTAUPara UTAUParameterPaser::get_para() {
    return utauPara;
}