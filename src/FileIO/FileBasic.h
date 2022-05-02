//
// Created by Ghost Gloomy on 2020/8/27.
//

#ifndef LESSAMPLER_FILEBASIC_H
#define LESSAMPLER_FILEBASIC_H

#include <iostream>

namespace less {
    class FileBasic {
    public:
        /*
         * File name
         */
        static std::string MakeFileName(const std::string &filename, const std::string &ext);
    };
}

#endif //LESSAMPLER_FILEBASIC_H
