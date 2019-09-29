#pragma once
#include "tinyxml2.h"

class LangPack {
  public:
    /*
     * Init The Class
     */
    LangPack();

    /*
     * Fetch Origin Language and change for translate.
     */
    void fetch(char *instr);

    /*
     * Get The System Language
     * OUT :: CHAR (Language
     */
    const char* systemLanguage();
    
    /*
     * Find The XML File
     * IN :: FileName
     * OUT :: bool
     */
    bool exists (const char *name);

  private:
    tinyxml2::XMLDocument doc;
};


/*
* Structure of the config XML file:
* - Element "LESSAMPLERSETTING"      the root Element, which is the FirstChildElement of the Document
* - - Element "LANGUAGE"             the name for language
* - - - Applaction Display Language
*/


/*
 * Structure of the lang XML file:
 * - Element "LESSAMPLERLANG"      the root Element, which is the FirstChildElement of the Document
 * - - Element "(Language name)"   the name for language
 * - - - translate
 */
