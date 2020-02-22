
#include "display.h"

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif

//U8G2_ST7565_ZOLEN_128X64_1_4W_HW_SPI u8g2(U8G2_MIRROR, /* cs=*/ 5, /* dc=*/ 33, /* reset=*/ 25);

static U8G2_UC1701_MINI12864_F_4W_HW_SPI u8g2(U8G2_R2, /* cs=*/ 26, /* dc=*/ 33, /* reset=*/ 25);

static bool clearing = false;


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
    
    pinMode(LIGHT_PIN, OUTPUT);
}

void displayHnd(display_hnd_t hnd) {
    _dspl_hnd = hnd;
    clearing = true;
}

void displayClear() { clearing = true; }

void displayUpdate() {
    if (_dspl_hnd == NULL)
        return;
    
    if (clearing) {
        u8g2.clearDisplay();
        clearing = false;
    }
     
    u8g2.firstPage();
    do {
        _dspl_hnd(u8g2);
    }  while( u8g2.nextPage() );
}

/* ------------------------------------------------------------------------------------------- *
 * Управление дисплеем: вкл/выкл самого дисплея и его подсветки, изменение контрастности
 * ------------------------------------------------------------------------------------------- */
static bool lght = false;
void displayOn() {
    u8g2.setPowerSave(false);
    u8g2.clearDisplay();
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
