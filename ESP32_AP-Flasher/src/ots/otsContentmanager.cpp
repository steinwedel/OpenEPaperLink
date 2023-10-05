#ifdef OTS
#include "ots/otsContentmanager.h"

#define CONTENT_QR
#define CONTENT_RSS
#define CONTENT_CAL
#define CONTENT_BUIENRADAR

#ifdef CONTENT_RSS
#include <rssClass.h>
#endif
#include "commstructs.h"
#include "newproto.h"
#include "ots/otsScript.h"
#include "storage.h"
#include "truetype.h"
#include "util.h"

void drawNewOBSOLET(const uint8_t mac[8], const bool buttonPressed, tagRecord *&taginfo) {
    time_t now;
    time(&now);

    const HwType hwdata = getHwType(taginfo->hwType);
    if (hwdata.bpp == 0) {
        taginfo->nextupdate = now + 60;
        wsErr("No definition found for tag type " + String(taginfo->hwType));
        return;
    }

    uint8_t wifimac[8];
    WiFi.macAddress(wifimac);
    memset(&wifimac[6], 0, 2);

    const bool isAp = memcmp(mac, wifimac, 8) == 0;
    if ((taginfo->wakeupReason == WAKEUP_REASON_FIRSTBOOT || taginfo->wakeupReason == WAKEUP_REASON_WDT_RESET) && taginfo->contentMode == 0) {
        if (isAp) {
            taginfo->contentMode = 21;
            taginfo->nextupdate = 0;
        } else if (contentFS->exists("/tag_defaults.json")) {
            StaticJsonDocument<3000> doc;
            fs::File tagDefaults = contentFS->open("/tag_defaults.json", "r");
            DeserializationError err = deserializeJson(doc, tagDefaults);
            if (!err) {
                if (doc.containsKey("contentMode")) {
                    taginfo->contentMode = doc["contentMode"];
                }
                if (doc.containsKey("modecfgjson")) {
                    taginfo->modeConfigJson = doc["modecfgjson"].as<String>();
                }
            }
            tagDefaults.close();
        }
    }

    char hexmac[17];
    mac2hex(mac, hexmac);
    String filename = "/" + String(hexmac) + ".raw";
#ifdef YELLOW_IPS_AP
    if (isAp) {
        filename = "direct";
    }
#endif

    struct tm time_info;
    getLocalTime(&time_info);
    time_info.tm_hour = time_info.tm_min = time_info.tm_sec = 0;
    time_info.tm_mday++;
    const time_t midnight = mktime(&time_info);

    DynamicJsonDocument doc(500);
    deserializeJson(doc, taginfo->modeConfigJson);
    JsonObject cfgobj = doc.as<JsonObject>();
    char buffer[64];

    wsLog("Updating " + String(hexmac));
    taginfo->nextupdate = now + 60;

    imgParam imageParams;

    imageParams.width = hwdata.width;
    imageParams.height = hwdata.height;
    imageParams.bpp = hwdata.bpp;
    imageParams.rotatebuffer = hwdata.rotatebuffer;

    imageParams.hasRed = false;
    imageParams.dataType = DATATYPE_IMG_RAW_1BPP;
    imageParams.dither = false;
    if (taginfo->hasCustomLUT && taginfo->lut != 1) imageParams.grayLut = true;

    imageParams.invert = false;
    imageParams.symbols = 0;
    imageParams.rotate = taginfo->rotate;

    imageParams.lut = EPD_LUT_NO_REPEATS;

    imageParams.lut = EPD_LUT_NO_REPEATS;
    time_t last_midnight = now - now % (24 * 60 * 60) + 3 * 3600;  // somewhere in the middle of the night
    if (imageParams.shortlut == SHORTLUT_DISABLED || taginfo->lastfullupdate < last_midnight || taginfo->lut == 1) {
        imageParams.lut = EPD_LUT_DEFAULT;
        taginfo->lastfullupdate = now;
    }
    if (taginfo->hasCustomLUT && taginfo->capabilities & CAPABILITY_SUPPORTS_CUSTOM_LUTS && taginfo->lut != 1) {
        Serial.println("using custom LUT");
        imageParams.lut = EPD_LUT_OTA;
    }
    switch (taginfo->contentMode) {
        case 0:  // Static Image
        {
            const String configFilename = cfgobj["filename"].as<String>();
            if (!util::isEmptyOrNull(configFilename) && !cfgobj["#fetched"].as<bool>()) {
                imageParams.dither = cfgobj["dither"] && cfgobj["dither"] == "1";
                jpg2buffer(configFilename, filename, imageParams);
                if (imageParams.hasRed) {
                    imageParams.dataType = DATATYPE_IMG_RAW_2BPP;
                }
                if (prepareDataAvail(filename, imageParams.dataType, imageParams.lut, mac, cfgobj["timetolive"].as<int>(), true)) {
                    cfgobj["#fetched"] = true;
                    if (cfgobj["delete"].as<String>() == "1") {
                        contentFS->remove("/" + configFilename);
                    }
                } else {
                    wsErr("Error accessing " + filename);
                }
            }
            taginfo->nextupdate = 3216153600;
        } break;

        case 1:  // Today
        {
            // filename = "current/" + String(hexmac) + ".raw";
            processTemplateFile("file:///templates/date.json", taginfo, imageParams);
            break;
        }

        case 2:  // CountDays
        {
            int32_t counter = cfgobj["counter"].as<int32_t>();
            if (buttonPressed) counter = 0;
            processTemplateFile("file:///templates/countdays.json", taginfo, imageParams);  // midnight
            cfgobj["counter"] = counter + 1;
            break;
        }

        case 3:  // CountHours
        {
            int32_t counter = cfgobj["counter"].as<int32_t>();
            if (buttonPressed) counter = 0;
            processTemplateFile("file:///templates/counthours.json", taginfo, imageParams);  // all 60 minutes
            cfgobj["counter"] = counter + 1;
            break;
        }

        case 4:                                                                           // Weather
            processTemplateFile("file:///templates/weather.json", taginfo, imageParams);  // all 60 minutes
            break;

        case 8:                                                                            // Forecast
            processTemplateFile("file:///templates/forecast.json", taginfo, imageParams);  // all 60 minutes
            break;

        case 5:  // Firmware

            filename = cfgobj["filename"].as<String>();
            if (!util::isEmptyOrNull(filename) && !cfgobj["#fetched"].as<bool>()) {
                if (prepareDataAvail(filename, imageParams.dataType, imageParams.lut, mac, cfgobj["timetolive"].as<int>(), true)) {
                    cfgobj["#fetched"] = true;
                } else {
                    wsErr("Error accessing " + filename);
                }
                cfgobj["filename"] = "";
                taginfo->nextupdate = 3216153600;
                taginfo->contentMode = 0;
            } else {
                taginfo->nextupdate = now + 300;
            }
            break;

        case 7:  // ImageUrl
        {
            const int httpcode = getImgURL(filename, cfgobj["url"], (time_t)cfgobj["#fetched"], imageParams, String(hexmac));
            const int interval = cfgobj["interval"].as<int>();
            if (httpcode == 200) {
                taginfo->nextupdate = now + 60 * (interval < 3 ? 15 : interval);
                updateTagImage(filename, mac, interval, taginfo, imageParams);
                cfgobj["#fetched"] = now;
            } else if (httpcode == 304) {
                taginfo->nextupdate = now + 60 * (interval < 3 ? 15 : interval);
            } else {
                taginfo->nextupdate = now + 300;
            }
            break;
        }

        case 9:  // RSSFeed

            /*if (getRssFeed(filename, cfgobj["url"], cfgobj["title"], taginfo, imageParams)) {
                const int interval = cfgobj["interval"].as<int>();
                taginfo->nextupdate = now + 60 * (interval < 3 ? 60 : interval);
                updateTagImage(filename, mac, interval, taginfo, imageParams);
            } else {
                taginfo->nextupdate = now + 300;
            }
            break;*/

        case 10:                                                                         // QRcode:
            processTemplateFile("file:///templates/qrcode.json", taginfo, imageParams);  // all 60 minutes
            break;

        case 11:  // Calendar:

            /*if (getCalFeed(filename, cfgobj["apps_script_url"], cfgobj["title"], taginfo, imageParams)) {
                const int interval = cfgobj["interval"].as<int>();
                taginfo->nextupdate = now + 60 * (interval < 3 ? 15 : interval);
                updateTagImage(filename, mac, interval, taginfo, imageParams);
            } else {
                taginfo->nextupdate = now + 300;
            }*/
            break;

        case 12:  // RemoteAP

            taginfo->nextupdate = 3216153600;
            break;

        case 13:  // SegStatic

            sprintf(buffer, "%-4.4s%-2.2s%-4.4s", cfgobj["line1"].as<const char *>(), cfgobj["line2"].as<const char *>(), cfgobj["line3"].as<const char *>());
            taginfo->nextupdate = 3216153600;
            sendAPSegmentedData(mac, (String)buffer, 0x0000, false, (taginfo->isExternal == false));
            break;

        case 14:  // NFC URL

            taginfo->nextupdate = 3216153600;
            prepareNFCReq(mac, cfgobj["url"].as<const char *>());
            break;

        case 15:  // send gray LUT

            taginfo->nextupdate = 3216153600;
            prepareLUTreq(mac, cfgobj["bytes"]);
            taginfo->hasCustomLUT = true;
            break;

        case 16:  // buienradar

        {
            processTemplateFile("file:///templates/preception.json", taginfo, imageParams);  // all 60 minutes
            break;
        }

        case 17:  // tag command
            sendTagCommand(mac, cfgobj["cmd"].as<int>(), (taginfo->isExternal == false));
            cfgobj["filename"] = "";
            taginfo->nextupdate = 3216153600;
            taginfo->contentMode = 0;
            break;

        case 18:  // tag config
            prepareConfigFile(mac, cfgobj);
            cfgobj["filename"] = "";
            taginfo->nextupdate = 3216153600;
            taginfo->contentMode = 0;
            break;

        case 19:  // json template
        {
            const String url = cfgobj["url"].as<String>();
            processTemplate(url, taginfo, imageParams);
            //updateTagImage(filename, mac, 0, taginfo, imageParams);
            //taginfo->nextupdate = 3216153600;

            break;
        }

        case 20:  // display a copy
        {
            break;
        }

        case 21:  // ap info
            processTemplate("file:///templates/apinfo.json", taginfo, imageParams);
            break;

            /*TFT_eSprite spr = TFT_eSprite(&tft);
            initSprite(spr, imageParams.width, imageParams.height, imageParams);
            spr2buffer(spr, filename, imageParams);
            spr.deleteSprite();*/

/*            drawAPinfo(filename, cfgobj, taginfo, imageParams);
            updateTagImage(filename, mac, 0, taginfo, imageParams);
            taginfo->nextupdate = 3216153600;*/
            break;

    }

    taginfo->modeConfigJson = doc.as<String>();
}

#endif