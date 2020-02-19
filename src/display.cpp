
#include "display.h"

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif

//U8G2_ST7565_ZOLEN_128X64_1_4W_HW_SPI u8g2(U8G2_MIRROR, /* cs=*/ 5, /* dc=*/ 33, /* reset=*/ 25);

U8G2_UC1701_MINI12864_F_4W_HW_SPI u8g2(U8G2_R2, /* cs=*/ 26, /* dc=*/ 33, /* reset=*/ 25);

static display_mode_t mode_curr = DISP_INIT, mode_prev = DISP_INIT;
static uint8_t change = 0;
static uint32_t updated = 0;




/* ------------------------------------------------------------------------------------------- *
 * Функция отрисовки Меню
 * ------------------------------------------------------------------------------------------- */
inline void displayMenu() {
    menu_item_t ml[3];

    menuList(ml);
    u8g2.setFont(u8g2_font_helvB10_tr); 
    for (uint8_t i = 0; i < 3; i++) {
        uint8_t pos = 4+(40*i);
        u8g2.setFont(ml[i].font);
        u8g2.drawGlyph(pos+3, 36, ml[i].icon);
    }

    u8g2.drawFrame(46, 3, 34, 34);
    u8g2.drawFrame(45, 2, 36, 36);
    u8g2.drawFrame(44, 1, 38, 38);
    
    u8g2.setFont(u8g2_font_helvB10_tr);
    char name[30], val[15];
    menuInfo(name, val);
    if (val[0] == '\0')
        u8g2.setCursor((u8g2.getDisplayWidth()-u8g2.getStrWidth(name))/2, u8g2.getDisplayHeight()-5);
    else
        u8g2.setCursor(0, u8g2.getDisplayHeight()-5);
    u8g2.print(name);
    if (val[0] != '\0') {
        u8g2.setCursor(u8g2.getDisplayWidth()-u8g2.getStrWidth(val)-4, u8g2.getDisplayHeight()-5);
        u8g2.print(val);
    }
}

inline void displayMenuHold() {
    menu_item_t ml[3];

    menuList(ml);
    u8g2.setFont(ml[1].font);
    u8g2.drawGlyph(47, 36, ml[1].icon);
    
    u8g2.setFont(u8g2_font_helvB10_tr);
    char name[30], val[15];
    menuInfo(name, val);
    u8g2.setCursor(0, u8g2.getDisplayHeight()-5);
    u8g2.print(name);
    u8g2.setCursor(u8g2.getDisplayWidth()-u8g2.getStrWidth(val)-4, u8g2.getDisplayHeight()-5);
    u8g2.print(val);
}

/* ------------------------------------------------------------------------------------------- *
 * Базовые функции обновления дисплея
 * ------------------------------------------------------------------------------------------- */
static display_hnd_t _dspl_hnd = NULL;

void displayInit() {
    u8g2.begin();
    displayOn();
    u8g2.setFont(u8g2_font_helvB10_tr);  
    do {
        u8g2.drawStr(0,24,"Init...");
    } while( u8g2.nextPage() );
}

void displayHnd(display_hnd_t hnd) {
    _dspl_hnd = hnd;
    change = 2;
}

void displayUpdate() {
    if (_dspl_hnd == NULL)
        return;
    
    if (change == 0)
        return;
    
    if (change > 1)
        u8g2.clearDisplay();
     
    u8g2.firstPage();
    do {
        _dspl_hnd(u8g2);
    }  while( u8g2.nextPage() );

    updated = millis();
    change = 0;
}

static bool lght = false;
void displayOn() {
    u8g2.setPowerSave(false);
    u8g2.clearDisplay();
    change = 2;
    digitalWrite(LIGHT_PIN, lght ? HIGH : LOW);
}
void displayOff() {
    u8g2.setPowerSave(true);
    digitalWrite(LIGHT_PIN, LOW);
}

void displayLightTgl() {
    lght = not lght;
    digitalWrite(LIGHT_PIN, lght ? HIGH : LOW);
}

bool displayLight() {
    return lght;
}

void displayContrast(uint8_t val) {
    //u8g2.setContrast(val*3);
    u8g2.setContrast(115+val*3);
}

void displaySetMode(display_mode_t mode) {
    if (mode == mode_curr)
        return;
    mode_curr = mode;
    change = 2;
}

display_mode_t displayMode() {
    return mode_curr;
}

void displayChange(display_mode_t mode, uint32_t timeout) {
    if ((mode != mode_curr) || (change != 0))
        return;
    if ((updated > 0) && ((updated+timeout) > millis()))
        return;
    
    change = 1;
}
