
#if HWVER >= 5

#include "menu.h"
#include "main.h"

#include "../log.h"
#include "../jump/logbook.h"
#include "../jump/track.h"
#include "../core/file.h"
#include "../clock.h"

#include <Update.h>         // Обновление прошивки
#include <SD.h>
#include <vector>

#define ERR(s)              fin(PSTR(TXT_FWSD_ERR_ ## s))

class ViewFwUpd : public ViewBase {
    public:
        // старт процедуры синхронизации
        void begin(const char *fname) {
            char name[64], md5[64];
            snprintf_P(name, sizeof(name), PSTR("/%s"), fname);
            
            // md5
            strcpy(md5, name);
            int len = strlen(md5);
            md5[len-3] = 'm';
            md5[len-2] = 'd';
            md5[len-1] = '5';
            if (SD.exists(md5)) {
                if (!(fh = SD.open(md5))) {
                    ERR(FILEOPEN);
                    return;
                }
                if (fh.read(reinterpret_cast<uint8_t *>(md5), 32) != 32) {
                    ERR(FILEREAD);
                    return;
                }
                md5[32] = '\0';
                if (strlen(md5) != 32) {
                    ERR(MD5);
                    return;
                }
                usemd5 = true;
            }
            else {
                usemd5 = false;
            }
            
            if (!(fh = SD.open(name))) {
                ERR(FILEOPEN);
                return;
            }
            
            // size
            uint32_t freesz = ESP.getFreeSketchSpace();
            uint32_t cursz = ESP.getSketchSize();
            CONSOLE("current fw size: %lu, avail size for new fw: %lu, new fw size: %lu; md5: %s", 
                    cursz, freesz, fh.size(), md5);
            
            if (fh.size() > freesz) {
                ERR(FWSIZEBIG);
                return;
            }

            // start burn
            if (!Update.begin(fh.size(), U_FLASH)) {
                CONSOLE("Upd begin fail: errno=%d", Update.getError());
                ERR(FWINIT);
                return;
            }
            if (usemd5 && !Update.setMD5(md5)) {
                CONSOLE("Upd md5 fail: errno=%d", Update.getError());
                ERR(MD5);
                return;
            }
        }
        
        void close() {
            fh.close();
            fileExtStop();
        }
        
        // вывод сообщения
        void msg(const char *_title) {
            if (_title == NULL)
                title[0] = '\0';
            else {
                strncpy_P(title, _title, sizeof(title));
                title[sizeof(title)-1] = '\0';
                CONSOLE("msg: %s", title);
            }
            displayUpdate();
        }
        
        // завершение всего процесса, но выход не сразу, а через несколько сек,
        // чтобы прочитать сообщение
        void fin(const char *_title) {
            close();
            msg(_title);
        }
        
        // фоновый процессинг - обрабатываем этапы синхронизации
        void process() {
            if (!fh)
                return;
                
            uint8_t buf[512];
            auto m = millis();
            
            while (fh.available() > 0) {
                if ((millis()-m) > 90)
                    return;
                int sz = fh.read(buf, sizeof(buf));
            
                if (sz < 1) {
                    ERR(FILEREAD);
                    return;
                }
                if (Update.write(buf, sz) != sz) {
                    ERR(FWWRITE);
                    return;
                }
            }

            if (!Update.end()) {
                ERR(FWFIN);
                return;
            }
            
            ESP.restart();
        }
        
        void btnSmpl(btn_code_t btn) {
            // короткое нажатие на любую кнопку ускоряет выход,
            // если процес завершился и ожидается таймаут финального сообщения
            if (!fh) {
                close();
                setViewMain();
            }
        }
        
        bool useLong(btn_code_t btn) {
            return btn == BTN_SEL;
        }
        void btnLong(btn_code_t btn) {
            // длинное нажатие из любой стадии завершает процесс синхнонизации принудительно
            if (btn != BTN_SEL)
                return;
            ERR(CANCEL);
        }
        
