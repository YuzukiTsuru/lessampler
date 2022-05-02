//
// Created by Ghost Gloomy on 2020/8/27.
//

#include <fstream>
#include <iostream>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include "FileWriteUnit.h"

#if (defined (__WIN32__) || defined (_WIN32)) && !defined (__MINGW32__)
#pragma warning(disable : 4996)
#endif

void less::FileWriteUnit::WavWrite(const double *x, int x_length, int fs, const char *filename) {
    SNDFILE *sf;
    SF_INFO info;
    info.format = (SF_FORMAT_WAV | SF_FORMAT_PCM_16);
    info.samplerate = fs;
    info.channels = 1;
    sf = sf_open(filename, SFM_WRITE, &info);
    if (sf == nullptr) {
        std::cerr << "Failed to write the file." << std::endl;
        exit(-1);
    }
    sf_write_double(sf, x, x_length);
    sf_close(sf);
}

void less::FileWriteUnit::F0ToFile(const std::string &Path, double *F0, int F0Len) {
    rapidjson::StringBuffer s;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);
    writer.StartObject();
    writer.Key("NODE");
    writer.String("LESS_F0_DOUBLE");
    writer.Key("F0LEN");
    writer.Int(F0Len);
    writer.Key("F0");
    writer.StartArray();
    for (int i = 0; i < F0Len; ++i) {
        writer.Double(F0[i]);
    }
    writer.EndArray();
    writer.EndObject();
    std::ofstream fout(Path);
    fout << s.GetString();
    fout.close();
}

void less::FileWriteUnit::SpToFile(const std::string &Path, double **Spec, int fft_size, int f0_length) {
    rapidjson::StringBuffer s;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);
    writer.StartObject();
    writer.Key("FFTSIZE");
    writer.Int(fft_size);
    writer.Key("F0LEN");
    writer.Int(f0_length);
    writer.Key("SEQ");
    writer.StartArray();
    for (int i = 0; i < f0_length; ++i) {
        writer.StartArray();
        for (int j = 0; j < fft_size / 2 + 1; ++j) {
            writer.Double(Spec[i][j]);
        }
        writer.EndArray();
    }
    writer.EndArray();
    writer.EndObject();
    std::ofstream fout(Path);
    fout << s.GetString();
    fout.close();
}

void less::FileWriteUnit::ApToFile(const std::string &Path, double **ap, int fft_size, int f0_length) {
    rapidjson::StringBuffer s;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);
    writer.StartObject();
    writer.Key("FFTSIZE");
    writer.Int(fft_size);
    writer.Key("F0LEN");
    writer.Int(f0_length);
    writer.Key("AP");
    writer.StartArray();
    for (int i = 0; i < f0_length; ++i) {
        writer.StartArray();
        for (int j = 0; j < fft_size / 2 + 1; ++j) {
            writer.Double(ap[i][j]);
        }
        writer.EndArray();
    }
    writer.EndArray();
    writer.EndObject();
    std::ofstream fout(Path);
    fout << s.GetString();
    fout.close();
}

void less::FileWriteUnit::WavToFile(const std::string &Path, int x_length, double *x, int sr) {
    rapidjson::StringBuffer s;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);
    writer.StartObject();
    writer.Key("NODE");
    writer.String("LESS_FREAM_DOUBLE");
    writer.Key("X_LENGTH");
    writer.Int(x_length);
    writer.Key("SAMPLE_RATE");
    writer.Int(sr);
    writer.Key("AUDIO");
    writer.StartArray();
    for (int i = 0; i < x_length; ++i) {
        writer.Double(x[i]);
    }
    writer.EndArray();
    writer.EndObject();
    std::ofstream fout(Path);
    fout << s.GetString();
    fout.close();
}

