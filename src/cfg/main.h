/*
    Cfg
*/

#ifndef _cfg_H
#define _cfg_H

#include "../../def.h"

#include "../view/main.h"

/* 
 *  Конфиг, сохраняемый в SPIFFS
 */
#define CFG_MGC         0xe4

#define CFG_MAIN_ID     3
#define CFG_MAIN_VER    1
#define CFG_MAIN_NAME   "cfg"

// Основной конфиг
typedef struct __attribute__((__packed__)) {
    uint8_t _0;                                 // резерв для выравнивания до 4 байт
    
    uint8_t contrast = 10;                      // контраст дисплея (0..30)
    int16_t timezone = 180;                     // часовой пояс (в минутах)
    
    bool gndmanual = true;                      // Разрешение на ручную корректировку нуля
    bool gndauto = true;                        // Разрешение на автокорректировку нуля
    
    bool dsplautoff = true;                     // Разрешить авто-режим своб падения (переключать экран на высотомер и запрет переключений)
    int8_t dsplcnp  = MODE_MAIN_GPSALT;         // смена экрана при повисании под куполом (высотомер+жпс)
    int8_t dsplland = MODE_MAIN_NONE;           // смена экрана сразу после приземления (не менять)
    int8_t dsplgnd  = MODE_MAIN_NONE;           // смена экрана при длительном нахождении на земле
    int8_t dsplpwron= MODE_MAIN_LAST;           // смена экрана при включении питания
    
    int8_t fwupdind    = 0;                     // Номер версии прошивки в файле veravail, на которую следует обновиться
    
    uint8_t hrpwrnofly  = 0;                    // количество часов отсутствия взлётов перед автовыключением
    uint8_t hrpwrafton  = 0;                    // количество часов после включения перед автовыключением
    
    bool    gpsonpwron  = true;                 // автоматическое включение gps при включении питания
    bool    gpsontrkrec = false;                // автоматическое включение gps при старте записи трека
    uint16_t gpsonalt   = 0;                    // автоматическое включение gps на заданной высоте
    bool    gpsoffland  = false;                // всегда автоматически отключать gps после приземления
    bool    _1 = false;                         // резерв для выравнивания до 4 байт
    
    uint16_t trkonalt = 0;                      // автоматически запускать запись трека на заданной высоте
} cfg_main_t;

template <class T>
class Config {
    public:
        Config(const char *_fname, uint8_t _cfgid, uint8_t _ver = 1);
        bool load();
        bool save(bool force = false);
        void reset();
        void upgrade(uint8_t v, const uint8_t *d);
        
        uint32_t chksum() const;
        
        const T &d() const { return data; }
        T &set() { _modifed = true; return data; }
        bool modifed() { return _modifed; }
    
    protected:
        char fname[10];
        uint8_t cfgid;
        uint8_t ver;
        T data;
        bool _modifed = false;
};

bool cfgLoad(bool apply = true);
bool cfgSave(bool force = false);
bool cfgFactory();

extern Config<cfg_main_t> cfg;


#endif // _cfg_H
