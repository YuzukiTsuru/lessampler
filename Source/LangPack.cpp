#include <locale.h>
#include <sys/stat.h>
#include <fstream>
#include "BasicFile.h"
#include "LangPack.h"

LangPack::LangPack() {}

void LangPack::fetch(char *filename, char *instr) {
    doc.LoadFile(filename);
}

const char *LangPack::systemLanguage() {
    doc.LoadFile("config.xml");
    const char *langName = doc.FirstChildElement("LESSAMPLERSETTING")->FirstChildElement("LANGUAGE")->GetText();
    return langName;
}

bool LangPack::exists(const char *name) {
    struct stat buffer;
    return stat(name, &buffer) == 0;
}

void LangPack::initFile() {
    // Open Stream
    std::ofstream File;
    // Open file
    File.open("config.xml");
    if (File.is_open()) {
        File << initConfigFileStr;
    }else{
        dialog.errorDialog("No write permission");
    }
    File.close();
    File.open("lang.xml");
    if (File.is_open()) {
        File << initLangStr;
    }else{
        dialog.errorDialog("No write permission");
    }
    File.close();
}
