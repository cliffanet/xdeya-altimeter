/*
    Data convert function
*/

#include "data.h"
#include "srv.h"
#include "../cfg/main.h"

#include <WiFi.h> // htonl

/* ------------------------------------------------------------------------------------------- *
 *  простые преобразования данных
 * ------------------------------------------------------------------------------------------- */
static uint8_t bton(bool val) {
    return val ? 1 : 0;
}
static bool ntob(uint8_t val) {
    return val != 0;
}

/* ------------------------------------------------------------------------------------------- *
 *  Преобразования display mode
 * ------------------------------------------------------------------------------------------- */
static char modeton(int8_t mode) {
    switch (mode) {
        case MODE_MAIN_NONE:    return 'N';
        case MODE_MAIN_LAST:    return 'L';
        case MODE_MAIN_GPS:     return 'g';
        case MODE_MAIN_ALT:     return 'a';
        case MODE_MAIN_ALTGPS:  return 'm';
        case MODE_MAIN_TIME:    return 't';
    };
    return 'U';
}
static int8_t ntomode(char n) {
    switch (n) {
        case 'N': return MODE_MAIN_NONE;
        case 'L': return MODE_MAIN_LAST;
        case 'g': return MODE_MAIN_GPS;
        case 'a': return MODE_MAIN_ALT;
        case 'm': return MODE_MAIN_ALTGPS;
        case 't': return MODE_MAIN_TIME;
    };
    return MODE_MAIN_NONE;
}

/* ------------------------------------------------------------------------------------------- *
 *  отправка на сервер
 * ------------------------------------------------------------------------------------------- */
bool sendCfg() {
    const auto cfg_d = cfg.d();
    
    struct __attribute__((__packed__)) { // Для передачи по сети
        uint8_t contrast;
        int16_t timezone;
        
        uint8_t gndmanual;
        uint8_t gndauto;
        
        uint8_t dsplautoff;
        char    dsplcnp;
        char    dsplland;
        char    dsplgnd;
        char    dsplpwron;
    } d = {
        .contrast   = cfg_d.contrast,
        .timezone   = htons(cfg_d.timezone),
        .gndmanual  = bton(cfg_d.gndmanual),
        .gndauto    = bton(cfg_d.gndauto),
        .dsplautoff = bton(cfg_d.dsplautoff),
        .dsplcnp    = modeton(cfg_d.dsplcnp),
        .dsplland   = modeton(cfg_d.dsplland),
        .dsplgnd    = modeton(cfg_d.dsplgnd),
        .dsplpwron  = modeton(cfg_d.dsplpwron),
    };
    
    return srvSend(0x21, d);
}

