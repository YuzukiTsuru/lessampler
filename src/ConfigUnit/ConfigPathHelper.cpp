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
// Created by gloom on 2022/8/7.
//

#include "ConfigPathHelper.h"

#include "StaticCast.h"

std::filesystem::path ConfigPathHelper::GetConfigPath() {
    return exec_config_path;
}

void ConfigPathHelper::SetExecPath(const std::filesystem::path &execPath) {
    exec_path = execPath;
    exec_config_path = exec_path / CONFIGFILENAME;
    if (std::filesystem::exists(exec_config_path)) {
        is_exec = true;
    } else {
        is_exec = false;
    }
}

void ConfigPathHelper::SetVoiceBankPath(const std::filesystem::path &VBPath) {
    voice_bank_path = VBPath;
    voice_bank_config_path = voice_bank_path / VOIICEBANKCONFIGFILENAME;
    if (std::filesystem::exists(voice_bank_config_path)) {
        is_voicebank = true;
    } else {
        is_voicebank = false;
    }
}
