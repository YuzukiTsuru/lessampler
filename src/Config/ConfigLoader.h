/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_CONFIGUNIT_H
#define LESSAMPLER_CONFIGUNIT_H

#include <filesystem>

#include "Config.h"

namespace lessampler {

    class ConfigLoader {
    public:
        explicit ConfigLoader(const std::filesystem::path &exec_path);

        void SetConfig(const std::filesystem::path &exec_path);

        ~ConfigLoader();

        [[nodiscard]] Config GetConfig() const;

    private:
        std::filesystem::path config_file_path;
        Config configure;

    private:
        void load_or_create();

        void migrate_legacy_ini();

        void migrate_legacy_config(const std::filesystem::path &source_path);

        static void resolve_neural_model_path(Config &configure, const std::filesystem::path &base);
    };

}// namespace lessampler

#endif//LESSAMPLER_CONFIGUNIT_H
