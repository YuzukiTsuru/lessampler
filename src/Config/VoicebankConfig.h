/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_CONFIGVOICEBANK_H
#define LESSAMPLER_CONFIGVOICEBANK_H

#include <filesystem>

#include "Config.h"

namespace lessampler {

    class VoicebankConfig {
    public:
        VoicebankConfig(const std::filesystem::path &voice_file_path, Config configure);

        explicit VoicebankConfig(Config configure);

        void SetVoiceConfig();

        [[nodiscard]] Config GetConfig() const;

    private:
        Config configure;
        std::filesystem::path voice_path;
        std::filesystem::path voice_config_path;

    private:
        void parse_config();
    };

}// namespace lessampler

#endif//LESSAMPLER_CONFIGVOICEBANK_H
