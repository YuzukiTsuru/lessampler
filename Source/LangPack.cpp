#include "LangPack.h"

#include <sys/stat.h>

#include <fstream>

#include "BasicFile.h"
#include "json/json.h"

LangPack::LangPack() {
  if (filexists.exists("config.json") || filexists.exists("en_US.json")) {
    Json::Value config;
    std::ifstream configData("config.json", std::ifstream::binary);
    configData >> config;
    std::string langname = config["LESSAMPLER"]["LANGUAGE"].asString();
    std::string fileNameAll = langname + ".json";
    std::ifstream data(fileNameAll, std::ifstream::binary);
    data >> root;
  }
}

LangPack::~LangPack() {
  root.clear();
}

const char* LangPack::tr(std::string instr) {
  return this->fetchs(instr);
}

void LangPack::initFile() {
  std::ofstream File;
  // Open file write config
  File.open("config.json");
  if (File.is_open()) {
    File << CONFIGDATA();
  } else {
    dialog.errorDialog("No write permission");
  }
  File.close();
  // Write en_US.json
  File.open("en_US.json");
  if (File.is_open()) {
    File << LANGPACKDATA();
  } else {
    dialog.errorDialog("No write permission");
  }
  File.close();
}

const char* LangPack::fetchs(std::string instr) {
  const char* trans = root["LESSAMPLERLANG"][instr].asCString();
  return trans;
}

Json::Value LangPack::readJsonFile(const char* filename) {
  std::ifstream filedata(filename, std::ifstream::binary);
  filedata >> root;
  return root;
}
