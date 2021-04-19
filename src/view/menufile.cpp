
#include "menu.h"
#include "main.h"

#include "../file/log.h"
#include "../file/track.h"

#include <Arduino.h>
#include <SPIFFS.h>
#include <vector>

typedef struct {
    char name[33];
    size_t sz;
} filei_t;


/* ------------------------------------------------------------------------------------------- *
 *  MenuFile
 * ------------------------------------------------------------------------------------------- */
class ViewMenuFile : public ViewMenu {
    public:
        void restore() {
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
    
            setSize(fileall.size()+2);
    
            Serial.printf("fileall: %d\r\n", fileall.size());
            
            ViewMenu::restore();
        }
        
        void close() {
            fileall.clear();
            setSize(0);
        }
        
        void getStr(menu_dspl_el_t &str, int16_t i) {
            Serial.printf("ViewMenuFile::getStr: %d (sz=%d)\r\n", i, fileall.size());
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
        
        void btnSmpl(btn_code_t btn) {
            // тут обрабатываем только BTN_SEL,
            // нажатую на любом не-exit пункте
            if ((btn != BTN_SEL) || isExit(isel)) {
                if (btn == BTN_SEL) close();
                ViewMenu::btnSmpl(btn);
                return;
            }

            switch (sel()) {
                case 0:
                case 1:
                    menuFlashP(PSTR("Hold to ReNum"));
                    return;
                default:
                    menuFlashP(PSTR("Hold to remove"));
            }
        }
        
        bool useLong(btn_code_t btn) {
            if ((btn != BTN_SEL) || isExit(isel))
                return false;
            return true;
        }
        
        void btnLong(btn_code_t btn) {
            if ((btn != BTN_SEL) || isExit(isel))
                return;
            
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
            Serial.printf("ViewMenuFile::btnLng: removing: %s\r\n", f.name);
            if (!SPIFFS.remove(f.name)) {
                menuFlashP(PSTR("Remove fail"));
                return;
            }
            fileall.erase(fileall.begin() + i);
            setSize(fileall.size()+2);
    
            menuFlashP(PSTR("Remove OK"));
            updStr();
        }
        
        void process() {
            if (btnIdle() > MENU_TIMEOUT) {
                close();
                setViewMain();
            }
        }
        
    private:
        std::vector<filei_t> fileall;
};

static ViewMenuFile vMenuFile;
ViewMenu *menuFile() { return &vMenuFile; }
