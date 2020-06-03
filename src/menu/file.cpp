
#include "file.h"

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
    
    setSize(fileall.size());
    
    Serial.printf("fileall: %d\r\n", fileall.size());
}

MenuFile::~MenuFile() {
    Serial.println("file end");
    
}

void MenuFile::getStr(menu_dspl_el_t &str, int16_t i) {
    Serial.printf("MenuFile::getStr: %d (sz=%d)\r\n", i, fileall.size());
    auto const &f = fileall[i];
    strncpy(str.name, f.name, sizeof(str.name));
    str.name[sizeof(str.name)-1] = '\0';
    
    if (f.sz >= 1024)
        snprintf_P(str.val, sizeof(str.val), PSTR("%dk"), f.sz/1024);
    else
        snprintf_P(str.val, sizeof(str.val), PSTR("%d "), f.sz);
}

void MenuFile::btnSmp() {
    if (sel() >= 0)
        menuFlashP(PSTR("Hold to remove"));
}
void MenuFile::btnLng() {
    if (sel() >= 0) {
        auto i = sel();
        auto f = fileall[i];
        if (!SPIFFS.remove(f.name)) {
            menuFlashP(PSTR("Remove fail"));
            return;
        }
        fileall.erase(fileall.begin() + i);
        setSize(fileall.size());
        
        menuFlashP(PSTR("Remove OK"));
        updStr();
    }
}
