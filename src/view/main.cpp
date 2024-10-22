
#include "main.h"

#include "menu.h"
#include "../log.h"
#include "../power.h" // pwrBattRaw()
#include "../clock.h"
#include "../jump/proc.h"
#include "../jump/track.h"
#include "../navi/proc.h"
#include "../cfg/point.h"

static RTC_DATA_ATTR uint8_t mode = MODE_MAIN_ALTNAV; // Текущая страница отображения, сохраняется при переходе в меню

static void btnDo(uint8_t op) {
    switch (op) {
        case BTNDO_LIGHT:
            CONSOLE("[%d] displayLightTgl", op);
            displayLightTgl();
            break;
            
        case BTNDO_NAVPWR:
            CONSOLE("[%d] gpsPwrTgl", op);
            gpsPwrTgl();
            break;
            
        case BTNDO_TRKREC:
            CONSOLE("[%d] trkStart/trkStop", op);
            if (trkRunning())
                trkStop();
            else
                trkStart(TRK_RUNBY_HAND);
            break;
            
        case BTNDO_PWROFF:
            CONSOLE("[%d] pwrOff", op);
            pwrOff();
            break;
    }
}

void ViewMain::btnLong(btn_code_t btn) {
    switch (btn) {
        case BTN_UP:
            btnDo(cfg.d().btndo_up);
            return;

        case BTN_SEL:
            setViewMenu();
            return;
            
        case BTN_DOWN:
            btnDo(cfg.d().btndo_down);
            return;
    }
}

void ViewMain::drawState(U8G2 &u8g2) {
    u8g2.setDrawColor(1);
#if HWVER >= 3
    uint8_t blev = pwrBattLevel();
    if ((blev > 0) || isblink()) {
        u8g2.setFont(u8g2_font_battery19_tn);
        u8g2.setFontDirection(1);
        u8g2.drawGlyph(0, 0, '0' + blev);
        u8g2.setFontDirection(0);
    }
    if (pwrBattCharge()) {
        u8g2.setFont(u8g2_font_open_iconic_embedded_1x_t);
        u8g2.drawGlyph(0, 20, 'C');
    }
#endif
    
    u8g2.setFont(u8g2_font_helvB08_tr);

#if HWVER == 2
    char s[10];
    sprintf_P(s, PSTR("%d"), pwrBattRaw());
    u8g2.drawStr(0, 8, s);
#endif

#ifdef USE4BUTTON
// нажатие на дополнительную 4ую кнопку
    if (btn4Pushed()) {
        u8g2.drawBox(0, 7, 10, 10);
        u8g2.setDrawColor(0);
    }
#endif
    
    if (trkRunning() && isblink())
        u8g2.drawGlyph(0, u8g2.getDisplayHeight()-10, 'R');
    
#ifdef USE4BUTTON
    u8g2.setDrawColor(1);
#endif
}

void ViewMain::drawClock(U8G2 &u8g2) {
    if (!tmValid())
        return;
    
    char s[10];
    sprintf_P(s, PSTR("%d:%02d"), tmNow().h, tmNow().m);

    u8g2.setDrawColor(1);
#if HWVER < 4
    u8g2.setFont(u8g2_font_tom_thumb_4x6_mn);
    u8g2.drawStr(53, 6, s);
#else // if HWVER < 4
    u8g2.setFont(u8g2_font_amstrad_cpc_extended_8n);
    u8g2.drawStr(80, 10, s);
#endif // if HWVER < 4
}

void ViewMain::drawAlt(U8G2 &u8g2, int x, int y) {
    auto &ac = altCalc();
    if (ac.state() <= ACST_INIT)
        return;
    
    int16_t alt = round(ac.alt() + cfg.d().altcorrect);
    int16_t o = alt % ALT_STEP;
    alt -= o;
    if (abs(o) > ALT_STEP_ROUND) alt+= o >= 0 ? ALT_STEP : -ALT_STEP;

    char s[10];
    snprintf_P(s, sizeof(s), PSTR("%d"), alt);

    if (x < 0)
        x = u8g2.getDisplayWidth()-u8g2.getStrWidth(s) + x + 1;
    u8g2.drawStr(x, y, s);
}

void ViewMain::drawNavSat(U8G2 &u8g2) {
    auto &gps = gpsInf();
    char s[50];
    
    u8g2.setFont(menuFont);
    
    switch (gpsState()) {
        case NAV_STATE_OFF:
            strcpy_P(s, PTXT(MAIN_NAVSTATE_OFF));
            break;
        
        case NAV_STATE_INIT:
            strcpy_P(s, PTXT(MAIN_NAVSTATE_INIT));
            break;
        
        case NAV_STATE_FAIL:
            strcpy_P(s, PTXT(MAIN_NAVSTATE_INITFAIL));
            break;
        
        case NAV_STATE_NODATA:
            strcpy_P(s, PTXT(MAIN_NAVSTATE_NODATA));
            break;
        
        case NAV_STATE_OK:
            sprintf_P(s, PTXT(MAIN_NAVSTATE_SATCOUNT), gps.numSV);
            break;
    }
    u8g2.drawTxt(0, u8g2.getDisplayHeight()-1, s);
}

void ViewMain::drawNavDist(U8G2 &u8g2, int y) {
    auto &gps = gpsInf();
    if (!gps.validLocation() || !gps.validPoint() || !pnt.numValid() || !pnt.cur().used)
        return;
    
    char s[10];
    double dist =
        gpsDistance(
            gps.getLat(),
            gps.getLon(),
            pnt.cur().lat, 
            pnt.cur().lng
        );

    if (dist < 950) 
        sprintf_P(s, PSTR("%0.0fm"), dist);
    else if (dist < 9500) 
        sprintf_P(s, PSTR("%0.1fk"), dist/1000);
    else if (dist < 950000) 
        sprintf_P(s, PSTR("%0.0fk"), dist/1000);
    else
        sprintf_P(s, PSTR("%0.2fM"), dist/1000000);

    u8g2.drawStr(u8g2.getDisplayWidth() - u8g2.getStrWidth(s), y, s);
}

void setViewMain(int8_t m, bool save) {
    menuClear();
    
    if (m <= MODE_MAIN_NONE)
        return;
    
    if (m == MODE_MAIN_LAST) {
        if ((mode < MODE_MAIN_MIN) || (mode > MODE_MAIN_MAX))
            mode = MODE_MAIN_ALTNAV;
        m = mode;
    }
    else
    if (save && (m >= MODE_MAIN_MIN) && (m <= MODE_MAIN_MAX))
        mode = m;
    
    switch (m) {
        case MODE_MAIN_ALTNAV:
            CONSOLE("click to gps-alt");
            setViewMainAltNav();
            break;
            
        case MODE_MAIN_ALT:
            CONSOLE("click to alt");
            setViewMainAlt();
            break;
        
        case MODE_MAIN_NAV:
            CONSOLE("click to nav");
            setViewMainNav();
            break;
        
        case MODE_MAIN_NAVPATH:
            CONSOLE("click to navpath");
            setViewMainNavPath();
            break;
    }
}
