/*
 * Copyright (c)  2022, YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <sstream>

#if MSVC
#include <algorithm>
#endif

#include "UTAUParameterPaser.h"
#include "ScaleConvert.h"
#include "UTAUParameterPaser.h"

UTAUParameterPaser::UTAUParameterPaser(int argc, char *argv[]) {
    utauPara.localName = argv[0];
    utauPara.inputFileName = argv[1];
    utauPara.outputFileName = argv[2];
    utauPara.scaleName = argv[3];
    ScaleConvert scaleConvert(utauPara.scaleName);
    utauPara.scaleNum = scaleConvert.GetScaleNum();

    // velocity
    if (argc > 3) {
        std::istringstream sstream(argv[4]);
        sstream >> utauPara.timePercent;
        utauPara.velocity = pow(2, utauPara.timePercent / 100.0 - 1.0);
        sstream.clear();
    }

    if (argc > 4) {
        std::istringstream sstream(argv[5]);
        sstream >> utauPara.flags;
        sstream.clear();
    }

    // offset
    if (argc > 5) {
        std::istringstream sstream(argv[6]);
        sstream >> utauPara.offset;
        sstream.clear();
    }

    // require length
    if (argc > 6) {
        std::istringstream sstream(argv[7]);
        sstream >> utauPara.requiredLength;
        sstream.clear();
    }

    // fixed length
    if (argc > 7) {
        std::istringstream sstream(argv[8]);
        sstream >> utauPara.firstHalfFixedPart;
        sstream.clear();
    }

    // blank length
    if (argc > 8) {
        std::istringstream sstream(argv[9]);
        sstream >> utauPara.lastUnusedPart;
        sstream.clear();
    }

    // volume
    if (argc > 9) {
        std::istringstream sstream(argv[10]);
        sstream >> utauPara.volumes;
        utauPara.volumes *= 0.01;
        sstream.clear();
    }

    // modulation
    if (argc > 10) {
        std::istringstream sstream(argv[11]);
        sstream >> utauPara.modulation;
        sstream.clear();
    }

    // tempo
    if (argc > 11) {
        std::istringstream sstream(argv[12]);
        sstream >> utauPara.tempo;
        utauPara.tempoNum = std::stoi(utauPara.tempo.substr(1, utauPara.tempo.length()));
        sstream.clear();
    }

    // pitchbend
    if (argc > 12) {
        std::istringstream sstream(argv[13]);
        sstream >> utauPara.pitch;
    }
}

UTAUPara UTAUParameterPaser::get_para() {
    return utauPara;
}
