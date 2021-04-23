
#include "main.h"
#include "../altimeter.h"

class ViewMainAlt : public ViewMain {
    public:
        void btnSmpl(btn_code_t btn) {
            if (btn != BTN_SEL)
                return;
    
            setViewMain(MODE_MAIN_GPSALT);
        }
        
        void draw(U8G2 &u8g2) {
            char s[20];
            auto &ac = altCalc();
    
            drawState(u8g2);
            drawClock(u8g2);

            u8g2.setFont(u8g2_font_fub49_tn);
            sprintf_P(s, PSTR("%0.1f"), ac.alt() / 1000);
            //uint8_t x = 17; // Для немоноширных шрифтов автоцентровка не подходит (прыгает при смене цифр)
            //uint8_t x = (u8g2.getDisplayWidth()-u8g2.getStrWidth(s))/2;
            uint8_t x = (u8g2.getDisplayWidth()-u8g2.getStrWidth(s))-15;
            u8g2.setCursor(x, 52);
            u8g2.print(s);

            u8g2.setFont(u8g2_font_ncenB08_tr); 
            sprintf_P(s, PSTR("%d km/h"), round(ac.speed() * 3.6));
            u8g2.drawStr(0, 64, s);
        }
};
static ViewMainAlt vAlt;
void setViewMainAlt() { viewSet(vAlt); }
