/*
    Data convert function
*/

#include "../log.h"
#include "data.h"
#include "srv.h"
#include "../cfg/main.h"
#include "../cfg/point.h"
#include "../file/track.h"
#include "../file/wifi.h"
#include "../file/veravail.h"

#include <lwip/inet.h>      // htonl

void netsyncProgMax(size_t max);
void netsyncProgInc(size_t val);

/* ------------------------------------------------------------------------------------------- *
 *  простые преобразования данных
 * ------------------------------------------------------------------------------------------- */
uint8_t bton(bool val) {
    return val ? 1 : 0;
}
bool ntob(uint8_t n) {
    return n != 0;
}

uint16_t fton(float val) {
    return htons(round(val*100));
}
float ntof(uint16_t n) {
    float val = ntohs(n);
    return val / 100;
}

dnet_t dton(double val) {
    int32_t i = val;
    double d = val-i;
    dnet_t n = { i };
    
    if (d < 0) d = d*(-1);
    n.d = d * 0xffffffff;
    
    n.i = htonl(n.i);
    n.d = htonl(n.d);
    
    return n;
}

tm_t tmton(const tm_t &tm) {
    auto n = tm;
    n.year = htons(n.year);
    return n;
}
tm_t ntotm(const tm_t &n) {
    auto tm = n;
    tm.year = ntohs(tm.year);
    return tm;
}

logchs_t ckston(const logchs_t &cks) {
    logchs_t n;
    n.csa = htons(cks.csa);
    n.csb = htons(cks.csb);
    n.sz = htonl(cks.sz);
    return n;
}
logchs_t ntocks(const logchs_t &n) {
    logchs_t cks;
    cks.csa = ntohs(n.csa);
    cks.csb = ntohs(n.csb);
    cks.sz = ntohl(n.sz);
    return cks;
}

/* ------------------------------------------------------------------------------------------- *
 *  строковые преобразования
 * ------------------------------------------------------------------------------------------- */
int ntostrs(char *str, int strsz, uint8_t *buf, uint8_t **bufnext) {
    if ((str == NULL) || (strsz < 2) || (buf == NULL))
        return -1;
    
    uint8_t len = *buf;
    
    if (strsz <= (len))
        len = strsz-1;
    memcpy(str, buf+1, len);
    str[len] = '\0';
    
    if (bufnext != NULL)
        *bufnext = buf + 1 + *buf;
    
    return *buf + 1;
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
        case MODE_MAIN_GPSALT:  return 'm';
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
        case 'm': return MODE_MAIN_GPSALT;
        case 't': return MODE_MAIN_TIME;
    };
    return MODE_MAIN_NONE;
}

/* ------------------------------------------------------------------------------------------- *
 *  Преобразования log_item_t
 * ------------------------------------------------------------------------------------------- */
static log_item_t jmpton(const log_item_t &j) {
    log_item_t n = {
        .tmoffset   = htonl(j.tmoffset),
        .flags      = htons(j.flags),
        .state      = j.state,
        .direct     = j.direct,
        .alt        = htons(j.alt),
        .altspeed   = htons(j.altspeed),
        .lon        = htonl(j.lon),
        .lat        = htonl(j.lat),
        .hspeed     = htonl(j.hspeed),
        .heading    = htons(j.heading),
        .gpsalt     = htons(j.gpsalt),
        .vspeed     = htonl(j.vspeed),
        .gpsdage    = htonl(j.gpsdage),
        .sat        = j.sat,
        ._          = 0,
        .batval     = htons(j.batval),
        .hAcc       = htonl(j.hAcc),
        .vAcc       = htonl(j.vAcc),
        .sAcc       = htonl(j.sAcc),
        .cAcc       = htonl(j.cAcc),
        .tm         = tmton(j.tm),
    };
    
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
    
    for (uint8_t i=0; i<PNT_COUNT; i++) {
        auto p = pnt_d.all[i];
        
        d.num   = i+1;
        d.used  = bton(p.used);
        d.lat   = dton(p.lat);
        d.lng   = dton(p.lng);
        
        if (!srvSend(0x24, d))
            return false;
    }
    
    uint32_t chksum = htonl(pnt.chksum());
    if (!srvSend(0x23, chksum))
        return false;
    
    return true;
}

static bool sendLogBookItem(const struct log_item_s<log_jmp_t> *r) {
    const auto &j = r->data;
    struct __attribute__((__packed__)) { // Для передачи по сети
        uint32_t    num;
        tm_t        tm;
        log_item_t  beg;
        log_item_t  cnp;
        log_item_t  end;
    } d = {
        .num    = htonl(j.num),
        .tm     = tmton(j.tm),
        .beg    = jmpton(j.beg),
        .cnp    = jmpton(j.cnp),
        .end    = jmpton(j.end)
    };
    
    return srvSend(0x32, d);
}

