/*
 * Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
 * lessampler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License v3.0 as published by
 *  the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * You should have received a copy of the GNU Lesser General Public License v3.0
 * along with lessampler. If not, see <http://www.gnu.org/licenses/>.
 */

#include <lessampler.h>

int main(int argc, char *argv[]) {
    try {
        lessampler lessampler(argc, argv);
        lessampler.run();
    } catch (const std::runtime_error &error) {
        YALL_ERROR_ << error.what();
    }
}
