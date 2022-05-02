//
// Created by Ghost Gloomy on 2020/8/27.
//

#include "FileBasic.h"

std::string less::FileBasic::MakeFileName(const std::string &filename, const std::string &ext) {
    return filename.substr(0, filename.find_last_of(".")) + ext;
}
