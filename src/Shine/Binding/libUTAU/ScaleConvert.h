/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_SCALECONVERT_H
#define LESSAMPLER_SCALECONVERT_H

#include <cmath>
#include <iostream>

class ScaleConvert {
public:
    explicit ScaleConvert(std::string scaleName);

    [[nodiscard]] double GetScaleNum() const;

private:
    double scaleNum = 0.0;
    /*
     * Scale Name Convert to number than double
     * IN: std::string scale_name
     * RET: bool
     *   true -> success
     *   false -> fail
     * OUT: double scale_num
     */
    bool ScaleConvertToDouble(std::string scaleName);
};


#endif //LESSAMPLER_SCALECONVERT_H
