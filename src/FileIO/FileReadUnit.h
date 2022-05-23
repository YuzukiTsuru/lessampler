//
// @Author Ghost Gloomy on 2020/1/26.
//
#ifndef LESSAMPLER_FILEREADUNIT_H
#define LESSAMPLER_FILEREADUNIT_H

#include <iostream>
#include <filesystem>


extern "C" {
#include <sndfile.h>
}

#define ENABLE_SNDFILE_WINDOWS_PROTOTYPES 1

class AudioFile {
public:
    std::filesystem::path audio_file_path;
    size_t x_length;
    double *x;
    int fs;
    int nbit;
};

class FileReadUnit {
public:
    /*
     * Get Audio Length
     */
    static size_t GetAudioLength(const char *filename);

    /*
     * Read Audio
     */
    static int WavRead(const char *FilePath, double *output);
};

#endif //LESSAMPLER_FILEREADUNIT_H
