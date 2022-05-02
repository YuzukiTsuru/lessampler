//
// Created by Ghost Gloomy on 2020/8/27.
//

#ifndef LESSAMPLER_FILEWRITEUNIT_H
#define LESSAMPLER_FILEWRITEUNIT_H

#include <iostream>
#include <sndfile.h>

#define ENABLE_SNDFILE_WINDOWS_PROTOTYPES 1

namespace less {
    class FileWriteUnit {
    public:
        /*
         * Write Audio
         */
        static void WavWrite(const double *x, int x_length, int fs, const char *filename);

        /*
         * Save F0 Str to File
         */
        static void F0ToFile(const std::string &Path, double *F0, int F0Len);

        /*
         * Save Seq To File
         */
        static void SpToFile(const std::string &Path, double **Spec, int fft_size, int f0_length);

        /*
         * Save AP To File
         */
        static void ApToFile(const std::string &Path, double **ap, int fft_size, int f0_length);

        /*
         * Wave Array To File
         */
        static void WavToFile(const std::string &Path, int x_length, double *x, int sr);
    };
}


#endif //LESSAMPLER_FILEWRITEUNIT_H
