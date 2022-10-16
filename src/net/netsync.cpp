/*
    Data transfere functions
*/

#include "netsync.h"
#include "../log.h"
#include "../core/workerloc.h"
#include "binproto.h"
#include "../view/text.h"
#include "../cfg/main.h"
#include "../cfg/jump.h"
#include "../cfg/point.h"
#include "../core/filetxt.h"
#include "../jump/logbook.h"

#include <Update.h>         // Обновление прошивки

#define WRK_RETURN_ERR(s, ...)  do { CONSOLE(s, ##__VA_ARGS__); WRK_RETURN_END; } while (0)
#define SEND(cmd, ...)          if (!m_pro || !m_pro->send(cmd, ##__VA_ARGS__)) WRK_RETURN_ERR("send fail")
#define RECV(pk, ...)           if (!m_pro || !m_pro->rcvdata(PSTR(pk), ##__VA_ARGS__)) WRK_RETURN_ERR("recv data fail")
#define RECVRAW(data)           m_pro ? m_pro->rcvraw(data, sizeof(data)) : -1
#define RECVNEXT()              if (!m_pro || !m_pro->rcvnext()) WRK_RETURN_ERR("recv data fail")
#define WRK_BREAK_RECV          WRK_BREAK \
                                if (!m_pro || !m_pro->rcvvalid()) WRK_RETURN_ERR("recv not valid"); \
                                if (m_pro->rcvstate() < BinProto::RCV_COMPLETE) return chktimeout();

/* ------------------------------------------------------------------------------------------- *
 *  Основной конфиг
 * ------------------------------------------------------------------------------------------- */
bool sendCfgMain(BinProto *pro) {
    if (pro == NULL)
        return false;
    
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
    
    return pro->send( 0x21, "XCnbbbaaaa", d );
}

/* ------------------------------------------------------------------------------------------- *
 *  Количество прыгов
 * ------------------------------------------------------------------------------------------- */
bool sendJmpCount(BinProto *pro) {
    if (pro == NULL)
        return false;
    
    struct __attribute__((__packed__)) { // Для передачи по сети
        uint32_t chksum;
        uint32_t count;
    } d = {
        .chksum     = jmp.chksum(),
        .count      = jmp.d().count,
    };
    
    return pro->send( 0x22, "XN", d );
}

/* ------------------------------------------------------------------------------------------- *
 *  Отправка Navi-точек
 * ------------------------------------------------------------------------------------------- */
bool sendPoint(BinProto *pro) {
    if (pro == NULL)
        return false;
    
    const auto pnt_d = pnt.d();
    
    for (uint8_t i=0; i<PNT_COUNT; i++) {
        auto p = pnt_d.all[i];
        
        struct __attribute__((__packed__)) { // Для передачи по сети
            uint8_t     num;
            bool        used;
            double      lat;
            double      lng;
        } d = { static_cast<uint8_t>(i+1), p.used, p.lat, p.lng };
        
        if (!pro->send(0x24, "CCDD", d))
            return false;
    }
    
    return pro->send(0x23, "X", pnt.chksum());
}

/* ------------------------------------------------------------------------------------------- *
 *  Логбук
 *  В будущем надо lb.findfile(...) переделать через worker.
 *
 *  Эта процедура будет всегда отправлять 0x31/0x33, даже если не найдено новых прыжков
 *  Для wifisync это необязательно, а вот при синхре из приложения, эту будет ответ запрос,
 *  на который надо обязательно ответить
 * ------------------------------------------------------------------------------------------- */
