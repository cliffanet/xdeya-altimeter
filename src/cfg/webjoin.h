/*
    Cfg for Web-join
*/

#ifndef _cfg_webjoin_H
#define _cfg_webjoin_H

#include "main.h"

/* 
 *  Конфиг, сохраняемый в SPIFFS
 */

#define CFG_WEBJOIN_VER   1
#define CFG_WEBJOIN_NAME  "wjn"

//  Сохранение GPS-координат
typedef struct __attribute__((__packed__)) {
    uint8_t mgc1 = CFG_MGC1;                 // mgc1 и mgc2 служат для валидации текущих данных в eeprom
    uint8_t ver = CFG_WEBJOIN_VER;
    
    uint32_t    authid = 0;
    uint32_t    secnum = 0;
    
    uint8_t mgc2 = CFG_MGC2;
} cfg_webjoin_t;

class ConfigWebJoin : public Config<cfg_webjoin_t> {
    public:
        ConfigWebJoin();
        ConfigWebJoin(uint32_t authid, uint32_t secnum);
        
        uint8_t authid() const { return data.authid; }
};


#endif // _cfg_webjoin_H
