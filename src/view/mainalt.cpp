
#include "main.h"
#include "info.h"
#include "../jump/proc.h"

class ViewMainAlt : public ViewMain {
    public:
        void btnSmpl(btn_code_t btn) {
            if (btn != BTN_SEL)
                return;
    
            //setViewMain(MODE_MAIN_GPSALT);
            setViewInfoDebug();
        }
        
        void draw(U8G2 &u8g2) {
            char s[20];
            auto &ac = altCalc();
    
            drawState(u8g2);
            drawClock(u8g2);
            
            if (ac.state() == ACST_INIT)
                return;

            u8g2.setFont(u8g2_font_fub49_tn);
            sprintf_P(s, PSTR("%0.1f"), abs(ac.alt() / 1000));
            //uint8_t x = 17; // Для немоноширных шрифтов автоцентровка не подходит (прыгает при смене цифр)
            //uint8_t x = (u8g2.getDisplayWidth()-u8g2.getStrWidth(s))/2;
            uint8_t x = (u8g2.getDisplayWidth()-u8g2.getStrWidth(s))-15;
            u8g2.drawStr(x, 52, s);
            if (ac.alt() < -30) // чтобы минус не сдвигал цифры, пишем его отдельно
                u8g2.drawGlyph(-5, 48, '-');
            
            switch (ac.direct()) {
                case ACDIR_UP:
                    u8g2.setFont(u8g2_font_open_iconic_arrow_1x_t);
                    u8g2.drawGlyph(0, 64, 0x43);
                    break;
                    
                case ACDIR_DOWN:
                    u8g2.setFont(u8g2_font_open_iconic_arrow_1x_t);
                    u8g2.drawGlyph(0, 64, 0x40);
                    break;
            }

            u8g2.setFont(u8g2_font_ncenB08_tr);
            sprintf_P(s, PSTR("%.0f km/h"), round(abs(ac.speedapp() * 3.6)));
            u8g2.drawStr(10, 64, s);
        }
};
static ViewMainAlt vAlt;
void setViewMainAlt() { viewSet(vAlt); }