WRK_DEFINE(SEND_LOGBOOK) {
    private:
        int m_fn;
        bool m_isok, m_begsnd;
        FileLogBook m_lb;
        
        BinProto *m_pro;
        uint32_t m_cks;
        int32_t  m_pos;
    
    public:
        bool isok() const { return m_isok; }
    
    WRK_CLASS(SEND_LOGBOOK)(BinProto *pro, uint32_t cks, int32_t pos, bool noremove = false) :
        m_isok(false),
        m_begsnd(false),
        m_pro(pro),
        m_cks(cks),
        m_pos(pos)
    {
        if (noremove)
            optset(O_NOREMOVE);
    }
    
    WRK_PROCESS
        CONSOLE("sendLogBook: chksum: %08x, pos: %d", m_cks, m_pos);
        if (m_pro == NULL)
            WRK_RETURN_ERR("pro is NULL");
    
    WRK_BREAK_RUN
        // Ищем файл логбука, с которого начнём
        m_fn =
            m_cks > 0 ?
                m_lb.findfile(m_cks) :
                0;
        
        if ((m_cks > 0) && (m_fn <= 0)) {
            CONSOLE("sendLogBook: nothing finded by chksum");
        }
            
        if (m_fn > 0) {// среди файлов найден какой-то по chksum, будем в нём стартовать с _pos
            CONSOLE("sendLogBook: by chksum finded num: %d; start by pos: %d", m_fn, m_pos);
            if (!m_lb.open(m_fn))
                WRK_RETURN_ERR("Can't open logbook num=%d", m_fn);
            if (!m_lb.seekto(m_pos))
                WRK_RETURN_ERR("Can't logbook num=%d seek to pos=%d", m_fn, m_pos);
        }
        else
            m_fn = m_lb.count();
        
        if (m_fn < 0)       // ошибка поиска
            WRK_RETURN_ERR("Fail find file by cks=%08x", m_cks);
        
    WRK_BREAK_RUN
        SEND(0x31);
        m_pos = 0;
        m_begsnd = true;
        
    WRK_BREAK_RUN
        if (m_fn > 0) {
            if (!m_lb && !m_lb.open(m_fn))
                WRK_RETURN_ERR("Can't open logbook num=%d", m_fn);
            CONSOLE("logbook num=%d, pos=%d, avail=%d", m_fn, m_lb.pos(), m_lb.avail());
            if (m_lb.avail() > 0) {
                FileLogBook::item_t jmp;
                if (!m_lb.get(jmp))
                    WRK_RETURN_ERR("Can't get item jump");
                
                SEND(0x32, "NNT" LOG_PK LOG_PK LOG_PK LOG_PK, jmp);
            }
            
            if (m_lb.avail() <= 0) {
                if (m_fn > 1) {
                    m_lb.close();
                    CONSOLE("logbook closed num=%d", m_fn);
                }
                else {// Завершился процесс
                    m_isok = true;
                    CONSOLE("logbook finished");
                }
                m_fn--;
            }
        }
    WRK_END
        
    void end() {
        CONSOLE("lb isopen: %d", m_lb ? 1 : 0);
        if (m_begsnd) {
            struct __attribute__((__packed__)) {
                uint32_t    chksum;
                int32_t     pos;
            } d = { 
                m_lb ? m_lb.chksum() : 0,
                m_lb ? static_cast<int32_t>(m_lb.pos()) : 0
            };
            
            if (m_pro)
                m_pro->send(0x33, "XN", d);
        }
        
        m_lb.close();
    }
};

WrkProc::key_t sendLogBook(BinProto *pro, uint32_t cks, int32_t pos, bool noremove) {
    if (pro == NULL)
        return WRKKEY_NONE;
    
    if (!noremove)
        return wrkRand(SEND_LOGBOOK, pro, cks, pos, noremove);
    
    wrkRun(SEND_LOGBOOK, pro, cks, pos, noremove);
    return WRKKEY_SEND_LOGBOOK;
}

bool isokLogBook(const WrkProc *_wrk) {
    const auto wrk = 
        _wrk == NULL ?
            wrkGet(SEND_LOGBOOK) :
            reinterpret_cast<const WRK_CLASS(SEND_LOGBOOK) *>(_wrk);
    
    return (wrk != NULL) && (wrk->isok());
}


/* ------------------------------------------------------------------------------------------- *
 *  Завершение отправки данных на сервер
 * ------------------------------------------------------------------------------------------- */
bool sendDataFin(BinProto *pro) {
    if (pro == NULL)
        return false;
    
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
        // uint8_t     _[3]; - это не нужно тут указывать,
                            // т.к. мы в pk используем модификаторы ' '
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
        CONSOLE("update ver: {%s}", d.fwupdver);
    }
    
    return pro->send( 0x3f, "XXCCCaC   a32", d ); // datafin
}


/* ------------------------------------------------------------------------------------------- *
 *  Приём wifi-паролей
 * ------------------------------------------------------------------------------------------- */
