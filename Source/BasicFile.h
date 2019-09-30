#pragma once
#include <iostream>
#include "LangPack.h"
#include "json/json.h"

// Define LANGPACK
Json::Value LANGPACKDATA() {
    // Combined language and translation
    Json::Value basicTrans;
    basicTrans["The input parameter is wrong. please check if it is called correctly or the parameter is correct."] = Json::Value("The input parameter is wrong. please check if it is called correctly or the parameter is correct.");
    basicTrans["Use Suggestions"] = Json::Value("Use Suggestions");
    basicTrans["Please call in the UTAU program or call with relevant parameters"] = Json::Value("Please call in the UTAU program or call with relevant parameters");
    basicTrans["Whats Wrong???"] = Json::Value("Whats Wrong???");
    basicTrans["Whats Wrong??? Should not be like this......"] = Json::Value("Whats Wrong？？？Should not be like this......");
    
    
    Json::Value langpack;
    langpack["LESSAMPLERLANG"] = basicTrans;
    langpack["LC"] = Json::Value("en_US");
    return langpack;
}

// Define Config
Json::Value CONFIGDATA(){
    Json::Value basicData;
    basicData["LANGUAGE"] = Json::Value("en_US");
    Json::Value config;
    config["LESSAMPLER"] = basicData;
    return config;
}

/*
* Structure of LANGPACK:
* {
*  "LC" : "en_US",
*  "LESSAMPLERLANG" :
*   {
*       "Key" : "Value",
*             :
*             :
*    }
* }
*/

/*
 * Structure of CONFIG
 {
    "LESSAMPLER" :
    {
        "LANGUAGE" : "en_US"
    }
 }
 */
