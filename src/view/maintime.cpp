
#include "main.h"

class ViewMainTime : public ViewMain {
    public:
        void btnSmpl(btn_code_t btn) {
            if (btn != BTN_SEL)
                return;
            
            setViewMain(MODE_MAIN_GPS);
        }
        
        void draw(U8G2 &u8g2) {
            char s[20];
    
            drawState(u8g2);
    
            if (!timeOk()) {
                u8g2.setFont(u8g2_font_ncenB14_tr); 
                strcpy_P(s, PSTR("Clock wait"));
                u8g2.setCursor((u8g2.getDisplayWidth()-u8g2.getStrWidth(s))/2, 20);
                u8g2.print(s);
                strcpy_P(s, PSTR("GPS Sync..."));
                u8g2.setCursor((u8g2.getDisplayWidth()-u8g2.getStrWidth(s))/2, 50);
                u8g2.print(s);
                return;
            }

            u8g2.setFont(u8g2_font_logisoso38_tn); 
            sprintf_P(s, PSTR("%2d:%02d"), hour(), minute());
            u8g2.setCursor((u8g2.getDisplayWidth()-u8g2.getStrWidth(s))/2, 47);
            u8g2.print(s);

            u8g2.setFont(u8g2_font_ncenB08_tr); 
            sprintf_P(s, PSTR("%2d.%02d.%d"), day(), month(), year());
            u8g2.drawStr(0, 64, s);
        }
};
static ViewMainTime vTime;
void setViewMainTime() { viewSet(vTime); }
