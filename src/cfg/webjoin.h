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
    
    uint32_t    privkey = 0;
    uint32_t    extid = 0;
    
    uint8_t mgc2 = CFG_MGC2;
} cfg_webjoin_t;

class ConfigWebJoin : public Config<cfg_webjoin_t> {
    public:
        ConfigWebJoin();
        
        uint8_t extid() const { return data.extid; }
};


#endif // _cfg_webjoin_H
