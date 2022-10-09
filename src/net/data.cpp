/*
    Data convert function
*/

#include "../log.h"
#include "data.h"
#include "srv.h"
#include "../cfg/main.h"
#include "../cfg/point.h"
#include "../core/filetxt.h"
#include "../jump/logbook.h"

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

FileTrack::chs_t ckston(const FileTrack::chs_t &cks) {
    FileTrack::chs_t n;
    n.csa = htons(cks.csa);
    n.csb = htons(cks.csb);
    n.sz = htonl(cks.sz);
    return n;
}
FileTrack::chs_t ntocks(const FileTrack::chs_t &n) {
    FileTrack::chs_t cks;
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
        case MODE_MAIN_ALTNAV:  return 'm';
        case MODE_MAIN_ALT:     return 'a';
    };
    return 'U';
}
static int8_t ntomode(char n) {
    switch (n) {
        case 'N': return MODE_MAIN_NONE;
        case 'L': return MODE_MAIN_LAST;
        case 'm': return MODE_MAIN_ALTNAV;
        case 'a': return MODE_MAIN_ALT;
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
        //.tm         = tmton(j.tm),
        .millis     = htonl(j.millis),
        .msave      = htonl(j.msave),
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

bool sendLogBook(uint32_t _cks, uint32_t _pos) {
    int max;
    FileLogBook lb;
    
    CONSOLE("sendLogBook: chksum: %08x, pos: %d", _cks, _pos);
    
    if (_cks > 0) {
        max = lb.findfile(_cks);
        if (max > 0) {// среди файлов найден какой-то по chksum, будем в нём стартовать с _pos
            CONSOLE("sendLogBook: by chksum finded num: %d; start by pos: %d", max, _pos);
            if (!lb.open(max))
                return false;
            if ((max == 1) && (_pos >= lb.sizefile())) {
                CONSOLE("sendLogBook: by chksum finded num 1 and pos is last; no need send");
                return true;
            }
            if (!lb.seekto(_pos))
                return false;
        }
        else { // тот, что мы раньше передавали уже не найден, будем передавать всё заного
            CONSOLE("sendLogBook: nothing finded by chksum");
            max = lb.count();
        }
    }
    else { // ещё ничего не передавали, передаём всё заного
        max = lb.count();
    }
    
    if (max <= 0) // либо ошибка, либо вообще файлов нет - в любом случае выходим
        return max == 0;
    
    if (!srvSend(0x31))
        return false;
    
    int32_t pos = 0;
    for (int n = max; n > 0; n--) {
        if (!lb && !lb.open(n)) {
            pos = 0;
            break;
        }
        size_t beg = lb.pos();
        while (lb.avail() > 0) {
            FileLogBook::item_t jmp;
            if (!lb.get(jmp))
                break;

            struct __attribute__((__packed__)) { // Для передачи по сети
                uint32_t    num;
                uint32_t    key;
                tm_t        tm;
                log_item_t  toff;
                log_item_t  beg;
                log_item_t  cnp;
                log_item_t  end;
            } d = {
                .num    = htonl(jmp.num),
                .key    = htonl(jmp.key),
                .tm     = tmton(jmp.tm),
                .toff   = jmpton(jmp.toff),
                .beg    = jmpton(jmp.beg),
                .cnp    = jmpton(jmp.cnp),
                .end    = jmpton(jmp.end)
            };
    
            if (!srvSend(0x32, d))
                return false;
        }
        pos = lb.pos();
        if (pos < 0)
            break;
        CONSOLE("logbook sended ok: %d (beg: %d, pos: %d)", n, beg, pos);
        lb.close();
    }
    auto cks = lb.chksum(1);
    
    struct __attribute__((__packed__)) { // Для передачи по сети
        uint32_t    chksum;
        uint32_t    pos;
    } d = {
        .chksum = htonl(cks),
        .pos    = pos > 0 ? htonl(pos) : 0,
    };
    
    return srvSend(0x33, d) && (pos > 0);
}

bool sendTrack(FileTrack::chs_t _cks) {
    int max;
    int32_t ibeg = 0;
    
    CONSOLE("sendTrack: chksum: %04x%04x%08x", _cks.csa, _cks.csb, _cks.sz);
    
    if (_cks) {
        max = FileTrack().findfile(_cks);
        if (max == 1) {// самый свежий и есть тот, который мы уже передавали - больше передавать не надо
            CONSOLE("sendTrack: by chksum finded num 1; no need send");
            return true;
        }
        if (max > 1) // Этот трек мы уже отправили, надо начинать со следующего
            max--;
        if (max <= 0) {// тот, что мы раньше передавали уже не найден, будем передавать всё заного
            CONSOLE("sendTrack: nothing finded by chksum");
            max = FileTrack().count();
        }
    }
    else { // ещё ничего не передавали, передаём всё заного
        max = FileTrack().count();
    }
    
    if (max <= 0) // либо ошибка, либо вообще файлов нет - в любом случае выходим
        return max == 0;
    
    // Общий размер файла
    size_t fsz = 0;
    for (int n = max; n > 0; n--) {
        FileTrack tr(n);
        fsz += 1+tr.sizefile();
    }
    netsyncProgMax(fsz);
    
    for (int n = max; n > 0; n--) {
        FileTrack tr(n);
        
        FileTrack::head_t th;
        if (tr.get(th)) {
            struct __attribute__((__packed__)) {
                uint32_t id;
                uint32_t flags;
                uint32_t jmpnum;
                uint32_t jmpkey;
                tm_t     tmbeg;
            } d = {
                .id         = htonl(th.id),
                .flags      = htonl(th.flags),
                .jmpnum     = htonl(th.jmpnum),
                .jmpkey     = htonl(th.jmpkey),
                .tmbeg      = tmton(th.tmbeg),
            };
    
            if (!srvSend(0x34, d))
                return false;
    
            netsyncProgInc(1);
        }
        else
            return false;
        
        while (tr.available() >= tr.sizeitem()) {
            FileTrack::item_t ti;
            if (!tr.get(ti))
                return false;
            ti = jmpton(ti);
            if (!srvSend(0x35, ti))
                return false;
            
            netsyncProgInc(1);
        }
        
        auto cks = tr.chksum();
        
        if (!srvSend(0x36, ckston(cks)))
            return false;
        CONSOLE("track sended ok: %d", n);
    }
    
    netsyncProgMax(0);
    
    return true;
}

bool sendDataFin() {
    FileTxt f(PSTR(WIFIPASS_FILE));
    uint32_t ckswifi = f.chksum();
    f.close();
    f.open_P(PSTR(VERAVAIL_FILE));
    uint32_t cksver = f.chksum();
    f.close();
    
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
        FileTxt f(PSTR(VERAVAIL_FILE));
        if (!f.seek2line(cfg.d().fwupdind-1) ||
            (f.read_line(d.fwupdver, sizeof(d.fwupdver)) < 1))
            d.fwupdver[0] = '\0';
        f.close();
        CONSOLE("update ver: %s", d.fwupdver);
    }
    
    return srvSend(0x3f, d); // datafin
}

