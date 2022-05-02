//
// @Author Ghost Gloomy on 2020/1/26.
//
#ifndef LESSAMPLER_FILEREADUNIT_H
#define LESSAMPLER_FILEREADUNIT_H

#include <iostream>
#include <sndfile.h>

#define ENABLE_SNDFILE_WINDOWS_PROTOTYPES 1

namespace less {
    class FileReadUnit {
    public:
        /*
         * Get Audio Length
         */
        static int GetAudioLength(const char *filename);

        /*
         * Read Audio
         */
        static int WavRead(const char *FilePath, double *output);

        /*
         * File To Audio
         */
        static void FileToWav(const std::string &Path, double *output);
    };
}
#endif //LESSAMPLER_FILEREADUNIT_H