WRK_DEFINE(RECV_WIFIPASS) {
    private:
        bool        m_isok;
        uint16_t    m_timeout;
        
        BinProto *m_pro;
        FileTxt fh;
        
        const char *m_fname;
    
    public:
        bool isok() const { return m_isok; }

        state_t chktimeout() {
            if (m_timeout == 0)
                WRK_RETURN_WAIT;
            
            m_timeout--;
            if (m_timeout > 0)
                WRK_RETURN_WAIT;
            
            WRK_RETURN_ERR("Wait timeout");
        }
    
    WRK_CLASS(RECV_WIFIPASS)(BinProto *pro, bool noremove = false) :
        m_isok(false),
        m_timeout(0),
        m_pro(pro),
        m_fname(PSTR(WIFIPASS_FILE))
    {
        if (noremove)
            optset(O_NOREMOVE);
    }

    state_t every() {
        m_pro->rcvprocess();
        if (!m_pro->rcvvalid())
            WRK_RETURN_ERR("recv data fail");
        
        WRK_RETURN_RUN;
    }
    
    WRK_PROCESS
        if ( fileExists(m_fname) &&
            !fileRemove(m_fname))
            WRK_RETURN_ERR("Can't remove prev wifi-file");
    
    WRK_BREAK_RUN
        if (!fh.open_P(m_fname, FileMy::MODE_APPEND))
            WRK_RETURN_ERR("Can't open wifi-file for write");
        
        m_timeout = 200;
    WRK_BREAK_RECV
        if (m_pro->rcvcmd() != 0x41)
            WRK_RETURN_ERR("Recv wrong cmd=0x%02x", m_pro->rcvcmd());
        RECVNEXT();  // Эта команда без данных,
                            // нам надо перейти к приёму следующей команды
    
        m_timeout = 200;
    WRK_BREAK_RECV
        switch (m_pro->rcvcmd()) {
            case 0x42: { // wifi net
                struct __attribute__((__packed__)) {
                    char ssid[BINPROTO_STRSZ];
                    char pass[BINPROTO_STRSZ];
                } d;
                RECV("ss", d);
                
                CONSOLE("add wifi: {%s}, {%s}", d.ssid, d.pass);
                if (
                        !fh.print_param(PSTR("ssid"), d.ssid) ||
                        !fh.print_param(PSTR("pass"), d.pass)
                    )
                    WRK_RETURN_ERR("Fail write ssid/pass");
                m_timeout = 200;
                WRK_RETURN_RUN;
            }

            case 0x43: { // wifi end
                RECVNEXT();  // Эта команда без данных
                
                fh.close(); // надо переоткрыть файл, иначе из него нельзя прочитать
                if (!fh.open_P(m_fname))
                    WRK_RETURN_ERR("Can't open wifi-file for chksum");
                uint32_t cks = fh.chksum();
                fh.close();
                
                SEND(0x4a, "N", cks); // wifiok
                m_isok = true;
                break;
            }

            default: WRK_RETURN_ERR("Recv unknown cmd=0x%02x", m_pro->rcvcmd());
        }
    
    WRK_END
};

WrkProc::key_t recvWiFiPass(BinProto *pro, bool noremove) {
    if (pro == NULL)
        return WRKKEY_NONE;
    
    if (!noremove)
        return wrkRand(RECV_WIFIPASS, pro, noremove);
    
    wrkRun(RECV_WIFIPASS, pro, noremove);
    return WRKKEY_RECV_WIFIPASS;
}

bool isokWiFiPass(const WrkProc *_wrk) {
    const auto wrk = 
        _wrk == NULL ?
            wrkGet(RECV_WIFIPASS) :
            reinterpret_cast<const WRK_CLASS(RECV_WIFIPASS) *>(_wrk);
    
    return (wrk != NULL) && (wrk->isok());
}

/* ------------------------------------------------------------------------------------------- *
 *  Приём veravail - доступных версий прошивки
 * ------------------------------------------------------------------------------------------- */
