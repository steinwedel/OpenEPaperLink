#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

#include "makeimage.h"
#include "tag_db.h"

void processTemplate(String url, tagRecord *&taginfo, imgParam &imageParams);
bool processTemplateFile(String url, tagRecord *&taginfo, imgParam &imageParams);
bool processDisplayType(String displayTypeName, DynamicJsonDocument &vars, DynamicJsonDocument &doc, TFT_eSprite &spr, tagRecord *&taginfo, imgParam &imageParams);
String otsReplaceVars(DynamicJsonDocument &vars, String key);
int otsFileCopy(DynamicJsonDocument &vars);
bool otsFileDelete(DynamicJsonDocument &vars);
bool otsFileRename(DynamicJsonDocument &vars);