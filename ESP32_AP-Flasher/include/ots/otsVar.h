#define WEATHER 1

#ifdef WEATHER
#include <ArduinoJson.h>

void varReadJsonObjFromUrl(DynamicJsonDocument &vars, String url);
bool varReadStringFromUrl(DynamicJsonDocument &vars);
void otsVarSetGeoLocation(DynamicJsonDocument &vars);
void otsVarSetPreceptionForecast(DynamicJsonDocument &vars);
void otsVarSetWeather(DynamicJsonDocument &vars);
void otsVarSetTimeNow(DynamicJsonDocument &vars);
void otsVarSetTimeString(DynamicJsonDocument &vars);

// Helper functions
const String otsGetWeatherIcon(const uint8_t id, const bool isNight = false);
int otsWindSpeedToBeaufort(const float windSpeed);
String otsWindDirectionIcon(const int degrees);

#endif