//
// Created by YuzukiTsuru on 2021/3/17.
//

#include "libUTAU/libUTAU.h"

// Call: ./libUTAUTest "D:\UATU\voice\uta\偛.wav" "C:\Users\gloom\Desktop\main.cache\3_偛_F4_DLiBOd.wav" F4 100 "" 36.0 300 151.0 79.0 100 100 !240 AA#34#ABABADAFAHAKANAQAUAYAbAfAhAiAj#3#AiAiAgAfAeAcAaAXAVASAQANAKAHADAA/9/6/3/0/x/u/r/p/n/k/j/h/g/f/e/f/i/l/o/r/u/x/0/3/6/8/+7U

int main(int argc, char **argv) {
    try {
        libUTAU libUtau(argc, argv);
        libUtau.PrintUTAUPara();
    } catch (const std::runtime_error &e) {
        std::cerr << e.what() << std::endl;
    }
    return 0;
}