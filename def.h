/*
    common
*/

#ifndef _def_H
#define _def_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <TinyGPS++.h>
#include <TimeLib.h>

#include "src/button.h"
#include "src/display.h"
#include "src/mode.h"

// Время (мс) удержания кнопки для различных ситуаций
#define BTNTIME_PWRON     3000
#define BTNTIME_PWROFF    4000
#define BTNTIME_MENUENTER 3000
#define BTNTIME_MENUEXIT  2000
#define BTNTIME_FACTORY   4000
#define BTNTIME_SAVEPNT   2000

// Пин подсветки дисплея
#define LIGHT_PIN 32

// Интервал синхронизации времени
#define TIME_ADJUST_INTERVAL  1200000
#define TIME_ADJUST_TIMEOUT   3600000
bool timeOk();

// Логическое состояние включенности устройства
extern bool is_on;

// Количество сохраняемых GPS-точек
#define PNT_COUNT     3
#define PNT_NUMOK     ((cfg.pntnum > 0) && (cfg.pntnum <= PNT_COUNT))
#define POINT         cfg.pntall[cfg.pntnum-1]

/* 
 *  Конфиг, сохраняемый в EEPROM
 */
#define EEPROM_MGC1   0xe4
#define EEPROM_VER    1
#define EEPROM_MGC2   0x7a

typedef struct __attribute__((__packed__)) {  // структура для хранения точек в eeprom
    uint8_t mgc1 = EEPROM_MGC1;               // mgc1 и mgc2 служат для валидации текущих данных в eeprom
    uint8_t ver = EEPROM_VER;
    
    uint8_t contrast = 10;                    // контраст дисплея (0..30)
    int16_t timezone = 180;                   // часовой пояс (в минутах)
    
    uint8_t pntnum = 0;                          // текущая выбранная точка
    struct __attribute__((__packed__)) {      // координаты трёх точек (sizeof(double) == 8)
        bool used = false;
        double lat = 0;
        double lng = 0;
    } pntall[PNT_COUNT];
    
    char res[512-3-3-17*PNT_COUNT] = { 0 };   // выравнивание структуры до 512 байт на случай, если появятся новые параметры
    uint8_t mgc2 = EEPROM_MGC2;
} eeprom_cfg_t;

extern eeprom_cfg_t cfg;

void cfgLoad();
void cfgSave();
void cfgFactory();
void cfgApply();

/* 
 *  Дисплей
 */
typedef enum {
    DISP_INIT = 0,
    DISP_MAIN,
    DISP_TIME,
    DISP_MENU,
    DISP_MENUHOLD
} display_mode_t;

void displayOn();
void displayOff();
void displayLightTgl();
bool displayLight();

void displayContrast(uint8_t value);
void displaySetMode(display_mode_t mode);
display_mode_t displayMode();
void displayChange(display_mode_t mode, uint32_t timeout = 0);

// GPS
TinyGPSPlus &gpsGet();

/* 
 *  Меню настроек
 */

// таймаут бездействия в меню настроек
#define MENU_TIMEOUT    30000
 
typedef void (*menu_chg_t)();
typedef void (*menu_valshow_t)(char *txt);
typedef struct {
    const uint8_t *font;
    uint16_t icon;
    const char *name;
    menu_valshow_t show;
    menu_chg_t prev;
    menu_chg_t next;
    uint32_t   blng;
    menu_chg_t lng;
} menu_item_t;

void menuList(menu_item_t *menu);
void menuInfo(char *name, char *val);
void menuShow();
void menuExit();
void menuTimeOut();
void menuSelect();
void menuPrev();
void menuNext();
bool menuHold(uint8_t btn, uint32_t bhld);

void pwrOn();
void pwrOff();

#endif // _def_H
