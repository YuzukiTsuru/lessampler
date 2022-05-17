// Copyright (c) 2022. YuzukiTsuru <GloomyGhost@GloomyGhost.com>.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//
// Created by Ghost Gloomy on 2020/8/27.
//

#include <fstream>
#include <iostream>
#include <utility>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include "Utils/exception.h"
#include "JSONFileIO.h"

#if (defined (__WIN32__) || defined (_WIN32)) && !defined (__MINGW32__)
#pragma warning(disable : 4996)
#endif

JSONFileIO::JSONFileIO(lessAudioModel audioModel) : _audioModel(audioModel) {}

void JSONFileIO::SaveFile(std::string path) {
    Path = std::move(path);
    SaveJsonModel();
}

void JSONFileIO::SaveJsonModel() {
    rapidjson::StringBuffer s;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);
    writer.StartObject();
    writer.Key("NODE");
    writer.String("LESS_F0_DOUBLE");
    writer.Key("F0LEN");
    writer.Int(_audioModel.f0_length);
    writer.Key("F0");
    writer.StartArray();
    for (int i = 0; i < _audioModel.f0_length; ++i) {
        writer.Double(_audioModel.f0[i]);
    }
    writer.EndArray();
    writer.Key("FFTSIZE");
    writer.Int(_audioModel.fft_size);
    writer.Key("F0LEN");
    writer.Int(_audioModel.f0_length);
    writer.Key("SEQ");
    writer.StartArray();
    for (int i = 0; i < _audioModel.f0_length; ++i) {
        writer.StartArray();
        for (int j = 0; j < _audioModel.fft_size / 2 + 1; ++j) {
            writer.Double(_audioModel.spectrogram[i][j]);
        }
        writer.EndArray();
    }
    writer.EndArray();
    writer.Key("AP");
    writer.StartArray();
    for (int i = 0; i < _audioModel.f0_length; ++i) {
        writer.StartArray();
        for (int j = 0; j < _audioModel.fft_size / 2 + 1; ++j) {
            writer.Double(_audioModel.aperiodicity[i][j]);
        }
        writer.EndArray();
    }
    writer.EndArray();
    writer.EndObject();
    std::ofstream fout(Path, std::ios::out);
    if (!fout)
        throw file_open_error(Path);
    fout << s.GetString();
    fout.close();
}