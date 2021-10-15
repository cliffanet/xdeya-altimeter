/*
    Cfg for Web-join
*/

#ifndef _cfg_webjoin_H
#define _cfg_webjoin_H

#include "main.h"

/* 
 *  Конфиг, сохраняемый в SPIFFS
 */

#define CFG_WEBJOIN_ID      2
#define CFG_WEBJOIN_VER     1
#define CFG_WEBJOIN_NAME    "wjn"

//  Сохранение GPS-координат
typedef struct __attribute__((__packed__)) {
    uint32_t    authid = 0;
    uint32_t    secnum = 0;
} cfg_webjoin_t;

class ConfigWebJoin : public Config<cfg_webjoin_t> {
    public:
        ConfigWebJoin();
        ConfigWebJoin(uint32_t authid, uint32_t secnum);
        
        uint32_t authid() const { return data.authid; }
};


#endif // _cfg_webjoin_H
