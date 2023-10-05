#include "ots/otsVar.h"

#ifdef WEATHER
#endif

#include <ArduinoJson.h>

#include <cmath>
// #include "newproto.h"
#include "ots/otsHelper.h"
#include "storage.h"
#include "util.h"

void varReadJsonObjFromUrl(DynamicJsonDocument &vars, String url) {
    DynamicJsonDocument doc(5000);
    if (readJsonFromUrl(doc, url)) {
        for (const auto &kv : doc.as<JsonObject>()) {
            vars[String(kv.key().c_str())] = kv.value().as<String>();
            // wsLog(vars[String(kv.key().c_str())].as<String>());
        }
    }
}

bool varReadStringFromUrl(DynamicJsonDocument &vars) {
    String url = vars["url"].as<String>();
    String root = vars["root"].as<String>();

    if (util::isEmptyOrNull(url)) {
        wsLog("Empty Url");
        return false;
    }

    if (url.indexOf("://") == -1) {
        url = String("file://" + url);
    }

    // read json from file
    if (url.startsWith("file://")) {
        url.replace("file://", "");

        // add leading slash or other start point in front of filename
        if (!url.startsWith("/")) {
            url = root + url;
        }

        File file = contentFS->open(url, "r");
        if (file) {
            if (file.available()) {
                vars["key"] = file.readString();
            }
            file.close();
            return true;
        }
        wsErr("File not found");
        return false;

        // read json from http:// or https:// stream
    } else if (url.startsWith("http://") || url.startsWith("https://")) {
        HTTPClient http;
        http.begin(url);
        http.setTimeout(10000);
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        int httpCode = http.GET();
        if (httpCode != 200) {
            wsErr("http error " + String(httpCode));
            return false;
        }

        vars["key"] = http.getString();
        http.end();
        return true;

        // unsupported file format
    } else {
        wsErr("unsupported protocol format");  // ftp, etc.
        return false;
    }

    return true;
}

void otsVarSetGeoLocation(DynamicJsonDocument &vars) {
    const String lat = vars["lat"].as<String>();
    const String lon = vars["lon"].as<String>();

    if (util::isEmptyOrNull(lat) || util::isEmptyOrNull(lon)) {
        wsLog("get location");
        DynamicJsonDocument filter(80);
        filter["results"][0]["latitude"] = true;
        filter["results"][0]["longitude"] = true;
        filter["results"][0]["timezone"] = true;

        // filter = {"results":[{"latitude":true}]};

        String url = "https://geocoding-api.open-meteo.com/v1/search";
        url += "?name=" + otsUrlEncode(vars["location"]);
        url += "&count=1";

        DynamicJsonDocument doc(5000);
        // if (util::httpGetJson(url, doc, 5000, &filter)) {
        if (readJsonFromUrl(doc, url, "/", 5000, &filter)) {
            vars["lat"] = doc["results"][0]["latitude"].as<String>();
            vars["lon"] = doc["results"][0]["longitude"].as<String>();
            vars["tz"] = doc["results"][0]["timezone"].as<String>();
            vars["test"] = doc["results"][0];
        } else {
            wsErr("Geolocation failed");
        }
    }
}

void otsVarSetPreceptionForecast(DynamicJsonDocument &vars) {
    HTTPClient http;
    String url = "https://gpsgadget.buienradar.nl/data/raintext";  // Niederschlagsintensität mm/h = 10^((Wert-109)/32)
    url += "?lat=" + vars["lat"].as<String>();
    url += "&lon=" + vars["lon"].as<String>();
    wsLog("otsVarSetPreceptionForecast");
    wsLog(url);
    http.begin(url);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(5000);
    int httpCode = http.GET();

    if (httpCode == 200) {
        String response = http.getString();
        for (int i = 0; i < 24; i++) {
            const int startPos = i * 11;
            uint8_t value = response.substring(startPos, startPos + 3).toInt();  // String(pow(10,((value-109)/32)))
            value = pow(10, ((value - 109) / 32.0f)) * 10;                       // Value =  mm preception*10
            const String timestring = response.substring(startPos + 4, startPos + 9);
            vars["PreceptionValue" + String(i)] = value;
            vars["PreceptionTime" + String(i)] = timestring;

            // wsLog("Preception: " + timestring + " : " + value);
        }
    } else {
        wsErr("Buitenradar http " + String(httpCode));
    }
}

