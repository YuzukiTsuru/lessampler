/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LESSAMPLER_APPLICATION_H
#define LESSAMPLER_APPLICATION_H

#include <filesystem>

#include "Config/Config.h"

namespace lessampler {

    /*
 * CLI host. Parses the process arguments, selects a command, and keeps the
 * rendering engine independent of argument parsing.
 */
    class Application {
    public:
        Application(int argc, char **argv);

        [[nodiscard]] int run();

    private:
        enum class Command {
            none,
            help,
            version,
            usage,
            wavtool,
            wavtool_plan,
            wavtool_usage,
            model_voicebank,
            render_note,
            polish,
            feature_plan,
            f0_dump,
        };

        [[nodiscard]] Command parse_args() const;

        void load_config();

        void render_note();

        static void render_blank(int argc, char **argv);

        int run_wavtool();

        int run_wavtool_plan();

        void model_voicebank();

        void polish_audio();

        void feature_plan();

        void f0_dump();

        static void show_logo();

        static void show_usage();

    private:
        int argc_;
        char **argv_;
        std::filesystem::path exec_path_;
        std::filesystem::path input_path_;
        Config configure_;
    };

}// namespace lessampler

#endif//LESSAMPLER_APPLICATION_H
