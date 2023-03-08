
#include "menu.h"
#include "main.h"

#include "../log.h"
#include "../jump/logbook.h"

/* ------------------------------------------------------------------------------------------- *
 *  LogBook
 * ------------------------------------------------------------------------------------------- */
// Подробное инфо

class ViewMenuLogBookInfo : public View {
    public:
        void open(size_t _isel, size_t _sz) {
            isel = _isel;
            sz   = _sz;
            read();
        }
        
        void read() {
            readok = FileLogBook().getfull(jmp, isel);
        }
        
        void btnSmpl(btn_code_t btn) {
            switch (btn) {
                case BTN_UP:
                    if (isel <= 0)
                        return;
                    isel --;
                    read();
                    break;
                    
                case BTN_SEL: {
                        auto p = menuRestore();
                        if (p != NULL)
                            p->setSel(isel+1);
                    }
                    break;
                    
                case BTN_DOWN:
                    if ((isel+1) >= sz)
                        return;
                    isel ++;
                    read();
                    break;
            }
        }
        
        void draw(U8G2 &u8g2) {
            u8g2.setDrawColor(1);
            u8g2.drawBox(0,0,u8g2.getDisplayWidth(),12);
            
            if (!readok)
                return;
            
            u8g2.setFont(menuFont);
    
            // Заголовок
            char s[64];
            snprintf_P(s, sizeof(s), PTXT(LOGBOOK_JUMPNUM), jmp.num);
            u8g2.setDrawColor(0);
            u8g2.drawTxt((u8g2.getDisplayWidth()-u8g2.getTxtWidth(s))/2, 10, s);
    
            u8g2.setDrawColor(1);
            int8_t y = 10-1+14;
    
            const auto &tm = jmp.tm;
            snprintf_P(s, sizeof(s), PSTR("%2d.%02d.%04d"), tm.day, tm.mon, tm.year);
            u8g2.drawStr(0, y, s);
            snprintf_P(s, sizeof(s), PSTR("%2d:%02d"), tm.h, tm.m);
            u8g2.drawStr(u8g2.getDisplayWidth()-u8g2.getStrWidth(s), y, s);
    
            y += 10;
            strcpy_P(s, PTXT(LOGBOOK_ALTEXIT));
            u8g2.drawTxt(0, y, s);
            snprintf_P(s, sizeof(s), PSTR("%d"), jmp.beg.alt);
            u8g2.drawTxt(u8g2.getDisplayWidth()-u8g2.getTxtWidth(s), y, s);
    
            y += 10;
            strcpy_P(s, PTXT(LOGBOOK_ALTCNP));
            u8g2.drawTxt(0, y, s);
            snprintf_P(s, sizeof(s), PSTR("%d"), jmp.cnp.alt);
            u8g2.drawTxt(u8g2.getDisplayWidth()-u8g2.getTxtWidth(s), y, s);
    
            y += 10;
            strcpy_P(s, PTXT(LOGBOOK_TIMETOFF));
            u8g2.drawTxt(0, y, s);
            uint32_t toff = jmp.toff.tmoffset/1000;
            snprintf_P(s, sizeof(s), PSTR("%d:%02d"), toff/60, toff % 60);
            u8g2.drawTxt(u8g2.getDisplayWidth()-u8g2.getTxtWidth(s), y, s);
    
            y += 10;
            strcpy_P(s, PTXT(LOGBOOK_TIMEFF));
            u8g2.drawTxt(0, y, s);
            snprintf_P(s, sizeof(s), PTXT(LOGBOOK_TIMESEC), jmp.cnp.tmoffset/1000);
            u8g2.drawTxt(u8g2.getDisplayWidth()-u8g2.getTxtWidth(s), y, s);
    
            y += 10;
            strcpy_P(s, PTXT(LOGBOOK_TIMECNP));
            u8g2.drawTxt(0, y, s);
            snprintf_P(s, sizeof(s), PTXT(LOGBOOK_TIMESEC), (jmp.end.tmoffset-jmp.cnp.tmoffset)/1000);
            u8g2.drawTxt(u8g2.getDisplayWidth()-u8g2.getTxtWidth(s), y, s);
        }
        
        void process() {
            if (btnIdle() > MENU_TIMEOUT)
                menuClear();
        }
        
    private:
        size_t isel=0, sz=0;
        FileLogBook::item_t jmp;
        bool readok = false;
};

static ViewMenuLogBookInfo vLogBookInfo;

// список

#define DCNT 10
class ViewMenuLogBook : public ViewMenu {
    struct {
        int16_t i = -1;
        line_t str;
    } dd[DCNT];
    uint8_t n = 0;

    public:
        void restore() {
            setSize(FileLogBook().sizeall());
        }
        // система кеширования, т.к. весь логбук мы сразу не можем прочесть,
        // работаем простым кешем
        line_t & read(int16_t i) {
            for (auto &d : dd)
                if (i == d.i)
                    return d.str;
            
            auto &d = dd[n];
            d.i = i;
            n++;
            if (n >= DCNT) n=0;

            FileLogBook::item_t jmp;
            if (FileLogBook().getfull(jmp, i)) {
                auto &tm = jmp.tm;
                snprintf_P(d.str.name, sizeof(d.str.name), PSTR("%2d.%02d.%02d %2d:%02d"),
                                tm.day, tm.mon, tm.year % 100, tm.h, tm.m);
                snprintf_P(d.str.val, sizeof(d.str.val), PSTR("%d"), jmp.num);
            }
            else {
                d.str.name[0] = '\0';
                d.str.val[0] = '\0';
            }

            return d.str;
        }
        
        void getStr(line_t &str, int16_t i) {
            str = read(i);
        }
        
        void btnSmpl(btn_code_t btn) {
            // тут обрабатываем только BTN_SEL,
            // нажатую на любом не-exit пункте
            if ((btn != BTN_SEL) || isExit(isel)) {
                ViewMenu::btnSmpl(btn);
                return;
            }
            
            viewSet(vLogBookInfo);
            vLogBookInfo.open(sel(), size());
        }
        
        void process() {
            if (btnIdle() > MENU_TIMEOUT)
                menuClear();
        }
};

void menuLogBook() { menuOpen<ViewMenuLogBook>(); }
