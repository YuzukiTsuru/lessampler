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

#include "Shine.h"

#include <utility>

Shine::Shine(const UTAUPara& utau_para, UTAUFlags utau_flags) {
    SetShine(utau_para, utau_flags);
}

ShinePara Shine::GetShine() {
    return shine_para;
}

void Shine::SetShine(const UTAUPara& utau_para, UTAUFlags utau_flags) {
    shine_para.input_file_name = utau_para.inputFileName;
}