bool sendLogBook(uint32_t _cks, uint32_t _pos) {
    int max;
    int32_t ibeg = 0;
    
    CONSOLE("sendLogBook: chksum: %08x, pos: %d", _cks, _pos);
    
    if (_cks > 0) {
        max = logFind(PSTR(JMPLOG_SIMPLE_NAME), sizeof(struct log_item_s<log_jmp_t>), _cks);
        if (max > 0) {// среди файлов найден какой-то по chksum, будем в нём стартовать с _pos
            CONSOLE("sendLogBook: by chksum finded num: %d; start by pos: %d", max, _pos);
            ibeg = _pos;
            if ((max == 1) && (_pos*sizeof(struct log_item_s<log_jmp_t>) >= logSize(PSTR(JMPLOG_SIMPLE_NAME)))) {
                CONSOLE("sendLogBook: by chksum finded num 1 and pos is last; no need send");
                return true;
            }
        }
        if (max <= 0) {// тот, что мы раньше передавали уже не найден, будем передавать всё заного
            CONSOLE("sendLogBook: nothing finded by chksum");
            max = logCount(PSTR(JMPLOG_SIMPLE_NAME));
        }
    }
    else { // ещё ничего не передавали, передаём всё заного
        max = logCount(PSTR(JMPLOG_SIMPLE_NAME));
    }
    
    if (max <= 0) // либо ошибка, либо вообще файлов нет - в любом случае выходим
        return max == 0;
    
    if (!srvSend(0x31))
        return false;
    
    int32_t pos = 0;
    for (int num = max; num > 0; num--) {
        pos = logFileReadMono(sendLogBookItem, PSTR(JMPLOG_SIMPLE_NAME), num, ibeg);
        if (pos < 0)
            break;
        CONSOLE("logbook sended ok: %d (ibeg: %d, pos: %d)", num, ibeg, pos);
        ibeg = 0;
    }
    auto cks = logChkSumBeg(sizeof(struct log_item_s<log_jmp_t>), PSTR(JMPLOG_SIMPLE_NAME), 1);
    
    struct __attribute__((__packed__)) { // Для передачи по сети
        uint32_t    chksum;
        uint32_t    pos;
    } d = {
        .chksum = htonl(cks),
        .pos    = pos > 0 ? htonl(pos) : 0,
    };
    
    return srvSend(0x33, d) && (pos > 0);
}

static bool sendTrackBeg(const struct log_item_s <trk_head_t> *r) {
    struct __attribute__((__packed__)) {
        uint32_t id;
        uint32_t flags;
        uint32_t jmpnum;
        tm_t     tmbeg;
    } d = {
        .id         = htonl(r->data.id),
        .flags      = htonl(r->data.flags),
        .jmpnum     = htonl(r->data.jmpnum),
        .tmbeg      = tmton(r->data.tmbeg),
    };
    
    netsyncProgInc(sizeof(const struct log_item_s <trk_head_t>));
    
    return srvSend(0x34, d);
}

static bool sendTrackItem(const struct log_item_s <log_item_t> *r) {
    log_item_t d = jmpton(r->data);
    
    if (!srvSend(0x35, d))
        return false;
    
    netsyncProgInc(sizeof(const struct log_item_s <log_item_t>));
    
    return true;
}

bool sendTrack(logchs_t _cks) {
    int max;
    int32_t ibeg = 0;
    
    CONSOLE("sendTrack: chksum: %04x%04x%08x", _cks.csa, _cks.csb, _cks.sz);
    
    if (_cks) {
        max = logFind(PSTR(TRK_FILE_NAME), sizeof(struct log_item_s<log_item_t>), _cks);
        if (max == 1) {// самый свежий и есть тот, который мы уже передавали - больше передавать не надо
            CONSOLE("sendTrack: by chksum finded num 1; no need send");
            return true;
        }
        if (max > 1) // Этот трек мы уже отправили, надо начинать со следующего
            max--;
        if (max <= 0) {// тот, что мы раньше передавали уже не найден, будем передавать всё заного
            CONSOLE("sendTrack: nothing finded by chksum");
            max = logCount(PSTR(TRK_FILE_NAME));
        }
    }
    else { // ещё ничего не передавали, передаём всё заного
        max = logCount(PSTR(TRK_FILE_NAME));
    }
    
    if (max <= 0) // либо ошибка, либо вообще файлов нет - в любом случае выходим
        return max == 0;
    
    // Общий размер файла
    size_t fsz = 0;
    for (int num = max; num > 0; num--)
        fsz += logSize(PSTR(TRK_FILE_NAME), num);
    netsyncProgMax(fsz);
    
    for (int num = max; num > 0; num--) {
        uint8_t n = num;
        
        bool ok = logFileRead(sendTrackBeg, sendTrackItem, PSTR(TRK_FILE_NAME), num) >= 0;
        auto cks = logChkSumFull(sizeof(struct log_item_s<log_item_t>), PSTR(TRK_FILE_NAME), num);
        
        if (!srvSend(0x36, ckston(cks)) || !ok)
            return false;
        CONSOLE("track sended ok: %d", n);
    }
    
    netsyncProgMax(0);
    
    return true;
}

bool sendDataFin() {
    uint32_t ckswifi = wifiPassChkSum();
    uint32_t cksver = verAvailChkSum();
    
    struct __attribute__((__packed__)) {
        uint32_t    ckswifi;
        uint32_t    cksver;
        uint8_t     vern1;
        uint8_t     vern2;
        uint8_t     vern3;
        uint8_t     vtype;
        uint8_t     hwver;
        uint8_t     _[3];
        char        fwupdver[32];
    } d = {
        .ckswifi    = htonl(ckswifi),
        .cksver     = htonl(cksver),
        .vern1      = FWVER_NUM1,
        .vern2      = FWVER_NUM2,
        .vern3      = FWVER_NUM3,
        .vtype      = FWVER_TYPE_CODE,
        .hwver      = HWVER,
    };
    
    if (cfg.d().fwupdind > 0) {
        verAvailGet(cfg.d().fwupdind, d.fwupdver);
    }
    
    return srvSend(0x3f, d); // datafin
}

