/*
    EEPROM
*/

#ifndef _eeprom_H
#define _eeprom_H

#include <Arduino.h>
#include "gps.h"
#include "mode.h"

/* 
 *  Конфиг, сохраняемый в EEPROM
 */
#define EEPROM_MGC1     0xe4
#define EEPROM_VER      1
#define EEPROM_MGC2     0x7a
#define EEPROM_CFG_SIZE 512
#define EEPROM_CFG_NAME "cfg"


// Количество сохраняемых GPS-точек
#define PNT_COUNT     3
#define PNT_NUMOK     ((cfg.pntnum > 0) && (cfg.pntnum <= PNT_COUNT))
#define POINT         cfg.pntall[cfg.pntnum-1]

// Основной конфиг
typedef struct __attribute__((__packed__)) {
    uint8_t mgc1 = EEPROM_MGC1;                 // mgc1 и mgc2 служат для валидации текущих данных в eeprom
    uint8_t ver = EEPROM_VER;
    
    uint8_t contrast = 10;                      // контраст дисплея (0..30)
    int16_t timezone = 180;                     // часовой пояс (в минутах)
    
    uint8_t pntnum = 0;                         // текущая выбранная точка
    struct __attribute__((__packed__)) {        // координаты трёх точек (sizeof(double) == 8)
        bool used = false;
        double lat = 0;
        double lng = 0;
    } pntall[PNT_COUNT];
    
    bool gndmanual = true;                      // Разрешение на ручную корректировку нуля
    bool gndauto = true;                        // Разрешение на автокорректировку нуля
    
    bool dsplautoff = true;                     // Разрешить авто-режим своб падения (переключать экран на высотомер и запрет переключений)
    int8_t dsplcnp  = MODE_MAIN_ALTGPS;         // смена экрана при повисании под куполом (высотомер+жпс)
    int8_t dsplland = MODE_MAIN_NONE;           // смена экрана сразу после приземления (не менять)
    int8_t dsplgnd  = MODE_MAIN_NONE;           // смена экрана при длительном нахождении на земле
    int8_t dsplpwron= MODE_MAIN_LAST;           // смена экрана при включении питания
} eeprom_cfg_t;

extern eeprom_cfg_t cfg;

void cfgLoad();
void cfgSave();
void cfgFactory();
void cfgApply();

#endif // _eeprom_H
