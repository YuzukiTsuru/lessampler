#include <locale.h>
#include <sys/stat.h>
#include "LangPack.h"

LangPack::LangPack() {}

void LangPack::fetch(char *instr) {
    doc.LoadFile("lang.xml");
}

const char *LangPack::systemLanguage() {
    doc.LoadFile("config.xml");
    const char* langName = doc.FirstChildElement("LESSAMPLERSETTING")->FirstChildElement("LANGUAGE")->GetText();
    return langName;
}

bool LangPack::exists (const char *name) {
  struct stat buffer;
  return (stat (name, &buffer) == 0);
}
