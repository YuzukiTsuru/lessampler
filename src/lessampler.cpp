/*
 * Copyright (c)  2022, YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "lessampler.h"
#include "config.h"

lessampler::lessampler(int argc, char **argv) {
    this->argc = argc;
    this->argv = argv;
}

void lessampler::run() {
    if (argc < 2) {
        show_logo();
        Dialogs::notify("lessampler", "lessampler: no input file");
        return;
    }
}

void lessampler::show_logo() {
    std::cout << " _                           _         \n"
                 "| |___ ___ ___ ___ _____ ___| |___ ___ \n"
                 "| | -_|_ -|_ -| .'|     | . | | -_|  _|\n"
                 "|_|___|___|___|__,|_|_|_|  _|_|___|_|  \n"
                 " Version: " << PROJECT_GIT_HASH
                 << "       |_|            "
              << std::endl;

}

