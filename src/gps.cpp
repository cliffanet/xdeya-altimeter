
#include "gps.h"

#include <TinyGPS++.h>
static TinyGPSPlus gps;

// Доп модули, необходимые для режима DPS-Direct
#include "display.h"
#include "button.h"
#include "mode.h"
#include "menu/base.h"

// будем использовать стандартный экземпляр класса HardwareSerial, 
// т.к. он и так в системе уже есть и память под него выделена
// Стандартные пины для свободного аппаратного Serial2: 16(rx), 17(tx)
#define ss Serial2


/* ------------------------------------------------------------------------------------------- *
 *  GPS-обработчик
 * ------------------------------------------------------------------------------------------- */

TinyGPSPlus &gpsGet() { return gps; }

void gpsInit() {
    // инициируем uart-порт GPS-приёмника
    ss.begin(9600);
}

void gpsProcess() {
    while (ss.available()) {
        gps.encode( ss.read() );
    }
}

/* ------------------------------------------------------------------------------------------- *
 *  режим DPS-Direct
 * ------------------------------------------------------------------------------------------- */
static void gpsDirectExit() {
    menuClear();
    modeMain();
    
    loopMain = NULL;
}
static void gpsDirectProcess() {
    btnProcess();
    while (ss.available())
        Serial.write( ss.read() );
    while (Serial.available())
        ss.write( Serial.read() );
}
static void gpsDirectDisplay(U8G2 &u8g2) {
    char txt[128];
    
    u8g2.setFont(u8g2_font_ncenB08_tr);
    
    // Заголовок
    strcpy_P(txt, PSTR("GPS -> Serial Direct"));
    u8g2.drawBox(0,0,128,12);
    u8g2.setDrawColor(0);
    u8g2.drawStr((u8g2.getDisplayWidth()-u8g2.getStrWidth(txt))/2, 10, txt);
    
    u8g2.setDrawColor(1);
    strcpy_P(txt, PSTR("To continue"));
    u8g2.drawStr((u8g2.getDisplayWidth()-u8g2.getStrWidth(txt))/2, 30+14, txt);
    strcpy_P(txt, PSTR("press SELECT-button"));
    u8g2.drawStr((u8g2.getDisplayWidth()-u8g2.getStrWidth(txt))/2, 40+14, txt);
}

void gpsDirect() {
    btnHndClear();
    btnHnd(BTN_SEL, BTN_SIMPLE, gpsDirectExit);
    
    displayHnd(gpsDirectDisplay);
    displayUpdate();
    
    loopMain = gpsDirectProcess;
}
