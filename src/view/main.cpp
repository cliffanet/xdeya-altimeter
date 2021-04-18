
#include "main.h"

#include "../power.h" // pwrBattValue()
#include "../file/track.h"

static RTC_DATA_ATTR uint8_t mode = MODE_MAIN_GPS; // Текущая страница отображения, сохраняется при переходе в меню

void ViewMain::btnLong(btn_code_t btn) {
    switch (btn) {
        case BTN_UP:
            displayLightTgl();
            return;

        case BTN_SEL:
            //modeMenu();
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
    u8g2.setFont(u8g2_font_helvB08_tr);
    u8g2.setDrawColor(1);

#if HWVER > 1
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
        u8g2.drawGlyph(0, 16, 'R');
    
#ifdef USE4BUTTON
    u8g2.setDrawColor(1);
#endif
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
            setViewMainGps();
            break;
            
        case MODE_MAIN_ALT:
            setViewMainAlt();
            break;
            
        case MODE_MAIN_GPSALT:
            setViewMainGpsAlt();
            break;
            
        case MODE_MAIN_TIME:
            setViewMainTime();
            break;
    }
}
