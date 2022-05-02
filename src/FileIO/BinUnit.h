//
// Created by Ghost Gloomy on 2020/8/27.
//

#ifndef LESSAMPLER_BINUNIT_H
#define LESSAMPLER_BINUNIT_H

#include <iostream>
#include <fstream>
#include "lessAudioModel.h"

class BinUnit {
public:
    /*
     * F0 to binary
     */
    void F0BIN(const std::string &Path, AudioModel audioModel);

    /*
     * Sp to binary
     */
    void SPBIN(const std::string &Path, AudioModel audioModel);

    /*
     * Ap to binary
     */
    void APBIN(const std::string &Path, AudioModel audioModel);

    /*
     * Binary to F0
     */
    static AudioModel BINF0(const std::string &Path);

private:
    static std::ifstream::pos_type filesize(const std::string& filename);
};


#endif //LESSAMPLER_BINUNIT_H
