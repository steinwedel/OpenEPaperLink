#include "ots/otsDraw.h"

#include <Arduino.h>
#include <TJpg_Decoder.h>

#include "ots/otsContentmanager.h"
#include "qrcode.h"
#include "storage.h"
#include "truetype.h"

void otsDrawBox(TFT_eSprite &spr, DynamicJsonDocument &vars) {
    int16_t posx = vars["posX"].as<int16_t>();
    int16_t posy = vars["posY"].as<int16_t>();
    int16_t width = vars["width"].as<int16_t>();
    int16_t height = vars["height"].as<int16_t>();
    bool filled = vars["filled"].as<String>()=="true";

    if (width < 0) {
        posx += width;
        width *= -1;
    }

    if (height < 0) {
        posy += height;
        height *= -1;
    }

    // wsErr("x: "+ String(posx)+ " y: "+String(posy));

    uint16_t color = otsGetColor(vars["color"].as<String>());

    if (filled) {
        spr.fillRect(posx, posy, width, height, color);
    } else {
        spr.drawRect(posx, posy, width, height, color);
    }
    
    vars["posX"] = posx + width;
    vars["posY"] = posy + height;
}

void otsDrawRoundBox(TFT_eSprite &spr, DynamicJsonDocument &vars) {
    int16_t posx = vars["posX"].as<int16_t>();
    int16_t posy = vars["posY"].as<int16_t>();
    int16_t width = vars["width"].as<int16_t>();
    int16_t height = vars["height"].as<int16_t>();
    bool filled = vars["filled"].as<String>()=="true";
    int16_t radius = vars["radius"].as<uint16_t>();
    uint16_t color = otsGetColor(vars["color"].as<String>());
    
    if (width < 0) {
        posx += width;
        width *= -1;
    }

    if (height < 0) {
        posy += height;
        height *= -1;
    }

    // wsErr("x: "+ String(posx)+ " y: "+String(posy));
    
    if (filled) {
        //spr.fillRoundRect(posx, posy, width, height, radius, color);
        spr.fillSmoothRoundRect(posx, posy, width, height, radius, color);
    } else {
        spr.drawRoundRect(posx, posy, width, height, radius, color);
    }
    vars["posX"] = posx + width;
    vars["posY"] = posy + height;
}


void otsDrawLine(TFT_eSprite &spr, DynamicJsonDocument &vars) {
    int16_t posx = vars["posX"].as<uint16_t>();
    int16_t posy = vars["posY"].as<uint16_t>();
    int16_t postox = vars["posToX"].as<uint16_t>();
    int16_t postoy = vars["posToY"].as<uint16_t>();
    int16_t width = vars["width"].as<uint16_t>();
    if (width<1) width=1;

    uint16_t color = otsGetColor(vars["color"]);

    //spr.drawLine(posx, posy, postox, postoy, color);
    spr.drawWideLine(posx, posy, postox, postoy, width, color);
    vars["posX"] = postox;
    vars["posY"] = postoy;
}

void otsDrawCircle(TFT_eSprite &spr, DynamicJsonDocument &vars) {
    int16_t posx = vars["posX"].as<uint16_t>();
    int16_t posy = vars["posY"].as<uint16_t>();
    int16_t radius = vars["radius"].as<uint16_t>();
    bool filled = vars["filled"].as<String>()=="true";
    uint16_t color = otsGetColor(vars["color"]);

    spr.fillCircle(posx, posy, radius, color);
    if (filled) {
        spr.fillCircle(posx, posy, radius, color);
    } else {
        spr.drawCircle(posx, posy, radius, color);
    }
}


void otsDrawQR(TFT_eSprite &spr, DynamicJsonDocument &vars) {
    Storage.begin();

    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(2)];
    // https://github.com/ricmoo/QRCode
    qrcode_initText(&qrcode, qrcodeData, 2, ECC_MEDIUM, vars["value"]);

    const int size = qrcode.size;
    // const int dotsize = int((vars["displayHeight"].as<int>() - vars["posX"].as<int>()) / size);
    const int dotsize = vars["size"].as<int>();
    byte align = otsGetAlign(vars["align"]);

    int xpos = vars["posX"].as<int>();
    int ypos = vars["posY"].as<int>();
    uint16_t color = otsGetColor(vars["color"]);

    switch (align) {
        case TL_DATUM:
            break;

        case TC_DATUM:
            xpos = xpos - (dotsize * size / 2);
            break;

        case TR_DATUM:
            xpos = xpos - (dotsize * size);
            break;
        case BL_DATUM:
            ypos = ypos - (dotsize * size);
            break;

        case BC_DATUM:
            xpos = xpos - (dotsize * size / 2);
            ypos = ypos - (dotsize * size);
            break;

        case BR_DATUM:
            xpos = xpos - (dotsize * size);
            ypos = ypos - (dotsize * size);
            break;
    }

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            if (qrcode_getModule(&qrcode, x, y)) {
                spr.fillRect(xpos + x * dotsize, ypos + y * dotsize, dotsize, dotsize, color);
            }
        }
    }
}

