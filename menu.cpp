
#include "def.h"

static bool cfg_chg = false;
static uint32_t menuAct = 0;

static void dsplLightVal(char *txt) {
    strcpy_P(txt, displayLight() ? PSTR("On") : PSTR("Off"));
}

static void dsplContrastVal(char *txt) {
    sprintf_P(txt, PSTR("%d"), cfg.contrast);
}
static void dsplContrastL() {
    if (cfg.contrast == 0) return;
    cfg.contrast--;
    cfg_chg = true;
    displayContrast(cfg.contrast);
}
static void dsplContrastR() {
    if (cfg.contrast >= 30) return;
    cfg.contrast++;
    cfg_chg = true;
    displayContrast(cfg.contrast);
}

static void pntVal(char *txt) {
    if (PNT_NUMOK) {
        sprintf_P(txt, PSTR("%d"), cfg.pntnum);
        if (!(POINT.used))
            strcpy_P(txt+strlen(txt), PSTR(" [NO]"));
    }
    else
        strcpy_P(txt, PSTR("[no point]"));
}
static void pntSelL() {
    if (cfg.pntnum > 0)
        cfg.pntnum--;
    else
        cfg.pntnum = PNT_COUNT;
    cfg_chg = true;
}
static void pntSelR() {
    if (cfg.pntnum < PNT_COUNT)
        cfg.pntnum++;
    else
        cfg.pntnum = 0;
    cfg_chg = true;
}

static uint8_t psave = 0;
static void psaveVal(char *txt) {
    if ((psave > 0) && (psave <= PNT_COUNT))
        sprintf_P(txt, PSTR("%d"), psave);
    else
        strcpy_P(txt, PSTR("[no]"));
}
static void psaveSelL() {
    if (psave > 0)
        psave--;
    else
        psave = PNT_COUNT;
}
static void psaveSelR() {
    if (psave < PNT_COUNT)
        psave++;
    else
        psave = 0;
}
static void pntSave() {
    if ((psave == 0) || (psave > PNT_COUNT))
        return;
    TinyGPSPlus &gps = gpsGet();
    if ((gps.satellites.value() == 0) || !gps.location.isValid())
        return;
        
    cfg.pntall[psave-1].used = true;
    cfg.pntall[psave-1].lat = gps.location.lat();
    cfg.pntall[psave-1].lng = gps.location.lng();
    cfg_chg = true;
    psave = 0;
    Serial.print("Save point: ");
    Serial.println(psave);
}

static void timezoneVal(char *txt) {
    if (cfg.timezone == 0) {
        strcpy_P(txt, PSTR("UTC"));
        return;
    }
    *txt = cfg.timezone > 0 ? '+' : '-';
    txt++;

    txt += sprintf_P(txt, PSTR("%d"), abs(cfg.timezone) / 60);
    uint16_t m = abs(cfg.timezone);
    m = m % 60;
    if (m)
        sprintf_P(txt, PSTR(":%02d"), m);
}
static void timezoneL() {
    if (cfg.timezone <= -12*60)
        return;
    cfg.timezone -= 30;
    adjustTime(-30 * 60);
    cfg_chg = true;
}
static void timezoneR() {
    if (cfg.timezone >= 12*60)
        return;
    cfg.timezone += 30;
    adjustTime(30 * 60);
    cfg_chg = true;
}


static const menu_item_t menu_all[] = {
    { u8g2_font_open_iconic_gui_4x_t,       65, PSTR("Exit"),           NULL,             menuExit,       menuExit },
    { u8g2_font_open_iconic_embedded_4x_t,  78, PSTR("Power Off"),      NULL,             NULL,           NULL,           BTNTIME_PWROFF, pwrOff },
    { u8g2_font_open_iconic_weather_4x_t,   69, PSTR("Dspl Light:"),    dsplLightVal,     displayLightTgl,displayLightTgl },
    { u8g2_font_open_iconic_embedded_4x_t,  80, PSTR("Syncronisation") },
    { u8g2_font_open_iconic_other_4x_t,     71, PSTR("Point:"),         pntVal,           pntSelL,        pntSelR },
    { u8g2_font_open_iconic_mime_4x_t,      64, PSTR("Save Point?"),    psaveVal,         psaveSelL,      psaveSelR },
    { u8g2_font_open_iconic_embedded_4x_t,  75, PSTR("Dspl Contrast:"), dsplContrastVal,  dsplContrastL,  dsplContrastR },
    { u8g2_font_open_iconic_app_4x_t,       69, PSTR("Time Zone:"),     timezoneVal,      timezoneL,      timezoneR },
    { u8g2_font_open_iconic_embedded_4x_t,  79, PSTR("Factory Reset"),  NULL,             NULL,           NULL,           BTNTIME_FACTORY, cfgFactory },
};
static uint8_t menu_size = sizeof(menu_all)/sizeof(menu_item_t);
static uint8_t menu_cur = 0;
static uint8_t menuHolded = 0;

