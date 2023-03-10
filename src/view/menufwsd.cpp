

#include "menu.h"

#if HWVER >= 5
#include "main.h"

#include "../log.h"
#include "../sys/sys.h"
#include "../core/file.h"

#include <SD.h>
#include <vector>

/* ------------------------------------------------------------------------------------------- *
 *  ViewMenuFwSdCard
 * ------------------------------------------------------------------------------------------- */
class ViewMenuFwSdCard : public ViewMenu {
    typedef struct {
        char name[47];
        bool md5;
        size_t sz;
    } fwi_t;

    std::vector<fwi_t> fileall;

    public:
        bool isActive() { return true; }
        
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
        }
        
        void close() {
            fileExtStop();
            fileall.clear();
            setSize(0);
        }
        
        void getStr(line_t &str, int16_t i) {
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
            
            operRun<WrkFwUpdCard>(PTXT(FWSD_TITLE), f.name);
        }
        
        void process() {
            if (btnIdle() > MENU_TIMEOUT) {
                close();
                menuClear();
            }
        }
};

void menuFwSdCard() { menuOpen<ViewMenuFwSdCard>(); }

#endif // HWVER >= 5
