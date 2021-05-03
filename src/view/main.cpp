
#include "main.h"

#include "menu.h"
#include "../log.h"
#include "../power.h" // pwrBattValue()
#include "../clock.h"
#include "../file/track.h"

static RTC_DATA_ATTR uint8_t mode = MODE_MAIN_GPS; // Текущая страница отображения, сохраняется при переходе в меню

void ViewMain::btnLong(btn_code_t btn) {
    switch (btn) {
        case BTN_UP:
            displayLightTgl();
            return;

        case BTN_SEL:
            setViewMenu();
            return;
            
        case BTN_DOWN:
            if (trkRunning())
                trkStop();
            else
                trkStart(true);
            return;
    }
}

void ViewMain::drawState(U8G2 &u8g2) {
    u8g2.setDrawColor(1);
#if HWVER >= 3
    uint16_t bv = pwrBattValue();
    if ((bv > 2600) || ((millis() % 2000) >= 1000)) {
        u8g2.setFont(u8g2_font_battery19_tn);
        char b = 
                bv > 3300 ? '5' :
                bv > 3250 ? '4' :
                bv > 3150 ? '3' :
                bv > 3000 ? '2' :
                bv > 2800 ? '1' :
                '0';
        u8g2.drawGlyph(0, 20, b);
    }
    if (pwrBattCharge()) {
        u8g2.setFont(u8g2_font_open_iconic_embedded_1x_t);
        u8g2.drawGlyph(10, 9, 'C');
    }
#endif
    
    u8g2.setFont(u8g2_font_helvB08_tr);

#if HWVER == 2
    char s[10];
    sprintf_P(s, PSTR("%d"), pwrBattValue());
    u8g2.drawStr(0, 8, s);
#endif

#ifdef USE4BUTTON
// нажатие на дополнительную 4ую кнопку
    if (btn4Pushed()) {
        u8g2.drawBox(0, 7, 10, 10);
        u8g2.setDrawColor(0);
    }
#endif
    
    if (trkRunning() && ((millis() % 2000) >= 1000))
        u8g2.drawGlyph(0, 64, 'R');
    
#ifdef USE4BUTTON
    u8g2.setDrawColor(1);
#endif
}

void ViewMain::drawClock(U8G2 &u8g2) {
    if (!tmNow().valid)
        return;
    
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_tom_thumb_4x6_mn);
    
    char s[10];
    sprintf_P(s, PSTR("%d:%02d"), tmNow().h, tmNow().m);
    u8g2.drawStr(53, 6, s);
}

void setViewMain(int8_t m, bool save) {
    if (m <= MODE_MAIN_NONE)
        return;
    
    if (m == MODE_MAIN_LAST) {
        if ((mode < MODE_MAIN_MIN) || (mode > MODE_MAIN_MAX))
            mode = MODE_MAIN_GPS;
        m = mode;
    }
    else
    if (save && (m >= MODE_MAIN_MIN) && (m <= MODE_MAIN_MAX))
        mode = m;
    
    switch (m) {
        case MODE_MAIN_GPS:
            CONSOLE("click to gps");
            setViewMainGps();
            break;
            
        case MODE_MAIN_ALT:
            CONSOLE("click to alt");
            setViewMainAlt();
            break;
            
        case MODE_MAIN_GPSALT:
            CONSOLE("click to gps-alt");
            setViewMainGpsAlt();
            break;
            
        case MODE_MAIN_TIME:
            CONSOLE("click to time");
            setViewMainTime();
            break;
    }
}
