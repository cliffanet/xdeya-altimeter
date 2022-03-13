
#include "menu.h"
#include "main.h"

#include "../log.h"
#include "../jump/logbook.h"
#include "../jump/track.h"

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
            CONSOLE("Listing directory: %s", dirname);

            File root = fs.open(dirname);
            if(!root){
                CONSOLE("- failed to open directory");
                return;
            }
            if(!root.isDirectory()){
                CONSOLE(" - not a directory");
                return;
            }

            File fh;
            while (fh = root.openNextFile()) {
                if (fh.isDirectory())
                    continue;
                /*
                    CONSOLE("  DIR : %s", file.name());
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
    
            CONSOLE("fileall: %d", fileall.size());
            
            ViewMenu::restore();
        }
        
        void close() {
            fileall.clear();
            setSize(0);
        }
        
        void getStr(menu_dspl_el_t &str, int16_t i) {
            CONSOLE("ViewMenuFile::getStr: %d (sz=%d)", i, fileall.size());
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
#if !defined(FWVER_DEV) && !defined(FWVER_DEBUG)
                default:
                    menuFlashP(PSTR("Hold to remove"));
#endif
            }

#if defined(FWVER_DEV) || defined(FWVER_DEBUG)
            auto i = sel()-2;
            auto f = fileall[i];
            CONSOLE("ViewMenuFile::btnSmpl: dump: %s", f.name);
            File fh = SPIFFS.open(f.name, FILE_READ);
            if (!fh) {
                CONSOLE("ViewMenuFile::btnSmpl: open fail");
                return;
            }
            
            for (int n = 0; n < 64; n++) {
                uint8_t buf[16];
                if (fh.read(buf, sizeof(buf)) != sizeof(buf)) {
                    CONSOLE("ViewMenuFile::btnSmpl: read fail");
                    break;
                }
                uint8_t *b = buf;
                char str[128];
                char *s = str;
                for (int n1 = 0; n1 < sizeof(buf); n1++) {
                    sprintf_P(s, PSTR("%02X "), *b);
                    b++;
                    s+=3;
                }
                b = buf;
                for (int n1 = 0; n1 < sizeof(buf); n1++) {
                    *s = *b >= 0x20 ? *b : '.';
                    b++;
                    s++;
                }
                *s = '\0';
                
                CONSOLE("dump[%02x]: %s", n, str);
            }
            
            fh.close();
#endif
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
                    menuFlashP(PSTR("ReNum begin..."));
                    displayUpdate();
                    if (FileLogBook().renum())
                        menuFlashP(PSTR("ReNum OK"));
                    else
                        menuFlashP(PSTR("ReNum fail"));
                    updStr();
                    return;
                case 1:
                    menuFlashP(PSTR("ReNum begin..."));
                    displayUpdate();
                    if (FileTrack().renum())
                        menuFlashP(PSTR("ReNum OK"));
                    else
                        menuFlashP(PSTR("ReNum fail"));
                    updStr();
                    return;
            }
    
            auto i = sel()-2;
            auto f = fileall[i];
            CONSOLE("ViewMenuFile::btnLng: removing: %s", f.name);
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