WRK_DEFINE(RECV_VERAVAIL) {
    private:
        bool        m_isok;
        uint16_t    m_timeout;
        
        BinProto *m_pro;
        FileTxt fh;
        
        const char *m_fname;
    
    public:
        bool isok() const { return m_isok; }

        state_t chktimeout() {
            if (m_timeout == 0)
                WRK_RETURN_WAIT;
            
            m_timeout--;
            if (m_timeout > 0)
                WRK_RETURN_WAIT;
            
            WRK_RETURN_ERR("Wait timeout");
        }
    
    WRK_CLASS(RECV_VERAVAIL)(BinProto *pro, bool noremove = false) :
        m_isok(false),
        m_timeout(0),
        m_pro(pro),
        m_fname(PSTR(VERAVAIL_FILE))
    {
        if (noremove)
            optset(O_NOREMOVE);
    }

    state_t every() {
        m_pro->rcvprocess();
        if (!m_pro->rcvvalid())
            WRK_RETURN_ERR("recv data fail");
        
        WRK_RETURN_RUN;
    }
    
    WRK_PROCESS
        if ( fileExists(m_fname) &&
            !fileRemove(m_fname))
            WRK_RETURN_ERR("Can't remove prev veravail-file");
    
    WRK_BREAK_RUN
        if (!fh.open_P(m_fname, FileMy::MODE_APPEND))
            WRK_RETURN_ERR("Can't open veravail-file for write");
        
        m_timeout = 200;
    WRK_BREAK_RECV
        if (m_pro->rcvcmd() != 0x44)
            WRK_RETURN_ERR("Recv wrong cmd=0x%02x", m_pro->rcvcmd());
        RECVNEXT();  // Эта команда без данных,
                            // нам надо перейти к приёму следующей команды
    
        m_timeout = 200;
    WRK_BREAK_RECV
        switch (m_pro->rcvcmd()) {
            case 0x45: { // veravail item
                char ver[BINPROTO_STRSZ];
                // тут нужен вызов через два аргумента (ссылка на данные и sizeof()),
                // чтобы верно передалась ссылка и размер выделенной памяти
                RECV("s", ver, sizeof(ver));
                
                CONSOLE("add veravail: {%s}", ver);
                if ( !fh.print_line(ver) )
                    WRK_RETURN_ERR("Fail write version");
                m_timeout = 200;
                WRK_RETURN_RUN;
            }

            case 0x46: { // veravail end
                RECVNEXT();  // Эта команда без данных
                
                fh.close(); // надо переоткрыть файл, иначе из него нельзя прочитать
                if (!fh.open_P(m_fname))
                    WRK_RETURN_ERR("Can't open veravail-file for chksum");
                uint32_t cks = fh.chksum();
                fh.close();
                
                SEND(0x4b, "N", cks); // veravail ok
                m_isok = true;
                break;
            }

            default: WRK_RETURN_ERR("Recv unknown cmd=0x%02x", m_pro->rcvcmd());
        }
    
    WRK_END
};

WrkProc::key_t recvVerAvail(BinProto *pro, bool noremove) {
    if (pro == NULL)
        return WRKKEY_NONE;
    
    if (!noremove)
        return wrkRand(RECV_VERAVAIL, pro, noremove);
    
    wrkRun(RECV_VERAVAIL, pro, noremove);
    return WRKKEY_RECV_VERAVAIL;
}

bool isokVerAvail(const WrkProc *_wrk) {
    const auto wrk = 
        _wrk == NULL ?
            wrkGet(RECV_VERAVAIL) :
            reinterpret_cast<const WRK_CLASS(RECV_VERAVAIL) *>(_wrk);
    
    return (wrk != NULL) && (wrk->isok());
}

/* ------------------------------------------------------------------------------------------- *
 *  Обновление прошивки по сети
 * ------------------------------------------------------------------------------------------- */
