/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_VOICEBANKMODELER_H
#define LESSAMPLER_VOICEBANKMODELER_H

#include <filesystem>
#include <vector>

#include "Config/Config.h"
#include "Config/ConfigResolver.h"

namespace lessampler {

    /*
 * Batch host for building .lessaudio sidecars. Uses the same analysis path
 * and per-voicebank config resolution as note rendering.
 */
    class VoicebankModeler {
    public:
        VoicebankModeler(const std::filesystem::path &path, Config configure);

        void PrintWavFiles() const;

    private:
        std::filesystem::path target_voice_path_;
        std::vector<std::filesystem::path> wav_files_;
        ConfigResolver configs_;

    private:
        void GetWavFileLists();

        void GenerateModelFromFile();
    };

}// namespace lessampler

#endif//LESSAMPLER_VOICEBANKMODELER_H