void otsDrawString(TFT_eSprite &spr, DynamicJsonDocument &vars) {
    int16_t posx = vars["posX"].as<int16_t>();
    int16_t posy = vars["posY"].as<int16_t>();
    uint16_t color = otsGetColor(vars["color"].as<String>());
    uint16_t bgColor = otsGetColor(vars["bgColor"].as<String>());
    byte align = otsGetAlign(vars["align"].as<String>());
    switch (otsProcessFontType(vars["font"])) {
        case 1: {
            // u8g2 font
            U8g2_for_TFT_eSPI u8f;
            u8f.begin(spr);
            setU8G2Font(vars["font"].as<String>(), u8f);
            u8f.setForegroundColor(color);
            u8f.setBackgroundColor(bgColor);
            if (align == TC_DATUM) {
                posx -= u8f.getUTF8Width(vars["value"]) / 2;
            }
            if (align == TR_DATUM) {
                posx -= u8f.getUTF8Width(vars["value"]);
            }
            u8f.setCursor(posx, posy);
            u8f.print(vars["value"].as<String>());
            vars["posX"] = u8f.getCursorX();
            vars["posY"] = u8f.getCursorY();
        } break;
        case 2: {
            // truetype
            time_t t = millis();
            truetypeClass truetype = truetypeClass();
            void *framebuffer = spr.getPointer();
            truetype.setFramebuffer(spr.width(), spr.height(), spr.getColorDepth(), static_cast<uint8_t *>(framebuffer));
            File fontFile = contentFS->open(vars["font"].as<String>(), "r");
            if (!truetype.setTtfFile(fontFile)) {
                Serial.println("read ttf failed");
                return;
            }
            truetype.setCharacterSize(vars["size"].as<int>());
            truetype.setCharacterSpacing(0);
            if (align == TC_DATUM) {
                posx -= truetype.getStringWidth(vars["value"].as<String>()) / 2;
            }
            if (align == TR_DATUM) {
                posx -= truetype.getStringWidth(vars["value"].as<String>());
            }
            truetype.setTextBoundary(posx, spr.width(), spr.height());
            truetype.setTextColor(spr.color16to8(color), spr.color16to8(color));
            truetype.textDraw(posx, posy, vars["value"].as<String>());
            // Serial.println("Finish");
            truetype.end();
            // vars["posX"]=posx+truetype.getStringWidth(String(vars["value"].as<String>()));
            // todo
            //  richtige Position ermitteln. Gibt es eine Art von Cursor?
        } break;
        case 3: {
            // vlw bitmap font
            spr.setTextDatum(align);
            if (vars["font"].as<String>() != "") spr.loadFont(vars["font"].as<String>().substring(1), *contentFS);
            spr.setTextColor(color, color);
            spr.drawString(vars["value"].as<String>(), posx, posy);
            vars["posX"] = spr.getCursorX();
            vars["posY"] = spr.getCursorY();
            if (vars["font"].as<String>() != "") spr.unloadFont();
        }
    }
}

// Helper functions
uint16_t otsGetColor(String color) {
    if (color == "0" || color == "white") return TFT_WHITE;
    if (color == "1" || color == "" || color == "black") return TFT_BLACK;
    if (color == "2" || color == "red") return TFT_RED;
    uint16_t r, g, b;
    if (color.length() == 7 && color[0] == '#' &&
        sscanf(color.c_str(), "#%2hx%2hx%2hx", &r, &g, &b) == 3) {
        return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
    }
    return TFT_WHITE;
}

byte otsGetAlign(const String &align) {
    if (align.indexOf("center") == 0) {
        return TC_DATUM;
    }

    if (align.indexOf("right") == 0) {
        return TR_DATUM;
    }

    if (align.indexOf("topLeft") == 0) {
        return TL_DATUM;
    }

    if (align.indexOf("topRight") == 0) {
        return TR_DATUM;
    }

    if (align.indexOf("topCenter") == 0) {
        return TC_DATUM;
    }

    if (align.indexOf("bottomLeft") == 0) {
        return BL_DATUM;
    }

    if (align.indexOf("bottomRight") == 0) {
        return BR_DATUM;
    }

    if (align.indexOf("bottomCenter") == 0) {
        return BC_DATUM;
    }
    return TL_DATUM;
}

uint8_t otsProcessFontType(String font) {
    if (font == "") return 3;
    if (font == "glasstown_nbp_tf") return 1;
    if (font == "7x14_tf") return 1;
    if (font == "t0_14b_tf") return 1;
    // if (font.indexOf('/') == -1) font = "/fonts/" + font;
    // if (!font.startsWith("/")) font = "/" + font;
    // if (font.endsWith(".vlw")) font = font.substring(0, font.length() - 4);
    if (font.endsWith(".ttf")) return 2;
    return 3;
}