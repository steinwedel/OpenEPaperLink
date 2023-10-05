#include "ots/otsScript.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <esp_heap_caps.h>

#include "../../tag_types.h"
#include "newproto.h"
#include "ots/otsContentmanager.h"
#include "ots/otsDebug.h"
#include "ots/otsDraw.h"
#include "ots/otsHelper.h"
#include "ots/otsMath.h"
#include "ots/otsVar.h"
#include "serialap.h"
#include "storage.h"
#include "tag_db.h"
#include "util.h"

void processTemplate(String url, tagRecord *&taginfo, imgParam &imageParams) {
    otsDEBUG("processTemplate: Start - " + url);
    if (!processTemplateFile(url, taginfo, imageParams)) {
        wsErr("Default error template");
        processTemplateFile("file:///templates/error.json", taginfo, imageParams);
    }
    otsDEBUG("processTemplate: Stop");
}

bool processTemplateFile(String url, tagRecord *&taginfo, imgParam &imageParams) {
    otsDEBUG("processTemplateFile: Start - " + url);
    TFT_eSprite spr = TFT_eSprite(&tft);
    initSprite(spr, imageParams.width, imageParams.height, imageParams);

    char hexmac[17];
    mac2hex(taginfo->mac, hexmac);
    String filename = "/current/" + String(hexmac) + ".raw";

#define SIZE_DOC 10000
    DynamicJsonDocument doc(SIZE_DOC);
    DynamicJsonDocument vars(10000);

    // setlocale( LC_ALL, "de-DE.utf8");

    // Read variables from /vars.json into vars

    otsDEBUG("processTemplateFile: 1");
    varReadJsonObjFromUrl(vars, "file:///vars.json");
    // varReadJsonObjFromUrl(vars, taginfo->modeConfigJson);
    deserializeJson(doc, taginfo->modeConfigJson);
    JsonObject cfgobj = doc.as<JsonObject>();

    const HwType hwdata = getHwType(taginfo->hwType);

    otsDEBUG("processTemplateFile: 2");
    // System vars
    switch (config.language) {
        case 0:
            otsDEBUG("processTemplateFile: LangCase:0");
            vars["language"] = "";
            break;

        case 1:
            otsDEBUG("processTemplateFile: LangCase:1");
            vars["language"] = "nl";
            break;

        case 2:
            otsDEBUG("processTemplateFile: LangCase:2");
            vars["language"] = "de";
            break;

        default:
            otsDEBUG("processTemplateFile: LangCase:default");
            vars["language"] = "";
            break;
    }

    otsDEBUG("processTemplateFile: 3");
    vars["hwType"] = taginfo->hwType;
    vars["displayWidth"] = hwdata.width;
    vars["displayHeight"] = hwdata.height;
    vars["bpp"] = hwdata.bpp;
    vars["counter"] = cfgobj["counter"].as<int32_t>();
    vars["thresholdred"] = cfgobj["thresholdred"].as<int>();
    vars["location"] = cfgobj["location"].as<String>();
    vars["lat"] = cfgobj["#lat"].as<String>();
    vars["lon"] = cfgobj["#lon"].as<String>();
    vars["tz"] = cfgobj["#tz"].as<String>();
    vars["qr-content"] = cfgobj["qr-content"];
    vars["title"] = cfgobj["title"];
    vars["url"] = cfgobj["url"];

    vars["ap_ip"] = WiFi.localIP().toString();
    vars["ap_ch"] = apInfo.channel;
    vars["ap_pending"] = apInfo.pending;
    vars["mac"] = String(hexmac);

    vars["alias"] = String(taginfo->alias);

    otsDEBUG("processTemplateFile: 4");
    readJsonFromUrl(doc, url);  // read template

    // Check ots version
    if ((doc["ots"].as<signed int>() != 1)) {
        wsErr("Not supported ots Version");
        return false;
    }

    otsDEBUG("processTemplateFile: 5");
    processDisplayType("init", vars, doc, spr, taginfo, imageParams);
    if (!processDisplayType(String(taginfo->hwType), vars, doc, spr, taginfo, imageParams)) {
        if (!processDisplayType("errorDisplay", vars, doc, spr, taginfo, imageParams)) {
            return false;
        };
        return true;
    }
    processDisplayType("finish", vars, doc, spr, taginfo, imageParams);
    otsDEBUG("processTemplateFile: 6");

    spr2buffer(spr, filename, imageParams);
    spr.deleteSprite();
    updateTagImage(filename, taginfo->mac, 0, taginfo, imageParams);

    time_t now;
    time(&now);
    u_int32_t nextupdate = 3216153600;

    otsDEBUG("processTemplateFile: 7");
    if (vars["interval"].as<String>() == "midnight") {
        struct tm time_info;
        getLocalTime(&time_info);
        time_info.tm_hour = time_info.tm_min = time_info.tm_sec = 0;
        time_info.tm_mday++;
        const time_t midnight = mktime(&time_info);
        nextupdate = midnight;
    } else {
        int interval = cfgobj["interval"].as<int>();
        if (vars.containsKey("interval")) {
            interval = vars["interval"].as<int>();
        }
        nextupdate = now + 60 * (interval < 3 ? 15 : interval);
    }

    taginfo->nextupdate = nextupdate;
    otsDEBUG("processTemplateFile: 8");
    return true;

    /*    time_t now;
        time(&now);

        if (vars["interval"].as<String>() == "midnight") {
            struct tm time_info;
            getLocalTime(&time_info);
            time_info.tm_hour = time_info.tm_min = time_info.tm_sec = 0;
            time_info.tm_mday++;
            const time_t midnight = mktime(&time_info);
            taginfo->nextupdate = midnight;
        } else {
            int interval = cfgobj["interval"].as<int>();
            if (vars.containsKey("interval")) {
                interval = vars["interval"].as<int>();
            }
            taginfo->nextupdate = now + 60 * (interval < 3 ? 15 : interval);
        }
        if (imageParams.hasRed) imageParams.dataType = DATATYPE_IMG_RAW_2BPP;
        // prepareDataAvail(&filename, imageParams.dataType, taginfo->mac, 1);
        prepareDataAvail(filename, imageParams.dataType, imageParams.lut, taginfo->mac, 60, true);
        cfgobj["#lat"] = vars["lat"].as<String>();
        cfgobj["#lon"] = vars["lon"].as<String>();
        cfgobj["#tz"] = vars["tz"].as<String>();
        cfgobj["counter"] = vars["counter"].as<int32_t>();
        taginfo->modeConfigJson = doc.as<String>();

        updateTagImage(filename, taginfo->mac, 0, taginfo, imageParams);
        taginfo->nextupdate = 3216153600;


        return true;
        */
    otsDEBUG("processTemplateFile: Stop");
};

