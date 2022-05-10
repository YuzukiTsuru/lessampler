
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

#ifndef LESSAMPLER_TIMESTRETCH_H
#define LESSAMPLER_TIMESTRETCH_H

// TODO: Add baisc fix part diff

//伸縮の概念図
//  offset    fixed      m2      blank
//|--------|--------|---------|---------| 原音
//         |        |          |
//         |   l1   |    l2     |
//         |--------|------------|  出力
// l1  = fixed / velocity
// l2  = m2    / stretch
// l1 + l2  = 要求長＝argv[7]

#include "AudioModel/lessAudioModel.h"

class TimeStretch {
    explicit TimeStretch(lessAudioModel audioModel);

private:
    lessAudioModel audioModel;
};


#endif //LESSAMPLER_TIMESTRETCH_H
