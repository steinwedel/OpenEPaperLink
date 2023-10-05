#include <Arduino.h>
#include <ArduinoJson.h>

#include "U8g2_for_TFT_eSPI.h"

void otsDrawBox(TFT_eSprite &spr, DynamicJsonDocument &vars);
void otsDrawRoundBox(TFT_eSprite &spr, DynamicJsonDocument &vars);
void otsDrawLine(TFT_eSprite &spr, DynamicJsonDocument &vars);
void otsDrawCircle(TFT_eSprite &spr, DynamicJsonDocument &vars);
void otsDrawQR(TFT_eSprite &spr, DynamicJsonDocument &vars);
void otsDrawString(TFT_eSprite &spr, DynamicJsonDocument &vars);

// Helper
uint16_t otsGetColor(String color);
byte otsGetAlign(const String &align);
uint8_t otsProcessFontType(String font);