WRK_DEFINE(RECV_FIRMWARE) {
    private:
        bool        m_isok;
        uint16_t    m_timeout;
        
        BinProto *m_pro;
    
    public:
        uint32_t    m_sz, m_rcv;
        bool isok() const { return m_isok; }

        state_t chktimeout() {
            if (m_timeout == 0)
                WRK_RETURN_WAIT;
            
            m_timeout--;
            if (m_timeout > 0)
                WRK_RETURN_WAIT;
            
            WRK_RETURN_ERR("Wait timeout");
        }
    
    WRK_CLASS(RECV_FIRMWARE)(BinProto *pro, bool noremove = false) :
        m_isok(false),
        m_timeout(0),
        m_pro(pro),
        m_sz(0),
        m_rcv(0)
    {
        if (noremove)
            optset(O_NOREMOVE);
    }

    state_t every() {
        m_pro->rcvprocess();
        if (!m_pro->rcvvalid())
            WRK_RETURN_ERR("recv data fail");
        
        WRK_RETURN_RUN;
    }
    
    WRK_PROCESS
        
        m_timeout = 200;
    WRK_BREAK_RECV
        if (m_pro->rcvcmd() != 0x47)
            WRK_RETURN_ERR("Recv wrong cmd=0x%02x begin", m_pro->rcvcmd());
        RECVNEXT();  // Эта команда без данных,
        
        m_timeout = 200;
    WRK_BREAK_RECV
        if (m_pro->rcvcmd() != 0x48)
            WRK_RETURN_ERR("Recv wrong cmd=0x%02x info", m_pro->rcvcmd());
        
        struct {
            uint32_t    size;
            char        md5[37];
        } info;
        RECV("Na36", info);
        CONSOLE("recv fw info: size: %lu; md5: %s", info.size, info.md5);
        
        uint32_t freesz = ESP.getFreeSketchSpace();
        uint32_t cursz = ESP.getSketchSize();
        CONSOLE("current fw size: %lu, avail size for new fw: %lu", cursz, freesz);
        
        if (info.size > freesz)
            WRK_RETURN_ERR("FW size too big: %lu > %lu", info.size, freesz);

        // start burn
        if (!Update.begin(info.size, U_FLASH) || !Update.setMD5(info.md5))
            WRK_RETURN_ERR("Upd begin fail: errno=%d", Update.getError());
        
        m_sz = info.size;
    
        m_timeout = 200;
    WRK_BREAK_RECV
        switch (m_pro->rcvcmd()) {
            case 0x49: { // fwupd data
                uint16_t    sz;
                uint8_t     buf[1000];
                
                RECV("n", sz);
                int sz1 = RECVRAW(buf);
                if ((sz1 == 0) || (sz1 != sz))
                    WRK_RETURN_ERR("Recv sz wrong: %d <-> %u", sz1, sz);
                sz1 = Update.write(buf, sz);
                if ((sz1 == 0) || (sz1 != sz))
                    WRK_RETURN_ERR("Burn sz wrong: %d <-> %u", sz1, sz);
                
                m_rcv += sz;
                
                m_timeout = 200;
                WRK_RETURN_RUN;
            }

            case 0x4a: { // fwupd end
                RECVNEXT();  // Эта команда без данных
                if (!Update.end())
                    WRK_RETURN_ERR("Finalize fail: errno=%d", Update.getError());
                
                cfg.set().fwupdind = 0;
                if (!cfg.save())
                    WRK_RETURN_ERR("Config save fail");
                
                SEND(0x4c); // fwupd ok
                
                m_isok = true;
                break;
            }

            default: WRK_RETURN_ERR("Recv unknown cmd=0x%02x", m_pro->rcvcmd());
        }
    
    WRK_END
};

WrkProc::key_t recvFirmware(BinProto *pro, bool noremove) {
    if (pro == NULL)
        return WRKKEY_NONE;
    
    if (!noremove)
        return wrkRand(RECV_FIRMWARE, pro, noremove);
    
    wrkRun(RECV_FIRMWARE, pro, noremove);
    return WRKKEY_RECV_FIRMWARE;
}

bool isokFirmware(const WrkProc *_wrk) {
    const auto wrk = 
        _wrk == NULL ?
            wrkGet(RECV_FIRMWARE) :
            reinterpret_cast<const WRK_CLASS(RECV_FIRMWARE) *>(_wrk);
    
    return (wrk != NULL) && (wrk->isok());
}

cmpl_t cmplFirmware(const WrkProc *_wrk) {
    const auto wrk = 
        _wrk == NULL ?
            wrkGet(RECV_FIRMWARE) :
            reinterpret_cast<const WRK_CLASS(RECV_FIRMWARE) *>(_wrk);
    
    if (wrk == NULL)
        return { 0, 0 };
    return { wrk->m_rcv, wrk->m_sz };
}