void otsVarSetWeather(DynamicJsonDocument &vars) {
    wsLog("get weather");

    if (!vars.containsKey("temperatureUnit")) {
        vars["temperatureUnit"] = "celsius";
    }

    if (!vars.containsKey("windspeedUnit")) {
        vars["windspeedUnit"] = "kmh";
    }

    if (!vars.containsKey("precipitationUnit")) {
        vars["precipitationUnit"] = "mm";
    }

    if (!vars.containsKey("timezone")) {
        vars["timezone"] = "auto";
    }

    DynamicJsonDocument doc(3000);
    String url = "https://api.open-meteo.com/v1/forecast";
    url += "?latitude=" + vars["lat"].as<String>();
    url += "&longitude=" + vars["lon"].as<String>();
    url += "&current_weather=true";
    url += "&windspeed_unit=kmh";
    url += "&timezone=" + vars["tz"].as<String>();
    url += "&temperature_unit=" + vars["temperatureUnit"].as<String>();
    url += "&windspeed_unit=" + vars["windspeedUnit"].as<String>();
    url += "&precipitation_unit=" + vars["precipitationUnit"].as<String>();
    url += "&daily=weathercode,temperature_2m_max,temperature_2m_min,sunrise,sunset,precipitation_sum,precipitation_hours,precipitation_probability_max,windspeed_10m_max,windgusts_10m_max,winddirection_10m_dominant";
    url += "&timeformat=unixtime&forecast_days=5";
    // const bool success = util::httpGetJson(url, doc, 3000);
    const bool success = readJsonFromUrl(doc, url, "/", 3000);
    if (!success) {
        wsErr("Weather could not be retrieved: ");
        return;
    }

    const auto &currentWeather = doc["current_weather"];
    vars["temperatureCur"] = currentWeather["temperature"].as<double>();
    vars["windSpeedCur"] = currentWeather["windspeed"].as<int>();
    vars["windDirCur"] = currentWeather["winddirection"].as<int>();
    const bool isNight = currentWeather["is_day"].as<int>() == 0;
    uint8_t weathercode = currentWeather["weathercode"].as<int>();
    if (weathercode > 40) weathercode -= 40;
    vars["weatherIconCur"] = String(otsGetWeatherIcon(weathercode, isNight));
    // const int beaufort = windSpeedToBeaufort(windspeed);
    vars["windDirCur"] = String(currentWeather["winddirection"].as<int>());
    vars["windDirSignCur"] = otsWindDirectionIcon(currentWeather["winddirection"].as<int>());
    if (weathercode > 10) {
        vars["umbrellaIcon"] = "\uf084";
    } else {
        vars["umbrellaIcon"] = "";
    }

    for (int i = 0; i <= 4; i++) {
        vars["temperatureMin" + String(i)] = doc["daily"]["temperature_2m_min"][i].as<int>();
        vars["temperatureMax" + String(i)] = doc["daily"]["temperature_2m_max"][i].as<int>();
        vars["windSpeed" + String(i)] = doc["daily"]["windspeed_10m_max"][i];
        vars["windGusts" + String(i)] = doc["daily"]["windgusts_10m_max"][i];
        vars["windDirection" + String(i)] = doc["daily"]["winddirection_10m_dominant"][i];
        vars["windDirSign" + String(i)] = otsWindDirectionIcon(doc["daily"]["winddirection_10m_dominant"][i]);
        vars["precipitationSum" + String(i)] = doc["daily"]["precipitation_sum"][i];
        vars["precipitationHours" + String(i)] = doc["daily"]["precipitation_hours"][i];
        vars["precipitationProbabilityMax" + String(i)] = doc["daily"]["precipitation_probability_max"][i];
        vars["sunrise" + String(i)] = String(doc["daily"]["sunrise"][i].as<String>());
        vars["sunset" + String(i)] = String(doc["daily"]["sunset"][i].as<String>());
        weathercode = doc["daily"]["weathercode"][i].as<int>();
        if (weathercode > 40) weathercode -= 40;
        vars["weatherIcon" + String(i)] = String(otsGetWeatherIcon(weathercode, false));
    }

    doc.clear();
}

