#include <fstream>
#include <sys/stat.h>
#include "BasicFile.h"
#include "LangPack.h"

LangPack::LangPack() {}

const char *LangPack::fetch(std::string instr) {
    return " ";
}

bool LangPack::exists(const char *name) {
    struct stat buffer;
    return stat(name, &buffer) == 0;
}

void LangPack::initFile() {
    Json::Value langpack;
    
    std::ofstream File;
    // Open file write config
    File.open("config.json");
    if (File.is_open()) {
        File << CONFIGDATA();
    }else{
        dialog.errorDialog("No write permission");
    }
    File.close();
    // Write lang.json
    File.open("lang.json");
    if (File.is_open()) {
        File << LANGPACKDATA();
    }else{
        dialog.errorDialog("No write permission");
    }
    File.close();
}

Json::Value LangPack::readJsonFile(const char* filename){
    std::ifstream filedata(filename, std::ifstream::binary);
    filedata >> root;
    return root;
}
