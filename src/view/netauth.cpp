
#include "netauth.h"

#include "main.h"
#include "../log.h"

class ViewNetAuth : public View {
    public:
        uint16_t m_code = 0;
        uint16_t m_timeout = 0;
        
        void btnSmpl(btn_code_t btn) {
            setViewMain();
        }
        
        // отрисовка на экране
        void draw(U8G2 &u8g2) {
            char s[128];
            u8g2.setFont(menuFont);
    
            // Заголовок
            u8g2.setDrawColor(1);
            u8g2.drawBox(0,0,u8g2.getDisplayWidth(),12);
            u8g2.setDrawColor(0);
            strcpy_P(s, PTXT(NET_AUTH_TITLE));
            u8g2.drawTxt((u8g2.getDisplayWidth()-u8g2.getTxtWidth(s))/2, 10, s);
            
            u8g2.setDrawColor(1);
            int8_t y = 10-1+14;
            
            strcpy_P(s, PTXT(NET_AUTH_TEXT1));
            u8g2.drawTxt(0, y, s);
            
            y += 10;
            
            strcpy_P(s, PTXT(NET_AUTH_TEXT2));
            u8g2.drawTxt(0, y, s);
            
            y += 25;
            u8g2.setFont(u8g2_font_fub20_tr);
            snprintf_P(s, sizeof(s), PSTR("%04X"), m_code);
            u8g2.drawStr((u8g2.getDisplayWidth()-u8g2.getStrWidth(s))/2, y, s);
            
            y += 10;
            uint16_t sec = m_timeout / 5;
            uint16_t min = sec / 60;
            sec -= min*60;
            snprintf_P(s, sizeof(s), PTXT(NET_AUTH_WAIT), min, sec);
            u8g2.setFont(menuFont);
            u8g2.drawTxt(u8g2.getDisplayWidth()-u8g2.getTxtWidth(s)-10, y, s);
        }
        
        void process() {
            if (m_timeout > 0)
                m_timeout --;
            else
                setViewMain();
        }
};
ViewNetAuth vNetAuth;

void setViewNetAuth(uint16_t code) {
    vNetAuth.m_code = code;
    vNetAuth.m_timeout = 2 * 60 * 5;
    viewSet(vNetAuth);
}

bool viewIsNetAuth() {
    return viewIs(vNetAuth);
}
