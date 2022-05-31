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

Shine::Shine(ShinePara para): shine_para(std::move(para)) {}

Shine::Shine(const UTAUPara &utau_para, UTAUFlags utau_flags) {
    SetShine(utau_para, utau_flags);
}

ShinePara Shine::GetShine() {
    return shine_para;
}


void Shine::SetShine(const UTAUPara &utau_para, UTAUFlags utau_flags) {
    shine_para.input_file_name = utau_para.input_file_name;
    shine_para.output_file_name = utau_para.output_file_name;
    shine_para.time_percent = utau_para.time_percent;
    shine_para.velocity = utau_para.velocity;
    shine_para.offset = utau_para.offset;
    shine_para.required_length = utau_para.required_length;
    shine_para.first_half_fixed_part = utau_para.first_half_fixed_part;
    shine_para.last_unused_part = utau_para.last_unused_part;
    shine_para.volumes = utau_para.volumes;
    shine_para.modulation = utau_para.modulation;
    shine_para.wave_length = utau_para.wave_length;
    shine_para.pre_cross_length = utau_para.pre_cross_length;
    shine_para.base_length = utau_para.base_length;
    shine_para.cross_length = utau_para.cross_length;
    shine_para.stretch_length = utau_para.stretch_length;
    shine_para.output_samples = utau_para.output_samples;
    shine_para.scale_num = utau_para.scale_num;
    shine_para.tempo_num = utau_para.tempo_num;
    shine_para.is_custom_pitch = utau_para.is_custom_pitch;
}