String otsReplaceVars(DynamicJsonDocument &vars, String var) {
    size_t startIndex = 0;
    size_t openBraceIndex, closeBraceIndex;

    String search, lang_search;
    while ((openBraceIndex = var.indexOf("${", startIndex)) != -1 &&
           (closeBraceIndex = var.indexOf('}', openBraceIndex + 2)) != -1) {
        openBraceIndex += 2;
        search = var.substring(openBraceIndex, closeBraceIndex);
        lang_search = vars["language"].as<String>() + "." + search;

        String searchpattern = "${" + search + "}";
        if (vars.containsKey(lang_search)) {
            if (!searchpattern.equals(vars[lang_search].as<String>())) {
                otsDEBUG(String("search:") + searchpattern + "=" + vars[lang_search].as<String>());
                var.replace(searchpattern, vars[lang_search]);
                startIndex = 0;
            } else {
                startIndex = startIndex + 1;  // skip var because identical
                otsDEBUG("Skip");
            }
        } else if (vars.containsKey(search)) {
            if (!searchpattern.equals(vars[search].as<String>())) {
                otsDEBUG(String("search:") + searchpattern + "=" + vars[search].as<String>());
                var.replace(searchpattern, vars[search]);
                startIndex = 0;
            } else {
                startIndex = startIndex + 1;  // skip var because identical
                otsDEBUG("Skip");
            }
        } else {
            startIndex = startIndex + 1;  // if var nor found skip
            otsDEBUG("Skip");
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);  // add a small delay to allow other threads to run
    }
    return var;
}

