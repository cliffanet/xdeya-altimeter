/*
    Data transfere functions
*/

#include "wifisync.h"
#include "../log.h"
#include "../core/workerloc.h"
#include "binproto.h"
#include "../view/text.h"
#include "../cfg/main.h"
#include "../cfg/jump.h"



/* ------------------------------------------------------------------------------------------- *
 *  Основной конфиг
 * ------------------------------------------------------------------------------------------- */
bool sendCfgMain(NetSocket *nsock) {
    const auto &cfg_d = cfg.d();
    
    BinProtoSend s(nsock, '%');
    s.add( 0x21, PSTR("XCnCCCaaaa") );
    
    struct __attribute__((__packed__)) {
        uint32_t chksum;
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
        cfg.chksum(),
        cfg_d.contrast,
        cfg_d.timezone,
        cfg_d.gndmanual,
        cfg_d.gndauto,
        cfg_d.dsplautoff,
        cfg_d.dsplcnp,
        cfg_d.dsplland,
        cfg_d.dsplgnd,
        cfg_d.dsplpwron,
    };
    
    return s.send( 0x21, d );
}

/* ------------------------------------------------------------------------------------------- *
 *  Количество прыгов
 * ------------------------------------------------------------------------------------------- */
bool sendJmpCount(NetSocket *nsock) {
    BinProtoSend s(nsock, '%');
    s.add( 0x22, PSTR("XN") );
    
    struct __attribute__((__packed__)) { // Для передачи по сети
        uint32_t chksum;
        uint32_t count;
    } d = {
        .chksum     = jmp.chksum(),
        .count      = jmp.d().count,
    };
    
    return s.send( 0x22, d );
}
