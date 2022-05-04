//
// Created by gloom on 2022/5/4.
//

#include <filesystem>
#include "ConfigUnit.h"

int main(int argc, char *argv[]) {
    if (std::filesystem::exists("config.ini")) {
        std::filesystem::remove("config.ini");
    }
    // Create a config file
    ConfigUnit configUnit_1("config.ini");
    // Parse the config file
    ConfigUnit configUnit_2("config.ini");
    return 0;
}