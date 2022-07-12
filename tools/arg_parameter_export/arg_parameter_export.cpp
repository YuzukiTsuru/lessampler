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
// Created by gloom on 2022/7/10.
//

#include <fstream>
#include <filesystem>

int main(int argc, char *argv[]) {
    if (argc < 2)
        return 0;
    auto file_path = std::filesystem::weakly_canonical(std::filesystem::path(argv[0]));
    std::ofstream file(file_path.replace_extension("log"), std::ios::out | std::ios::app);

    file << "==========================" << std::endl;

    for (int i = 0; i < argc; ++i) {
        file << argv[i] << std::endl;
    }
    file.close();
}