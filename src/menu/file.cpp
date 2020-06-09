
#include "file.h"
#include "../file/log.h"
#include "../file/track.h"

#include <Arduino.h>
#include <SPIFFS.h>


/* ------------------------------------------------------------------------------------------- *
 *  Описание методов шаблона класса MenuFile
 * ------------------------------------------------------------------------------------------- */
MenuFile::MenuFile() :
    MenuBase(PSTR("WiFi Sync"))
{
    fs::FS &fs = SPIFFS;
    const char *dir = PSTR("/");
    char dirname[10];
    
    strcpy_P(dirname, dir);
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File fh;
    while (fh = root.openNextFile()) {
        if (fh.isDirectory())
            continue;
        /*
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
        */
        
        filei_t f;
        strncpy(f.name, fh.name(), sizeof(f.name));
        f.name[sizeof(f.name)-1]='\0';
        f.sz = fh.size();
        fileall.push_back(f);
    }
    
    updSize();
    
    Serial.printf("fileall: %d\r\n", fileall.size());
}

void MenuFile::updSize() {
    setSize(fileall.size()+2);
}

void MenuFile::getStr(menu_dspl_el_t &str, int16_t i) {
    Serial.printf("MenuFile::getStr: %d (sz=%d)\r\n", i, fileall.size());
    switch (i) {
        case 0:
            strncpy_P(str.name, PSTR("LogBook ReNum"), sizeof(str.name));
            str.val[0] = '\0';
            return;
        case 1:
            strncpy_P(str.name, PSTR("Track ReNum"), sizeof(str.name));
            str.val[0] = '\0';
            return;
    }
    
    auto const &f = fileall[i-2];
    strncpy(str.name, f.name, sizeof(str.name));
    str.name[sizeof(str.name)-1] = '\0';
    
    if (f.sz >= 1024)
        snprintf_P(str.val, sizeof(str.val), PSTR("%dk"), f.sz/1024);
    else
        snprintf_P(str.val, sizeof(str.val), PSTR("%d "), f.sz);
}

void MenuFile::btnSmp() {
    switch (sel()) {
        case 0:
        case 1:
            menuFlashP(PSTR("Hold to ReNum"));
            return;
        default:
            menuFlashP(PSTR("Hold to remove"));
    }
}
void MenuFile::btnLng() {
    switch (sel()) {
        case 0:
            if (logRenum(PSTR(JMPLOG_SIMPLE_NAME)))
                menuFlashP(PSTR("ReNum OK"));
            else
                menuFlashP(PSTR("ReNum fail"));
            updStr();
            return;
        case 1:
            if (!logRenum(PSTR(TRK_FILE_NAME)))
                menuFlashP(PSTR("ReNum OK"));
            else
                menuFlashP(PSTR("ReNum fail"));
            updStr();
            return;
    }
    
    auto i = sel()-2;
    auto f = fileall[i];
    Serial.printf("MenuFile::btnLng: removing: %s\r\n", f.name);
    if (!SPIFFS.remove(f.name)) {
        menuFlashP(PSTR("Remove fail"));
        return;
    }
    fileall.erase(fileall.begin() + i);
    updSize();
    
    menuFlashP(PSTR("Remove OK"));
    updStr();
}
