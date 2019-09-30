#pragma once
#include <iostream>
#include "json/json.h"
#include "Dialogs.h"

class LangPack {
  public:
    /*
     * Init The Class
     */
    LangPack();
    ~LangPack();

    /*
     * Translate
     */
    const char *tr(std::string instr);

    /*
     * Find The XML File
     * IN :: FileName
     * OUT :: bool
     */
    bool exists(const char *name);

    /*
     * init the file for basic
     */
    void initFile();

  private:
    /*
     * Read From Json
     * IN: Filename
     * OUT: JsonData :: Json::Value
     */
    Json::Value readJsonFile(const char *filename);

    /*
     * Fetch Origin Language and change for translate.
     */
    const char *fetchs(std::string instr);

    Dialogs dialog;
    Json::Value root;
};
