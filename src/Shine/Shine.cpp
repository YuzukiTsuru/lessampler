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

#include <utility>
#include <cstring>
#include <cmath>

#include "Shine.h"

#include "Utils/LOG.h"
#include "Binding/libUTAU/PitchBendDecoder.h"

[[maybe_unused]] Shine::Shine(ShinePara para) : shine_para(std::move(para)) {}

Shine::Shine(int argc, char *argv[], lessAudioModel audioModel, SHINE_MODE mode) {
    if (mode == SHINE_MODE::UTAU) {
        libUTAU utau(argc, argv);
        utau.CheckPara(audioModel);
        Shine(utau.GetUTAUPara(), utau.GetUTAUFlags(), audioModel);
    }
}

[[maybe_unused]] Shine::Shine(const UTAUPara &utau_para, UTAUFlags utau_flags, lessAudioModel audioModel) {
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
    DecodePitchBend(audioModel.fs, audioModel.frame_period, utau_para.pitch);
}

ShinePara Shine::GetShine() {
    return shine_para;
}

void Shine::DecodePitchBend(int fs, double frame_period, std::string pitch) {
    if (shine_para.tempo_num == 0)
        shine_para.tempo_num = 120;

    if (shine_para.is_custom_pitch) {
        shine_para.pitch_step = static_cast<int>(lround(60.0 / 96.0 / shine_para.tempo_num * fs));
        shine_para.pitch_length = shine_para.output_samples / shine_para.pitch_step + 1;

        YALL_DEBUG_ << "The Pitch Length is: " + std::to_string(shine_para.pitch_length);

        PitchBendDecoder pitchBendDecoder(pitch, shine_para.pitch_length);

        shine_para.pitch_bend = new int[shine_para.pitch_length + 1];
        for (int i = 0; i < shine_para.pitch_length + 1; ++i) {
            shine_para.pitch_bend[i] = 0;
        }
        std::memcpy(shine_para.pitch_bend, pitchBendDecoder.getPitchBend(), sizeof(int) * shine_para.pitch_length);
    } else {
        shine_para.pitch_bend = new int[shine_para.pitch_length + 1];
        for (int i = 0; i < shine_para.pitch_length + 1; ++i) {
            shine_para.pitch_bend[i] = 0;
        }
    }

    shine_para.required_frame = static_cast<int>(1000.0 * shine_para.output_samples / fs / frame_period) + 1;
    YALL_DEBUG_ << "The required frame is: " + std::to_string(shine_para.required_frame);
}