bool processDisplayType(String displayTypeName, DynamicJsonDocument &vars, DynamicJsonDocument &doc, TFT_eSprite &spr, tagRecord *&taginfo, imgParam &imageParams) {
    // process script for current display type
    otsDEBUG("processDisplayType: Start - " + displayTypeName);
    int displayType = -1;
    int errorDisplay = -1;
    for (int i = 0; i < doc["displays"].size(); i++) {
        String zw = otsReplaceVars(vars, doc["displays"][i]["type"]);
        if (zw == displayTypeName) {
            displayType = i;
            break;
        }
    }

    if (displayType == -1) return false;  // Return to run default error template "/templates/error.json"

    // process display per line entries
    for (int i = 0; i < doc["displays"][displayType]["script"].size(); i++) {
        // Serial.printf("SPIRam Total heap %d, SPIRam Free Heap %d\n", ESP.getPsramSize(), ESP.getFreePsram());
        otsDEBUG("loop begin");
        // util::printHeap();

        // Read variables from template into vars
        otsDEBUG("Read variables from template into vars");
        for (const auto &kv : doc["displays"][displayType]["script"][i].as<JsonObject>()) {
            if (String(kv.key().c_str()).compareTo("cmd") != 0) {
                vars[kv.key().c_str()] = kv.value().as<String>();
                // (String(vars.memoryUsage())+"/"+String(vars.capacity()));
            }
        }

        // Do variable substitutions
        // int counter=0;
        otsDEBUG("Do variable substitutions") for (const auto &kv : vars.as<JsonObject>()) {
            // if (counter>50) break;
            String key = kv.key().c_str();
            if (kv.value().as<String>().indexOf("${") >= 0) {
                vars[key] = otsReplaceVars(vars, vars[key].as<String>());
            }
            // counter++;
        }

        // Execute Command
        otsDEBUG("Execute Command");
        const auto &cmd = doc["displays"][displayType]["script"][i]["cmd"];
        // cmd drawString
        if (cmd.as<String>() == "drawString") {
            otsDEBUG("processDisplayType: cmd: " + cmd.as<String>());
            otsDrawString(spr, vars);

        } else if (cmd.as<String>() == "drawBox") {  // cmd drawBox
            otsDEBUG("processDisplayType: cmd: " + cmd.as<String>());
            otsDrawBox(spr, vars);

        } else if (cmd.as<String>() == "drawRoundBox") {  // cmd drawBox
            otsDEBUG("processDisplayType: cmd: " + cmd.as<String>());
            otsDrawRoundBox(spr, vars);

        } else if (cmd.as<String>() == "drawLine") {  // cmd drawLine
            otsDEBUG("processDisplayType: cmd: " + cmd.as<String>());
            otsDrawLine(spr, vars);

        } else if (cmd.as<String>() == "drawCircle") {  // cmd drawLine
            otsDEBUG("processDisplayType: cmd: " + cmd.as<String>());
            otsDrawCircle(spr, vars);

        } else if (cmd.as<String>() == "drawQR") {  // cmd drawQR
            otsDEBUG("processDisplayType: cmd: " + cmd.as<String>());
            otsDrawQR(spr, vars);

        } else if (cmd.as<String>() == "varSetTimeString") {
            otsDEBUG("processDisplayType: cmd: " + cmd.as<String>());
            otsVarSetTimeString(vars);
        }
        if (cmd.as<String>() == "varSetGeoLocation") {
#ifdef WEATHER
            otsDEBUG("processDisplayType: cmd: " + cmd.as<String>());
            otsVarSetGeoLocation(vars);
#endif

        } else if (cmd.as<String>() == "varSetWeather") {
#ifdef WEATHER
            otsDEBUG("processDisplayType: cmd: " + cmd.as<String>());
            otsVarSetWeather(vars);
#endif

        } else if (cmd.as<String>() == "varSetTimeNow") {
            otsDEBUG("processDisplayType: cmd: " + cmd.as<String>());
            otsVarSetTimeNow(vars);

        } else if (cmd.as<String>() == "fileDelete") {
            otsDEBUG("processDisplayType: cmd: " + cmd.as<String>());
            otsFileDelete(vars);

        } else if (cmd.as<String>() == "fileRename") {
            otsDEBUG("processDisplayType: cmd: " + cmd.as<String>());
            otsFileRename(vars);

        } else if (cmd.as<String>() == "fileCopy") {
            otsDEBUG("processDisplayType: cmd: " + cmd.as<String>());
            otsFileCopy(vars);

        } else if (cmd.as<String>() == "varReadJsonObjFromUrl") {
            otsDEBUG("processDisplayType: cmd: " + cmd.as<String>());
            varReadJsonObjFromUrl(vars, vars["url"].as<String>());

        } else if (cmd.as<String>() == "varReadStringFromUrl") {
            otsDEBUG("processDisplayType: cmd: " + cmd.as<String>());
            varReadStringFromUrl(vars);

        } else if (cmd.as<String>() == "varSetPreceptionForecast") {
            // vars["lat"].as<String>();
            // vars["lon"].as<String>();
            otsDEBUG("processDisplayType: cmd: " + cmd.as<String>());
            otsVarSetPreceptionForecast(vars);

        } else if ((cmd.as<String>() == "mathIsGreater") || (cmd.as<String>() == ">")) {
            otsDEBUG("processDisplayType: cmd: " + cmd.as<String>());
            otsMathIsGreater(vars);

        } else if ((cmd.as<String>() == "mathIsEqual") || (cmd.as<String>() == "=")) {
            otsDEBUG("processDisplayType: cmd: " + cmd.as<String>());
            otsMathIsEqual(vars);

        } else if ((cmd.as<String>() == "mathIsSmaller") || (cmd.as<String>() == "<")) {
            otsDEBUG("processDisplayType: cmd: " + cmd.as<String>());
            otsMathIsSmaller(vars);

        } else if ((cmd.as<String>() == "mathIsEqualOrSmaller") || (cmd.as<String>() == "<=")) {
            otsDEBUG("processDisplayType: cmd: " + cmd.as<String>());
            otsMathIsEqualOrSmaller(vars);

        } else if ((cmd.as<String>() == "mathIsEqualOrGreater") || (cmd.as<String>() == ">=")) {
            otsDEBUG("processDisplayType: cmd: " + cmd.as<String>());
            otsMathIsEqualOrGreater(vars);

        } else if ((cmd.as<String>() == "mathMultiply") || (cmd.as<String>() == "*")) {
            otsDEBUG("processDisplayType: cmd: " + cmd.as<String>());
            otsMathMultiply(vars);

        } else if ((cmd.as<String>() == "mathDivide") || (cmd.as<String>() == "/")) {
            otsDEBUG("processDisplayType: cmd: " + cmd.as<String>());
            otsMathDivide(vars);

        } else if ((cmd.as<String>() == "mathAdd") || (cmd.as<String>() == "+")) {
            otsDEBUG("processDisplayType: cmd: " + cmd.as<String>());
            otsMathAdd(vars);
            otsDEBUG("processDisplayType: cmd: " + cmd.as<String>() + " finished.");

        } else if ((cmd.as<String>() == "mathSubtract") || (cmd.as<String>() == "-")) {
            otsDEBUG("processDisplayType: cmd: " + cmd.as<String>());
            otsMathSubtract(vars);

        } else if ((cmd.as<String>() == "mathModulo") || (cmd.as<String>() == "%")) {
            otsDEBUG("processDisplayType: cmd: " + cmd.as<String>());
            otsMathModulo(vars);

        } else if (cmd.as<String>() == "callSub") {
            otsDEBUG("processDisplayType: cmd: " + cmd.as<String>());
            processDisplayType(vars["name"].as<String>(), vars, doc, spr, taginfo, imageParams);

        } else if (cmd.as<String>() == "for") {
            otsDEBUG("processDisplayType: cmd: " + cmd.as<String>());
            int from = vars["from"].as<int>();
            int to = vars["to"].as<int>();
            int step = vars["step"].as<int>();

            if ((from < to) && (step > 0)) {
                for (int i = from; i <= to; i += step) {
                    vars[vars["indexKey"].as<String>()] = i;
                    processDisplayType(vars["name"].as<String>(), vars, doc, spr, taginfo, imageParams);
                    vTaskDelay(10 / portTICK_PERIOD_MS);  // add a small delay to allow other threads to run
                }
            } else if ((from > to) && (step < 0)) {
                for (int i = from; i >= to; i += step) {
                    vars[vars["indexKey"].as<String>()] = i;
                    processDisplayType(vars["name"].as<String>(), vars, doc, spr, taginfo, imageParams);
                    vTaskDelay(10 / portTICK_PERIOD_MS);  // add a small delay to allow other threads to run
                }
            }

        } else if (cmd.as<String>() == "log") {
            otsDEBUG("processDisplayType: cmd: " + cmd.as<String>());
            wsLog(vars["text"].as<String>());

        } else if (cmd.as<String>() == "error") {
            otsDEBUG("processDisplayType: cmd: " + cmd.as<String>());
            wsErr(vars["text"].as<String>());
        }
        otsDEBUG("loop end");
        vTaskDelay(10 / portTICK_PERIOD_MS);  // add a small delay to allow other threads to run
    }

    otsDEBUG("processDisplayType: Stop");
    return true;
}