#include "ots/otsHelper.h"

#include "storage.h"
#include "util.h"

String otsUrlEncode(const char *msg) {
    static const char *hex = "0123456789ABCDEF";
    String encodedMsg = "";

    while (*msg != '\0') {
        if (
            ('a' <= *msg && *msg <= 'z') || ('A' <= *msg && *msg <= 'Z') || ('0' <= *msg && *msg <= '9') || *msg == '-' || *msg == '_' || *msg == '.' || *msg == '~') {
            encodedMsg += *msg;
        } else {
            encodedMsg += '%';
            encodedMsg += hex[(unsigned char)*msg >> 4];
            encodedMsg += hex[*msg & 0xf];
        }
        msg++;
    }
    return encodedMsg;
}

bool readJsonFromUrl(DynamicJsonDocument &doc, String url, String root, const uint16_t timeout, JsonDocument *filter) {
    // root is optional; default "/"

    // 1. Check if url contains url a protocol: ://, if not add file://
    // 2. Check if protocol is file:// then load from local
    // 3. Check if protocol is http:// or https:// then load 
    // 3. Otherwise unsupported protocol

    // Check if protocol is set; otherwise add "file://" as default
    // Could be made simpler; but this way you can change the default
    // protocol more easier
    wsLog("readJsonFromUrl: "+url);
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
            DeserializationError error;
            if (filter) {
                error = deserializeJson(doc, file, DeserializationOption::Filter(*filter));
            } else {
                error = deserializeJson(doc, file);
            }

            if (error) {
                if (error) {
                    Serial.printf("readJsonFromUrl() -> JSON error: %s\n", error.c_str());
                    wsErr("readJsonFromUrl() -> JSON error: " + String(error.c_str()));
                    return false;
                }

                return false;
            }
            file.close();
            return true;
        }
        wsErr("File " + url + "  not found");
        return false;

        // read json from http:// or https:// stream
    } else if (url.startsWith("http://") || url.startsWith("https://")) {
        HTTPClient http;
        http.begin(url);
        http.setTimeout(timeout);
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        int httpCode = http.GET();
        if (httpCode != 200) {
            http.end();
            wsErr("readJsonFromUrl() -> http error: " + String(httpCode));
            return false;
        }

        DeserializationError error;
        if (filter) {
            error = deserializeJson(doc, http.getString(), DeserializationOption::Filter(*filter));
        } else {
            error = deserializeJson(doc, http.getString());
        }
        http.end();
        if (error) {
            Serial.printf("readJsonFromUrl() -> JSON error: %s\n", error.c_str());
            wsErr("readJsonFromUrl() -> JSON error: " + String(error.c_str()));
            return false;
        }
        http.end();
        return true;

        // unsupported file format
    } else {
        wsErr("unsupported protocol format: " + url);  // ftp, etc.
        return false;
    }

    return true;
}
