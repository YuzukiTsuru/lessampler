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


#include <algorithm>
#include <cmath>
#include <cstring>
#include <utility>

#include "Shine.h"

#include "Core/Exception.h"
#include "Core/Log.h"
#include "Host/UTAU/libUTAU/PitchBendDecoder.h"

namespace lessampler {

    [[maybe_unused]] Shine::Shine(RenderParams para) : shine_para(std::move(para)) {}

    Shine::Shine(int argc, char *argv[], const AudioInfo &audioInfo, SHINE_MODE mode) {
        if (mode == SHINE_MODE::UTAU) {
            libUTAU utau(argc, argv);
            utau.CheckPara(audioInfo);
            SetShine(utau.GetUTAUPara(), audioInfo);
        }
    }

    Shine::Shine(int argc, char **argv, Shine::SHINE_MODE mode) {
        if (mode == SHINE_MODE::UTAU) {
            libUTAU utau(argc, argv);
            SetShine(utau.GetUTAUPara());
        }
    }

    Shine::Shine(std::span<const std::string_view> args, const AudioInfo &audioInfo, SHINE_MODE mode) {
        if (mode == SHINE_MODE::UTAU) {
            libUTAU utau(args);
            utau.CheckPara(audioInfo);
            SetShine(utau.GetUTAUPara(), audioInfo);
        }
    }

    void Shine::SetShine(const UTAUPara &utau_para) {
        shine_para.input_file_name = utau_para.input_file_name;
        shine_para.output_file_name = utau_para.output_file_name;
        shine_para.required_length = utau_para.required_length;
    }

    void Shine::SetShine(const UTAUPara &utau_para, const AudioInfo &audioInfo) {
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
        ParseVoiceStyleFlags(utau_para.flags, shine_para.style);
        DecodePitchBend(audioInfo.fs, audioInfo.frame_period, utau_para.pitch);
    }

    RenderParams Shine::GetShine() const {
        return shine_para;
    }

    void Shine::DecodePitchBend(int fs, double frame_period, std::string_view pitch) {
        if (shine_para.tempo_num == 0) {
            shine_para.tempo_num = 120;
        }

        shine_para.pitch_step = static_cast<int>(std::lround(60.0 / 96.0 / shine_para.tempo_num * fs));
        if (shine_para.pitch_step <= 0) {
            throw parameter_error("Invalid pitch step");
        }
        shine_para.pitch_length = shine_para.output_samples / shine_para.pitch_step + 1;

        LOG_DEBUG << "The Pitch Length is: " + std::to_string(shine_para.pitch_length);

        if (shine_para.is_custom_pitch) {
            PitchBendDecoder pitch_bend_decoder(pitch, shine_para.pitch_length + 1);
            shine_para.pitch_bend = std::move(pitch_bend_decoder.GetPitchBend());
        } else {
            shine_para.pitch_bend.assign(shine_para.pitch_length + 1, 0);
        }

        shine_para.required_frame = static_cast<int>(1000.0 * shine_para.output_samples / fs / frame_period) + 1;
        LOG_DEBUG << "The required frame is: " + std::to_string(shine_para.required_frame);
    }

}// namespace lessampler
