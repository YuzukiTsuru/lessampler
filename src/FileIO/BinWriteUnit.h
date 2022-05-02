//
// Created by Ghost Gloomy on 2020/8/27.
//

#ifndef LESSAMPLER_BINUNIT_H
#define LESSAMPLER_BINUNIT_H

#include <iostream>

namespace less {
    class BinWriteUnit {
    public:
        /*
         * F0 to binary
         */
        static void F0BIN(const std::string &Path, AudioModel audioModel);

        /*
         * Sp to binary
         */
        static void SPBIN(const std::string &Path, AudioModel audioModel);

        /*
         * Ap to binary
         */
        static void APBIN(const std::string &Path, AudioModel audioModel);
    };
}


#endif //LESSAMPLER_BINUNIT_H
