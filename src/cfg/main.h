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

typedef enum {
    BTNDO_NONE = 0,
    BTNDO_LIGHT,
    BTNDO_NAVPWR,
    BTNDO_TRKREC,
    BTNDO_PWROFF,
    BTNDO_MAX
} btndo_t;

// Основной конфиг
typedef struct __attribute__((__packed__)) {
    bool flipp180 = false;                      // перевернуть экран на 180
    
    uint8_t contrast = 10;                      // контраст дисплея (0..30)
    int16_t timezone = 180;                     // часовой пояс (в минутах)
    
    bool gndmanual = true;                      // Разрешение на ручную корректировку нуля
    bool gndauto = true;                        // Разрешение на автокорректировку нуля
    
    bool dsplautoff = true;                     // Разрешить авто-режим своб падения (переключать экран на высотомер и запрет переключений)
    int8_t dsplcnp  = MODE_MAIN_ALTNAV;         // смена экрана при повисании под куполом (высотомер+жпс)
    int8_t dsplland = MODE_MAIN_NONE;           // смена экрана сразу после приземления (не менять)
    int8_t dsplgnd  = MODE_MAIN_NONE;           // смена экрана при длительном нахождении на земле
    int8_t dsplpwron= MODE_MAIN_LAST;           // смена экрана при включении питания
    
    int8_t fwupdind    = 0;                     // Номер версии прошивки в файле veravail, на которую следует обновиться
    
    uint8_t hrpwrnofly  = 0;                    // количество часов отсутствия взлётов перед автовыключением
    uint8_t hrpwrafton  = 0;                    // количество часов после включения перед автовыключением
    
    bool    navonpwron  = true;                 // автоматическое включение gps при включении питания
    bool    navontrkrec = false;                // автоматическое включение gps при старте записи трека
    uint16_t navonalt   = 0;                    // автоматическое включение gps на заданной высоте
    bool    navoffland  = true;                 // всегда автоматически отключать gps после приземления
    uint8_t compas:1    = 1;                    // использовать компас
    uint8_t navtxtcourse:1 = 1;                 // курс движения и "до точки" текстом
    uint8_t navnoacc:1  = 0;                    // отключение точности координат как фильтра работы навигации
    uint8_t :0;
    
    uint16_t trkonalt = 0;                      // автоматически запускать запись трека на заданной высоте
    
    uint8_t btndo_up    = BTNDO_LIGHT;          // действие по кнопке "вверх"
    uint8_t btndo_down  = BTNDO_NAVPWR;         // действие по кнопке "вниз"
    
    int16_t altcorrect = 0;                     // Превышение площадки приземления
    
    uint8_t navgnss = 3;                        // режим навигации - gps/glonass
    uint8_t navmodel = 0;                       // режим навигации - model
    
    uint8_t _1 = 0;
    uint8_t _2 = 0;
    uint8_t _3 = 0;
    uint8_t net = 0;                            // net: 0x1 = bluetooth, 0x2 = wifi
} cfg_main_t;

template <class T>
class Config {
    public:
        Config(const char *_fname_P, uint8_t _cfgid, uint8_t _ver = 1);
        bool load();
        bool save(bool force = false);
        void reset();
        void upgrade(uint8_t v, const uint8_t *d);
        
        uint32_t chksum() const;
        
        const T &d() const { return data; }
        T &set() { _modifed = true; _empty = false; return data; }
        bool modifed() { return _modifed; }
        bool isempty() const { return _empty; }
    
    protected:
        const char *fname_P;
        uint8_t cfgid;
        uint8_t ver;
        T data;
        bool _modifed = false;
        bool _empty = true;
};

bool cfgLoad(bool apply = true);
bool cfgSave(bool force = false);
bool cfgFactory();

extern Config<cfg_main_t> cfg;


#endif // _cfg_H
