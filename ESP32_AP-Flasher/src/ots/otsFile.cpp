#include "ots/otsFile.h"

#include <HTTPClient.h>

#include "storage.h"
#include "util.h"

bool otsFileDelete(DynamicJsonDocument &vars) {
    Storage.begin();
    return contentFS->remove(vars["filename"].as<String>());
}

bool otsFileRename(DynamicJsonDocument &vars) {
    Storage.begin();
    return contentFS->rename(vars["filename"].as<String>(), vars["filenameNew"].as<String>());
}

int otsFileCopy(DynamicJsonDocument &vars) {
    Storage.begin();

    String target = vars["target"].as<String>();
    String source = vars["source"].as<String>();
    bool overwrite = vars["overwrite"].as<String>()=="true";
    time_t fetched = vars["fetched"].as<time_t>();

    if ((overwrite == false) && contentFS->exists(target)) {
        return 409;  // Conflict! File already exists
    }

    if (util::isEmptyOrNull(source)) {
        wsLog("Empty Url");
        return false;
    }

    if (source.indexOf("://") == -1) {
        source = String("file://" + source);
    }

    // read json from file
    if (source.startsWith("file://")) {
        source.replace("file://", "");

        // add leading slash or other start point in front of filename
        if (!source.startsWith("/")) {
            source = "/" + source;
        }

        File sourceFile = contentFS->open(source, "r");
        if (!sourceFile) {
            wsErr("File " + source + "  not found");
            return false;
        }

        File targetFile = contentFS->open(target, FILE_WRITE);
        if (!targetFile) {
            wsErr("File " + target + "  could not be open for writing");
            return false;
        }

        targetFile.write(sourceFile.read());
        sourceFile.close();
        targetFile.close();
        return true;

        // read json from http:// or https:// stream
    } else if (source.startsWith("http://") || source.startsWith("https://")) {
        HTTPClient http;
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        http.begin(source);
        http.setTimeout(5000);  // timeout in ms
        const int httpCode = http.GET();
        if (httpCode == 200) {
            File targetFile = contentFS->open(target, "w");
            if (targetFile) {
                http.writeToStream(&targetFile);
                targetFile.close();
            }
        } else {
            if (httpCode != 304) {
                wsErr("http " + source + " " + String(httpCode));
            }
        }
        http.end();
        return httpCode;
    } else {
        wsErr("unsupported protocol format: " + source);  // ftp, etc.
        return false;
    }
    return true;
}