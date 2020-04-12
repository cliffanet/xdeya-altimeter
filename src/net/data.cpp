/*
    Data convert function
*/

#include "data.h"
#include "srv.h"
#include "../cfg/main.h"
#include "../cfg/jump.h"
#include "../cfg/point.h"
#include "../logfile.h"
#include "../track.h"

#include <WiFi.h> // htonl

/* ------------------------------------------------------------------------------------------- *
 *  простые преобразования данных
 * ------------------------------------------------------------------------------------------- */
static uint8_t bton(bool val) {
    return val ? 1 : 0;
}
static bool ntob(uint8_t n) {
    return n != 0;
}

static uint16_t fton(float val) {
    return htons(round(val*100));
}
static float ntof(uint16_t n) {
    float val = ntohs(n);
    return val / 100;
}

typedef struct __attribute__((__packed__)) {
    uint32_t i;
    uint32_t d;
} dnet_t;
static dnet_t dton(double val) {
    int32_t i = val;
    double d = val-i;
    dnet_t n = { i };
    
    if (d < 0) d = d*(-1);
    n.d = d * 0xffffffff;
    
    n.i = htonl(n.i);
    n.d = htonl(n.d);
    
    return n;
}

static dt_t dtton(const dt_t &dt) {
    auto n = dt;
    n.y = htons(n.y);
    return n;
}
static dt_t ntodt(const dt_t &n) {
    auto dt = n;
    dt.y = htons(dt.y);
    return dt;
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
 *  Преобразования log_item_t
 * ------------------------------------------------------------------------------------------- */
typedef struct __attribute__((__packed__)) {
    uint32_t    mill;
    dnet_t      alt;
    dnet_t      vspeed;
    char        state;
    char        direct;
    dnet_t      lat;
    dnet_t      lng;
    dnet_t      hspeed;
    uint16_t    hang;
    uint8_t     sat;
    uint8_t     _;
} net_jmp_t;
static net_jmp_t jmpton(const log_item_t &j) {
    net_jmp_t n = {
        .mill       = htonl(j.mill),
        .alt        = dton(j.alt),
        .vspeed     = dton(j.vspeed),
        .state      = 'U',
        .direct     = 'U',
        .lat        = dton(j.lat),
        .lng        = dton(j.lng),
        .hspeed     = dton(j.hspeed),
        .hang       = htons(j.hang),
        .sat        = j.sat
    };
    
    switch (j.state) {
        case ACST_INIT:         n.state = 'i'; break;
        case ACST_GROUND:       n.state = 'g'; break;
        case ACST_TAKEOFF40:    n.state = 's'; break;
        case ACST_TAKEOFF:      n.state = 't'; break;
        case ACST_FREEFALL:     n.state = 'f'; break;
        case ACST_CANOPY:       n.state = 'c'; break;
        case ACST_LANDING:      n.state = 'l'; break;
    }
    switch (j.direct) {
        case ACDIR_INIT:        n.direct = 'i'; break;
        case ACDIR_UP:          n.direct = 'u'; break;
        case ACDIR_NULL:        n.direct = 'n'; break;
        case ACDIR_DOWN:        n.direct = 'd'; break;
    }
    
    return n;
}

/* ------------------------------------------------------------------------------------------- *
 *  отправка на сервер
 * ------------------------------------------------------------------------------------------- */
bool sendCfg() {
    const auto cfg_d = cfg.d();
    
    struct __attribute__((__packed__)) { // Для передачи по сети
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
        .chksum     = htonl(cfg.chksum()),
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

bool sendJump() {
    const auto jmp_d = jmp.d();
    
    struct __attribute__((__packed__)) { // Для передачи по сети
        uint32_t chksum;
        uint32_t count;
    } d = {
        .chksum     = htonl(jmp.chksum()),
        .count      = htonl(jmp_d.count),
    };
    
    return srvSend(0x22, d);
}

bool sendPoint() {
    const auto pnt_d = pnt.d();
    
    struct __attribute__((__packed__)) { // Для передачи по сети
        uint8_t     num;
        uint8_t     used;
        dnet_t      lat;
        dnet_t      lng;
    } d;
    
    uint32_t chksum = htonl(pnt.chksum());
    if (!srvSend(0x23, chksum))
        return false;
    
    for (uint8_t i=0; i<PNT_COUNT; i++) {
        auto p = pnt_d.all[i];
        
        d.num   = i+1;
        d.used  = bton(p.used);
        d.lat   = dton(p.lat);
        d.lng   = dton(p.lng);
        
        if (!srvSend(0x24, d))
            return false;
    }
    
    return true;
}

static bool sendLogBookItem(const struct log_item_s<log_jmp_t> *r) {
    const auto &j = r->data;
    struct __attribute__((__packed__)) { // Для передачи по сети
        uint32_t    num;
        dt_t        dt;
        net_jmp_t   beg;
        net_jmp_t   cnp;
        net_jmp_t   end;
    } d = {
        .num    = htonl(j.num),
        .dt     = dtton(j.dt),
        .beg    = jmpton(j.beg),
        .cnp    = jmpton(j.cnp),
        .end    = jmpton(j.end)
    };
    
    return srvSend(0x32, d);
}

bool sendLogBook() {
    if (!srvSend(0x31))
        return false;
    
    bool ok = logFileRead(sendLogBookItem, PSTR(JMPLOG_SIMPLE_NAME), 1);
    
    auto cks = logChkSum(sizeof(struct log_item_s<log_jmp_t>), PSTR(JMPLOG_SIMPLE_NAME), 1);
    cks.cs = htonl(cks.cs);
    cks.sz = htonl(cks.sz);
    
    return srvSend(0x33, cks) && ok;
}

static bool sendTrackItem(const struct log_item_s <log_item_t> *r) {
    net_jmp_t d = jmpton(r->data);
    
    return srvSend(0x35, d);
}

bool sendTrack() {
    int cnt = logCount(PSTR(TRK_FILE_NAME));
    if (cnt <= 0)
        return cnt == 0;
    
    for (int num = cnt; num > 0; num--) {
        uint8_t n = num;
        if (!srvSend(0x34, n))
            return false;
        
        bool ok = logFileRead(sendTrackItem, PSTR(TRK_FILE_NAME), num);
        auto cks = logChkSum(sizeof(struct log_item_s<log_jmp_t>), PSTR(TRK_FILE_NAME), num);
        cks.cs = htonl(cks.cs);
        cks.sz = htonl(cks.sz);
        
        if (!srvSend(0x36, cks) || !ok)
            return false;
        Serial.printf("track sended ok: %d\r\n", n);
    }
    
    return true;
}

