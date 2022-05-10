//
// Created by Ghost Gloomy on 2020/8/27.
//

#ifndef LESSAMPLER_BINUNIT_H
#define LESSAMPLER_BINUNIT_H

#include <iostream>
#include <fstream>
#include "AudioModel/lessAudioModel.h"

class BinUnit {
public:
    /*
     * F0 to binary
     */
    void F0BIN(const std::string &Path, lessAudioModel audioModel);

    /*
     * Sp to binary
     */
    void SPBIN(const std::string &Path, lessAudioModel audioModel);

    /*
     * Ap to binary
     */
    void APBIN(const std::string &Path, lessAudioModel audioModel);

    /*
     * Binary to F0
     */
    static lessAudioModel BINF0(const std::string &Path);
};


#endif //LESSAMPLER_BINUNIT_H
