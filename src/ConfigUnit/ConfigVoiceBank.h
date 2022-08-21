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

#ifndef LESSAMPLER_CONFIGVOICEBANK_H
#define LESSAMPLER_CONFIGVOICEBANK_H

#include <filesystem>
#include <iostream>

#include "lessConfigure.h"

class ConfigVoiceBank {
public:
    ConfigVoiceBank(std::filesystem::path _voice_path, lessConfigure _configure);

    explicit ConfigVoiceBank(lessConfigure _configure);

    void SetVoiceConfig();

private:
    lessConfigure configure;
    std::filesystem::path voice_path;
    std::filesystem::path voice_config_path;

private:


};


#endif //LESSAMPLER_CONFIGVOICEBANK_H