/*void otsVarSetMoonPhase(DynamicJsonDocument &vars) {
    
    //  calculates the moon phase (0-27), accurate to 1 segment.
    //  0 = > new moon.
    //  14 => full moon.
    
    int y = vars["year"].as<int>();
    int m = vars["month"].as<int>();
    int d = vars["day"].as<int>();

    int c, e;
    double jd;
    int b;

    if (m < 3) {
        y--;
        m += 12;
    }
    ++m;
    c = 365.25 * y;
    e = 30.6 * m;
    jd = c + e + d - 694039.09; // jd is total days elapsed
    jd /= 29.53;                // divide by the moon cycle (29.53 days)
    b = jd;                     // int(jd) -> b, take integer part of jd
    jd -= b;                    // subtract integer part to leave fractional part of original jd
    b = jd * 28 + 0.5;          // scale fraction from 0-27 and round by adding 0.5
    b = b & 7;                  // 0 and 8 are the same so turn 8 into 0
    vars[vars["resultkey"].as<String>()] = b;
}*/

void otsVarSetTimeNow(DynamicJsonDocument &vars) {
    time_t current_time;
    time(&current_time);
    vars[vars["resultkey"].as<String>()] = (long)current_time;
}

void otsVarSetTimeString(DynamicJsonDocument &vars) {
    time_t current_time;
    struct tm *time_info;
    char timeString[20];
    String timekey = vars["key"];
    current_time = (time_t)vars[vars["key"].as<String>()].as<long>();
    time_info = localtime(&current_time);
    strftime(timeString, sizeof(timeString), vars["value"], time_info);
    vars[vars["resultkey"].as<String>()] = String(timeString);
}

// Helper functions

/// @brief Get a weather icon
/// @param id Icon identifier/index
/// @param isNight Use night icons (true) or not (false)
/// @return String reference to icon
const String otsGetWeatherIcon(const uint8_t id, const bool isNight) {
    const String weatherIcons[] = {"\uf00d", "\uf00c", "\uf002", "\uf013", "\uf013", "\uf014", "", "", "\uf014", "", "",
                                   "\uf01a", "", "\uf01a", "", "\uf01a", "\uf017", "\uf017", "", "", "",
                                   "\uf019", "", "\uf019", "", "\uf019", "\uf015", "\uf015", "", "", "",
                                   "\uf01b", "", "\uf01b", "", "\uf01b", "", "\uf076", "", "", "\uf01a",
                                   "\uf01a", "\uf01a", "", "", "\uf064", "\uf064", "", "", "", "",
                                   "", "", "", "", "\uf01e", "\uf01d", "", "", "\uf01e"};
    if (isNight && id <= 3) {
        const String nightIcons[] = {"\uf02e", "\uf083", "\uf086"};
        return nightIcons[id];
    }
    return weatherIcons[id];
}

int otsWindSpeedToBeaufort(const float windSpeed) {
    constexpr static const float speeds[] = {0.3, 1.5, 3.3, 5.5, 8, 10.8, 13.9, 17.2, 20.8, 24.5, 28.5, 32.7};
    constexpr static const int numSpeeds = sizeof(speeds) / sizeof(speeds[0]);
    int beaufort = 0;
    for (int i = 0; i < numSpeeds; i++) {
        if (windSpeed >= speeds[i]) {
            beaufort = i + 1;
        }
    }
    return beaufort;
}

String otsWindDirectionIcon(const int degrees) {
    static const String directions[] = {"\uf044", "\uf043", "\uf048", "\uf087", "\uf058", "\uf057", "\uf04d", "\uf088"};
    int index = (degrees + 22) / 45;
    if (index >= 8) {
        index = 0;
    }
    return directions[index];
}
