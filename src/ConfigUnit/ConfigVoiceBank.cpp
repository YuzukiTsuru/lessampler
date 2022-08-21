/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

//
// Created by gloom on 2022/8/21.
//

#include <utility>

#include "StaticCast.h"
#include "ConfigVoiceBank.h"
#include "Utils/exception.h"

ConfigVoiceBank::ConfigVoiceBank(std::filesystem::path _voice_path, lessConfigure _configure) : voice_path(std::move(_voice_path)),
                                                                                                configure(std::move(_configure)) {
    SetVoiceConfig();
}

ConfigVoiceBank::ConfigVoiceBank(lessConfigure _configure) : configure(std::move(_configure)) {
    SetVoiceConfig();
}

void ConfigVoiceBank::SetVoiceConfig() {
    voice_config_path = voice_path / VOIICEBANKCONFIGFILENAME;
    if (voice_config_path.empty()) {
        throw file_open_error("Configure file: " + voice_config_path.string());
    }
}
