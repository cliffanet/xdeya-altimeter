/*
    EEPROM
*/

#ifndef _eeprom_H
#define _eeprom_H

#include "../def.h"

/* 
 *  Конфиг, сохраняемый в EEPROM
 */
#define EEPROM_MGC1     0xe4
#define EEPROM_VER      1
//#define EEPROM_MGC2     0x7a
#define EEPROM_CFG_SIZE 512
#define EEPROM_CFG_NAME "cfg"

typedef struct __attribute__((__packed__)) {    // структура для хранения точек в eeprom
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
    
//    char res[512-3-3-17*PNT_COUNT] = { 0 };   // выравнивание структуры до 512 байт на случай, если появятся новые параметры
//   uint8_t mgc2 = EEPROM_MGC2;
} eeprom_cfg_t;

extern eeprom_cfg_t cfg;

void cfgLoad();
void cfgSave();
void cfgFactory();
void cfgApply();

#endif // _eeprom_H