        // отрисовка на экране
        void draw(U8G2 &u8g2) {
            u8g2.setFont(menuFont);
    
            // Заголовок
            u8g2.setDrawColor(1);
            u8g2.drawBox(0,0,u8g2.getDisplayWidth(),12);
            u8g2.setDrawColor(0);
            char s[64];
            strcpy_P(s, PTXT(FWSD_TITLE));
            u8g2.drawTxt((u8g2.getDisplayWidth()-u8g2.getTxtWidth(s))/2, 10, s);
            
            u8g2.setDrawColor(1);
            int8_t y = 10-1+14;
            
            if (fh)
                u8g2.drawTxt(0, y, fh.name());
            
            y += 20;
            u8g2.drawTxt((u8g2.getDisplayWidth()-u8g2.getTxtWidth(title))/2, y, title);

            y += 10;
            if (fh.size() > 0) {
                uint8_t p = fh.position() * 20 / fh.size();
                char *ss = s;
                *ss = '|';
                ss++;
                for (uint8_t i=0; i< 20; i++, ss++)
                    *ss = i < p ? '.' : ' ';
                *ss = '|';
                ss++;
                *ss = '\0';
                u8g2.drawStr((u8g2.getDisplayWidth()-u8g2.getStrWidth(s))/2, y, s);
            }
        }
        
    private:
#if defined(FWVER_LANG) && (FWVER_LANG == 'R')
        char title[50];
#else
        char title[24];
#endif
        File fh;
        bool usemd5;
};
ViewFwUpd vFwUpd;



typedef struct {
    char name[47];
    bool md5;
    size_t sz;
} fwi_t;


/* ------------------------------------------------------------------------------------------- *
 *  MenuFile
 * ------------------------------------------------------------------------------------------- */
class ViewMenuFwSdCard : public ViewMenu {
    public:
        void restore() {
            fileExtInit();
            fs::FS &fs = SD;
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
                
                const auto name = fh.name();
                int len = strlen(name);
                if ((len<5) || (len >= sizeof(fwi_t::name)) ||
                    (name[len-4] != '.') ||
                    ((name[len-3] != 'b') && (name[len-3] != 'B')) ||
                    ((name[len-2] != 'i') && (name[len-2] != 'I')) ||
                    ((name[len-1] != 'n') && (name[len-1] != 'N')))
                    continue;
        
                fwi_t f;
                strncpy(f.name, name, sizeof(f.name)-1);
                f.name[sizeof(f.name)-1]='\0';
                
                char fn[sizeof(f.name)+1];
                fn[0] = '/';
                strncpy(fn+1, name, sizeof(fn)-1);
                fn[sizeof(fn)-1]='\0';
                fn[len-2] = 'm';
                fn[len-1] = 'd';
                fn[len]   = '5';
                f.md5 = SD.exists(fn);
                
                f.sz = fh.size();
                fileall.push_back(f);
            }
    
            setSize(fileall.size());
    
            CONSOLE("fileall: %d", fileall.size());
            
            ViewMenu::restore();
        }
        
        void close() {
            fileExtStop();
            fileall.clear();
            setSize(0);
        }
        
        void getStr(menu_dspl_el_t &str, int16_t i) {
            CONSOLE("ViewMenuFwSdCard::getStr: %d (sz=%d)", i, fileall.size());
    
            auto const &f = fileall[i];
            const size_t sz = sizeof(str.name) < 30 ? sizeof(str.name) : 30;
            snprintf(str.name, sz, PSTR("%c %s"), f.md5 ? '*' : ' ', f.name);

            if (f.sz >= 1024*1024)
                snprintf_P(str.val, sizeof(str.val), PSTR("%0.1fM"), static_cast<double>(f.sz)/1024/1024);
            else
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
            
            menuFlashP(PSTR("Hold to FirmWare"));
        }
        
        bool useLong(btn_code_t btn) {
            if ((btn != BTN_SEL) || isExit(isel))
                return false;
            return true;
        }
        
        void btnLong(btn_code_t btn) {
            if ((btn != BTN_SEL) || isExit(isel))
                return;
            
            const auto &f = fileall[sel()];
            
            viewSet(vFwUpd);
            vFwUpd.begin(f.name);
        }
        
        void process() {
            if (btnIdle() > MENU_TIMEOUT) {
                close();
                setViewMain();
            }
        }
        
    private:
        std::vector<fwi_t> fileall;
};

static ViewMenuFwSdCard vMenuFwSdCard;
ViewMenu *menuFwSdCard() { return &vMenuFwSdCard; }


bool menuIsFwSd() {
    return viewIs(vFwUpd);
}

#endif // HWVER >= 5
