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
#include "../cfg/point.h"
#include "../core/filetxt.h"
#include "../jump/logbook.h"


#define SOCK                BinProtoSend(nsock, '%')
#define SEND(cmd, pk, ...)  SOCK.send(cmd, PSTR(pk), ##__VA_ARGS__)
#define SND(cmd)            SOCK.send(cmd)

/* ------------------------------------------------------------------------------------------- *
 *  Основной конфиг
 * ------------------------------------------------------------------------------------------- */
bool sendCfgMain(NetSocket *nsock) {
    const auto &cfg_d = cfg.d();
    struct __attribute__((__packed__)) {
        uint32_t chksum;
        uint8_t contrast;
        int16_t timezone;
    
        bool gndmanual;
        bool gndauto;
    
        bool    dsplautoff;
        int8_t  dsplcnp;
        int8_t  dsplland;
        int8_t  dsplgnd;
        int8_t  dsplpwron;
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
    
    return SEND( 0x21, "XCnbbbaaaa", d );
}

/* ------------------------------------------------------------------------------------------- *
 *  Количество прыгов
 * ------------------------------------------------------------------------------------------- */
bool sendJmpCount(NetSocket *nsock) {
    struct __attribute__((__packed__)) { // Для передачи по сети
        uint32_t chksum;
        uint32_t count;
    } d = {
        .chksum     = jmp.chksum(),
        .count      = jmp.d().count,
    };
    
    return SEND( 0x22, "XN", d );
}

/* ------------------------------------------------------------------------------------------- *
 *  Отправка Navi-точек
 * ------------------------------------------------------------------------------------------- */
bool sendPoint(NetSocket *nsock) {
    const auto pnt_d = pnt.d();
    
    for (uint8_t i=0; i<PNT_COUNT; i++) {
        auto p = pnt_d.all[i];
        
        struct __attribute__((__packed__)) { // Для передачи по сети
            uint8_t     num;
            bool        used;
            double      lat;
            double      lng;
        } d = { static_cast<uint8_t>(i+1), p.used, p.lat, p.lng };
        
        if (!SEND(0x24, "CCDD", d))
            return false;
    }
    
    return SEND(0x23, "X", pnt.chksum());
}

/* ------------------------------------------------------------------------------------------- *
 *  Логбук
 *  В будущем надо переделать через worker,
 *  в т.ч. разбить lb.findfile(...);
 *  Пока временно рассчитываем, что тут за раз не будет много прыжков
 * ------------------------------------------------------------------------------------------- */
bool sendLogBook(NetSocket *nsock, uint32_t _cks, uint32_t _pos) {
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
    
    if (!SND(0x31))
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
    
            if (!SEND(0x32, "NNT" LOG_PK LOG_PK LOG_PK LOG_PK, jmp))
                return false;
        }
        pos = lb.pos();
        if (pos < 0)
            break;
        CONSOLE("logbook sended ok: %d (beg: %d, pos: %d)", n, beg, pos);
        lb.close();
    }
    auto cks = lb.chksum(1);
    
    struct __attribute__((__packed__)) {
        uint32_t    chksum;
        int32_t     pos;
    } d = { cks, pos > 0 ? pos : 0 };
    
    return SEND(0x33, "XN", d) && (pos > 0);
}

/* ------------------------------------------------------------------------------------------- *
 *  Завершение отправки данных на сервер
 * ------------------------------------------------------------------------------------------- */
bool sendDataFin(NetSocket *nsock) {
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
        char        fwupdver[33];
    } d = {
        .ckswifi    = ckswifi,
        .cksver     = cksver,
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
    
    return SEND( 0x3f, "XXCCCaC   a32", d ); // datafin
}
