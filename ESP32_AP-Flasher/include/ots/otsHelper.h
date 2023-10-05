#include <ArduinoJson.h>

String otsUrlEncode(const char* msg);
//bool readJsonFromUrl(DynamicJsonDocument& doc, String URL, String root = "/");
bool readJsonFromUrl(DynamicJsonDocument &doc, String url, String root = "/", const uint16_t timeout = 5000, JsonDocument *filter = nullptr);