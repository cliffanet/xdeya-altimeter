
#include "menu.h"
#include "main.h"

#include "../file/log.h"
#include "../cfg/jump.h"

/* ------------------------------------------------------------------------------------------- *
 *  LogBook
 * ------------------------------------------------------------------------------------------- */
ViewMenu *menuLogBook();

// Подробное инфо

class ViewMenuLogBookInfo : public ViewBase {
    public:
        void open(size_t _isel, size_t _sz) {
            isel = _isel;
            sz   = _sz;
            read();
        }
        
        void read() {
            readok = logRead(r, PSTR(JMPLOG_SIMPLE_NAME), isel);
        }
        
        void btnSmpl(btn_code_t btn) {
            switch (btn) {
                case BTN_UP:
                    if (isel <= 0)
                        return;
                    isel --;
                    read();
                    break;
                    
                case BTN_SEL:
                    viewSet(*(menuLogBook()));
                    menuLogBook()->restore();
                    menuLogBook()->setSel(isel+1);
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
            u8g2.drawBox(0,0,128,12);
            
            if (!readok)
                return;
            
            u8g2.setFont(u8g2_font_ncenB08_tr);
    
            const auto &d = r.data;
    
            // Заголовок
            char s[20];
            snprintf_P(s, sizeof(s), PSTR("Jump # %d"), d.num);
            u8g2.setDrawColor(0);
            u8g2.drawStr((u8g2.getDisplayWidth()-u8g2.getStrWidth(s))/2, 10, s);
    
            u8g2.setDrawColor(1);
            int8_t y = 10-1+14;
    
            const auto &dt = d.dt;
            snprintf_P(s, sizeof(s), PSTR("%2d.%02d.%04d"), dt.d, dt.m, dt.y);
            u8g2.drawStr(0, y, s);
            snprintf_P(s, sizeof(s), PSTR("%2d:%02d"), dt.hh, dt.mm);
            u8g2.drawStr(u8g2.getDisplayWidth()-u8g2.getStrWidth(s), y, s);
    
            y += 10;
            strcpy_P(s, PSTR("Alt"));
            u8g2.drawStr(0, y, s);
            snprintf_P(s, sizeof(s), PSTR("%.0f"), d.beg.alt);
            u8g2.drawStr(u8g2.getDisplayWidth()-u8g2.getStrWidth(s), y, s);
    
            y += 10;
            strcpy_P(s, PSTR("Deploy"));
            u8g2.drawStr(0, y, s);
            snprintf_P(s, sizeof(s), PSTR("%.0f"), d.cnp.alt);
            u8g2.drawStr(u8g2.getDisplayWidth()-u8g2.getStrWidth(s), y, s);
    
            y += 10;
            strcpy_P(s, PSTR("FF time"));
            u8g2.drawStr(0, y, s);
            snprintf_P(s, sizeof(s), PSTR("%d s"), (d.cnp.mill-d.beg.mill)/1000);
            u8g2.drawStr(u8g2.getDisplayWidth()-u8g2.getStrWidth(s), y, s);
        }
        
        void process() {
            if (btnIdle() > MENU_TIMEOUT)
                setViewMain();
        }
        
    private:
        size_t isel=0, sz=0;
        struct log_item_s<log_jmp_t> r;
        bool readok = false;
};

static ViewMenuLogBookInfo vLogBookInfo;

// список

class ViewMenuLogBook : public ViewMenu {
    public:
        void restore() {
            size_t cnt = logRCountFull(PSTR(JMPLOG_SIMPLE_NAME), struct log_item_s<log_jmp_t>);
            setSize(cnt);
            
            ViewMenu::restore();
        }
        
        void getStr(menu_dspl_el_t &str, int16_t i) {
            Serial.printf("ViewMenuLogBook::getStr: %d\r\n", i);
    
            struct log_item_s<log_jmp_t> r;
            if (logRead(r, PSTR(JMPLOG_SIMPLE_NAME), i)) {
                auto &dt = r.data.dt;
                snprintf_P(str.name, sizeof(str.name), PSTR("%2d.%02d.%02d %2d:%02d"),
                                dt.d, dt.m, dt.y % 100, dt.hh, dt.mm);
                snprintf_P(str.val, sizeof(str.val), PSTR("%d"), r.data.num);
            }
            else {
                str.name[0] = '\0';
                str.val[0] = '\0';
            }
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
                setViewMain();
        }
};

static ViewMenuLogBook vMenuLogBook;
ViewMenu *menuLogBook() { return &vMenuLogBook; }
