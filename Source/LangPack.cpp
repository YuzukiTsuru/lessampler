#include <fstream>
#include <sys/stat.h>
#include "json/json.h"
#include "BasicFile.h"
#include "LangPack.h"

LangPack::LangPack() {
    Json::Value config;
    std::ifstream configData("config.json", std::ifstream::binary);
    configData >> config;
    std::string langname    = config["LESSAMPLER"]["LANGUAGE"].asString();
    std::string fileNameAll = langname + ".json";
    std::ifstream data(fileNameAll, std::ifstream::binary);
    data >> root;
}

LangPack::~LangPack(){
    root.clear();
}

const char *LangPack::tr(std::string instr) {
    return this->fetchs(instr);
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
    } else {
        dialog.errorDialog("No write permission");
    }
    File.close();
    // Write lang.json
    File.open("lang.json");
    if (File.is_open()) {
        File << LANGPACKDATA();
    } else {
        dialog.errorDialog("No write permission");
    }
    File.close();
}

const char *LangPack::fetchs(std::string instr) {
    const char *trans = root["LESSAMPLERLANG"][instr].asCString();
    return trans;
}

Json::Value LangPack::readJsonFile(const char *filename) {
    std::ifstream filedata(filename, std::ifstream::binary);
    filedata >> root;
    return root;
}
