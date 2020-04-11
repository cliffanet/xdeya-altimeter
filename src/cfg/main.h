/*
    Cfg
*/

#ifndef _cfg_H
#define _cfg_H

#include <Arduino.h>
#include "../mode.h"

/* 
 *  Конфиг, сохраняемый в SPIFFS
 */
#define CFG_MGC1        0xe4
#define CFG_MGC2        0x7a

#define CFG_MAIN_VER    1
#define CFG_MAIN_NAME   "cfg"
#define CFG_JUMP_VER    1
#define CFG_JUMP_NAME   "jmp"
#define CFG_INFO_VER    1
#define CFG_INFO_NAME   "inf"


// Основной конфиг
typedef struct __attribute__((__packed__)) {
    uint8_t mgc1 = CFG_MGC1;                 // mgc1 и mgc2 служат для валидации текущих данных в eeprom
    uint8_t ver = CFG_MAIN_VER;
    
    uint8_t contrast = 10;                      // контраст дисплея (0..30)
    int16_t timezone = 180;                     // часовой пояс (в минутах)
    
    bool gndmanual = true;                      // Разрешение на ручную корректировку нуля
    bool gndauto = true;                        // Разрешение на автокорректировку нуля
    
    bool dsplautoff = true;                     // Разрешить авто-режим своб падения (переключать экран на высотомер и запрет переключений)
    int8_t dsplcnp  = MODE_MAIN_ALTGPS;         // смена экрана при повисании под куполом (высотомер+жпс)
    int8_t dsplland = MODE_MAIN_NONE;           // смена экрана сразу после приземления (не менять)
    int8_t dsplgnd  = MODE_MAIN_NONE;           // смена экрана при длительном нахождении на земле
    int8_t dsplpwron= MODE_MAIN_LAST;           // смена экрана при включении питания
    
    uint32_t _;                                 // Более не используется
    uint8_t mgc2 = CFG_MGC2;
} cfg_main_t;

// Оперативные данные, которые надо сохранять при уходе в сон и выключении
typedef struct __attribute__((__packed__)) {
    uint8_t mgc1 = CFG_MGC1;                 // mgc1 и mgc2 служат для валидации текущих данных в eeprom
    uint8_t ver = CFG_JUMP_VER;
    
    bool pwron = true;                          // Включено ли наше устр-во
    int8_t mainmode = MODE_MAIN_GPS;            // Текущий режим главного экрана
    uint8_t mgc2 = CFG_MGC2;
} cfg_info_t;

template <class T>
class Config {
    public:
        Config(const char *_fname, uint8_t _ver = 1);
        bool load();
        bool save(bool force = false);
        void reset();
        
        const T &d() const { return data; }
        T &set() { _modifed = true; return data; }
        bool modifed() { return _modifed; }
    
    protected:
        char fname[10];
        uint8_t ver;
        T data;
        bool _modifed = false;
};

bool cfgLoad(bool apply = true);
bool cfgSave(bool force = false);
bool cfgFactory();

extern Config<cfg_main_t> cfg;
extern Config<cfg_info_t> inf;


#endif // _cfg_H
