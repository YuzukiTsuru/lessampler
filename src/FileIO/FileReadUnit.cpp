//
// @Author Ghost Gloomy on 2020/1/26.
//

#include "FileReadUnit.h"

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

#include <fstream>
#include <iostream>

#if (defined (__WIN32__) || defined (_WIN32)) && !defined (__MINGW32__)
#pragma warning(disable : 4996)
#endif

int less::FileReadUnit::GetAudioLength(const char *filename) {
    SNDFILE *sf;
    SF_INFO info;
    info.format = 0;
    sf = sf_open(filename, SFM_READ, &info);
    if (sf == nullptr) {
        std::cerr << "Failed to open the file." << std::endl;
        exit(-1);
    }
    int len = info.frames;
    return len;
}

int less::FileReadUnit::WavRead(const char *FilePath, double *output) {
    SNDFILE *sf;
    SF_INFO info;
    info.format = 0;
    sf = sf_open(FilePath, SFM_READ, &info);
    if (sf == nullptr) {
        std::cerr << "Failed to open the file." << std::endl;
        exit(-1);
    }
    int f = info.frames;
    int c = info.channels;
    if (c > 1) {
        std::cerr << "Can't read stereo file for lessampler." << std::endl;
        exit(-1);
    }
    int num_items = f * c;
    auto buf = new double[num_items];
    int num = sf_read_double(sf, buf, num_items);
    sf_close(sf);
    for (int i = 0; i < num; i += c) {
        for (int j = 0; j < c; ++j) {
            if ((i + j) < f) {
                output[i + j] = buf[i + j];
            }
        }
    }
    delete[] buf;
    return info.samplerate;
}

//TODO
void less::FileReadUnit::FileToWav(const std::string &Path, double *output) {
    rapidjson::Document AudioData;
    std::ifstream filedata;
    filedata.open(Path);
    filedata.seekg(0, std::ios::end);
    int len = filedata.tellg();
    filedata.seekg(0, std::ios::beg);
    auto file = new char[len];
    filedata.read(file, len);
    filedata.close();
    AudioData.Parse(file);
    std::cout << AudioData.GetString();
}