void menuList(menu_item_t *menu) {
    menu[0] = menu_cur > 0 ? menu_all[menu_cur-1] : menu_all[menu_size-1];
    menu[1] = menu_all[menu_cur];
    menu[2] = menu_cur < (menu_size-1) ? menu_all[menu_cur+1] : menu_all[0];
}
void menuInfo(char *name, char *val) {
    strcpy_P(name, menu_all[menu_cur].name);
    if ((menu_all[menu_cur].blng > 0) && (displayMode() == DISP_MENUHOLD))
        sprintf_P(val, PSTR("%d"), menuHolded);
    else
    if (menu_all[menu_cur].show == NULL)
        *val = '\0';
    else
        menu_all[menu_cur].show(val);
}

void menuShow() {
    displaySetMode(DISP_MENU);
    menu_cur = 0;
    menuAct = millis() + MENU_TIMEOUT;
}

void menuExit() {
    if ((displayMode() != DISP_MENU) && (displayMode() != DISP_MENUHOLD))
        return;
    pntSave();
    if (cfg_chg) {
        cfgSave();
        cfg_chg = false;
    }
    menuAct = 0;
    displaySetMode(DISP_MAIN);
}

void menuTimeOut() {
    if (menuAct == 0)
        return;
        
    if (displayMode() == DISP_MENU) {
        if (millis() >= menuAct)
            menuExit();
    }
    else
        menuAct = 0;
}

void menuSelect() {
    menuAct = millis() + MENU_TIMEOUT;
    pntSave(); // Проверка, надо ли сохранять текущую GSP-точку, если она была выбрана
    menu_cur++;
    if (menu_cur >= menu_size)
        menu_cur=0;
    Serial.println(menu_cur);
    
    displayChange(DISP_MENU);
}

void menuPrev() {
    menuAct = millis() + MENU_TIMEOUT;
    if (menu_all[menu_cur].prev != NULL) {
        menu_all[menu_cur].prev();
        displayChange(DISP_MENU);
    }
    if (menu_all[menu_cur].blng > 0) {
        menuHolded = menu_all[menu_cur].blng / 1000;
        displaySetMode(DISP_MENUHOLD);
    }
}

void menuNext() {
    menuAct = millis() + MENU_TIMEOUT;
    if (menu_all[menu_cur].next != NULL) {
        menu_all[menu_cur].next();
        displayChange(DISP_MENU);
    }
    if (menu_all[menu_cur].blng > 0) {
        menuHolded = menu_all[menu_cur].blng / 1000;
        displaySetMode(DISP_MENUHOLD);
    }
}

bool menuHold(uint8_t btn, uint32_t bhld) {
    if (((btn & (BTN_UP | BTN_DOWN)) > 0) &&
        (menu_all[menu_cur].blng > 0)) {
        menuAct = millis() + MENU_TIMEOUT;
        if (menu_all[menu_cur].blng <= bhld) {
            menuExit();
            menuHolded = 0;
            if (menu_all[menu_cur].lng != NULL)
                menu_all[menu_cur].lng();
        }
        else {
            uint32_t mhold = menu_all[menu_cur].blng - bhld;
            mhold = mhold / 1000 + (mhold % 1000 ? 1 : 0);
            if (mhold != menuHolded) {
                menuHolded = mhold;
                displayChange(DISP_MENUHOLD);
            }
            return false;
        }
    }
    else {
        displaySetMode(DISP_MENU);
        menuHolded = 0;
    }
    
    return true;
}
