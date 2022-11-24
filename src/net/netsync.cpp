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
#include "../jump/track.h"
#include "../view/main.h"
#include "../view/netauth.h"

#include <Update.h>         // Обновление прошивки
#include "esp_system.h" // esp_random

#define WRK_RETURN_ERR(s, ...)  do { CONSOLE(s, ##__VA_ARGS__); WRK_RETURN_END; } while (0)
#define SEND(cmd, ...)          if (!m_pro || !m_pro->send(cmd, ##__VA_ARGS__)) WRK_RETURN_ERR("send fail")
#define RECV(pk, ...)           if (!m_pro || !m_pro->rcvdata(PSTR(pk), ##__VA_ARGS__)) WRK_RETURN_ERR("recv data fail")
#define RECVRAW(data)           m_pro ? m_pro->rcvraw(data, sizeof(data)) : -1
#define RECVNEXT()              if (!m_pro || !m_pro->rcvnext()) WRK_RETURN_ERR("recv data fail")
#define CHK_RECV                if (!m_pro || !m_pro->rcvvalid()) WRK_RETURN_ERR("recv not valid"); \
                                if (m_pro->rcvstate() < BinProto::RCV_COMPLETE) return chktimeout();
#define WRK_BREAK_RECV          WRK_BREAK CHK_RECV

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
        uint32_t m_cks, m_beg, m_cnt, m_pos;
        enum {
            byCks,
            byBeg
        } m_meth;
    
    public:
        bool isok() const { return m_isok; }
    
    WRK_CLASS(SEND_LOGBOOK)(BinProto *pro, uint32_t cks, uint32_t pos, bool noremove = false) :
        m_isok(false),
        m_begsnd(false),
        m_pro(pro),
        m_cks(cks),
        m_beg(pos),
        m_cnt(0),
        m_pos(0),
        m_meth(byCks)
    {
        if (noremove)
            optset(O_NOREMOVE);
    }
    
    WRK_CLASS(SEND_LOGBOOK)(BinProto *pro, const posi_t &posi, bool noremove = false) :
        // struct в аргументах, чтобы вычленить нужный метод по аргументам, иначе они такие же
        m_fn(0),
        m_isok(false),
        m_begsnd(false),
        m_pro(pro),
        m_cks(0),
        m_beg(posi.beg),
        m_cnt(posi.count),
        m_pos(0),
        m_meth(byBeg)
    {
        if (noremove)
            optset(O_NOREMOVE);
    }
    
    WRK_PROCESS
        CONSOLE("sendLogBook: chksum: %08x, beg: %lu, count: %lu", m_cks, m_beg, m_cnt);
        if (m_pro == NULL)
            WRK_RETURN_ERR("pro is NULL");
    
    WRK_BREAK_RUN
        // Ищем файл логбука, с которого начнём
        if (m_meth == byCks) {
            m_fn =
                m_cks > 0 ?
                    m_lb.findfile(m_cks) :
                    0;
        
            if ((m_cks > 0) && (m_fn <= 0)) {
                CONSOLE("sendLogBook: nothing finded by chksum");
            }
            
            if (m_fn > 0) {// среди файлов найден какой-то по chksum, будем в нём стартовать с _pos
                CONSOLE("sendLogBook: by chksum finded num: %d; start by pos: %lu", m_fn, m_beg);
                if (!m_lb.open(m_fn))
                    WRK_RETURN_ERR("Can't open logbook num=%d", m_fn);
                if (!m_lb.seekto(m_beg))
                    WRK_RETURN_ERR("Can't logbook num=%d seek to pos=%lu", m_fn, m_beg);
            }
            else
                m_fn = m_lb.count();
        
            if (m_fn < 0)       // ошибка поиска
                WRK_RETURN_ERR("Fail find file by cks=%08x", m_cks);
        }
        else
        if (m_meth == byBeg) {
            if ((m_fn < 99) && (m_lb.exists(m_fn + 1))) {
                m_fn++;
                if (m_lb)
                    m_lb.close();
                if (!m_lb.open(m_fn))
                    WRK_RETURN_ERR("Can't open logbook num=%d", m_fn);
                auto cnt = m_lb.sizefile();
                if (cnt < 1)
                    WRK_RETURN_ERR("Empty logbook file num=%d", m_fn);
                CONSOLE("logbook file num=%d; all rec count=%d", m_fn, cnt);
                m_pos += cnt;
                if (m_pos < m_beg) // ещё не нашли нужной позиции, пойдём к следующему файлу
                     WRK_RETURN_RUN;
                m_beg = m_pos-m_beg;
                if ((m_beg > 0) && !m_lb.seekto(m_beg))
                    WRK_RETURN_ERR("Can't logbook num=%d seek to pos=%lu", m_fn, m_beg);
                m_pos -= m_beg;
            }
            else
                m_beg = 0;
            
            CONSOLE("Begin send from num=%d; start by pos: %lu", m_fn, m_beg);
        }
        else
            WRK_RETURN_ERR("Unknown meth: %d", m_meth);
        
    WRK_BREAK_RUN
        struct {
            uint32_t beg, count;
        } d = { m_pos, m_cnt };
        SEND(0x31, "NN", d);
        m_begsnd = true;
        
        if (m_fn == 0)
            m_isok = true;
        
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
                if (m_cnt > 0) {
                    m_cnt--;
                    if (m_cnt == 0) {
                        // отправили нужное количество
                        m_isok = true;
                        CONSOLE("logbook need count finished");
                        WRK_RETURN_END;
                    }
                }
            }
            
            if (m_lb.avail() <= 0) {
                if (m_fn > 1) {
                    m_lb.close();
                    CONSOLE("logbook closed num=%d", m_fn);
                }
                else {// Завершился процесс
                    m_isok = true;
                    CONSOLE("logbook finished");
                    WRK_RETURN_END;
                }
                m_fn--;
            }
            WRK_RETURN_RUN;
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

WrkProc::key_t sendLogBook(BinProto *pro, uint32_t cks, uint32_t pos, bool noremove) {
    if (pro == NULL)
        return WRKKEY_NONE;
    
    if (!noremove)
        return wrkRand(SEND_LOGBOOK, pro, cks, pos, noremove);
    
    wrkRun(SEND_LOGBOOK, pro, cks, pos, noremove);
    return WRKKEY_SEND_LOGBOOK;
}
WrkProc::key_t sendLogBook(BinProto *pro, const posi_t &posi, bool noremove) {
    if (pro == NULL)
        return WRKKEY_NONE;
    
    if (!noremove)
        return wrkRand(SEND_LOGBOOK, pro, posi, noremove);
    
    wrkRun(SEND_LOGBOOK, pro, posi, noremove);
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
 *  Треки: Отправка списка треков (новый формат пересылки треков)
 * ------------------------------------------------------------------------------------------- */
WRK_DEFINE(SEND_TRACKLIST) {
    private:
        uint8_t m_fn;
        bool m_isok;
        bool m_useext;
        
        BinProto *m_pro;
    
    public:
        bool isok() const { return m_isok; }
    
    WRK_CLASS(SEND_TRACKLIST)(BinProto *pro, bool noremove = false) :
        m_isok(false),
        m_fn(0),
        m_pro(pro)
    {
        if (noremove)
            optset(O_NOREMOVE);
    }
    
    WRK_PROCESS
        if (m_pro == NULL)
            WRK_RETURN_ERR("pro is NULL");
#ifdef USE_SDCARD
        m_useext = fileExtInit();
#else
        m_useext = false;
#endif
        CONSOLE("useext: %d", m_useext);
        SEND(0x51);
    
    WRK_BREAK_RUN
        m_fn++;
        FileTrack tr;
        if ((m_fn > 99) || !tr.exists(m_fn)) {
            CONSOLE("files: %d, terminate", m_fn-1);
            m_isok = true;
            WRK_RETURN_END;
        }

        if (!tr.open(m_fn))
            WRK_RETURN_ERR("Can't open num: %d", m_fn);
        
        FileTrack::head_t th;
        if (!tr.get(th))
            WRK_RETURN_ERR("Can't get head num: %d", m_fn);

        struct __attribute__((__packed__)) {
            uint32_t id;
            uint32_t flags;
            uint32_t jmpnum;
            uint32_t jmpkey;
            tm_t     tmbeg;
            uint32_t fsize;
            uint8_t  fnum;
        } h = {
            .id         = th.id,
            .flags      = th.flags,
            .jmpnum     = th.jmpnum,
            .jmpkey     = th.jmpkey,
            .tmbeg      = th.tmbeg,
            .fsize      = tr.sizebin(),
            .fnum       = m_fn
        };
        SEND(0x52, "NNNNTNC", h);
        tr.close();
        
        WRK_RETURN_RUN;
    WRK_END
        
    void end() {
        if (m_pro)
            m_pro->send(0x53);
        
        if (m_useext)
            fileExtStop();
    }
};

WrkProc::key_t sendTrackList(BinProto *pro, bool noremove) {
    if (pro == NULL)
        return WRKKEY_NONE;
    
    if (!noremove)
        return wrkRand(SEND_TRACKLIST, pro, noremove);
    
    wrkRun(SEND_TRACKLIST, pro, noremove);
    return WRKKEY_SEND_TRACKLIST;
}

bool isokTrackList(const WrkProc *_wrk) {
    const auto wrk = 
        _wrk == NULL ?
            wrkGet(SEND_TRACKLIST) :
            reinterpret_cast<const WRK_CLASS(SEND_TRACKLIST) *>(_wrk);
    
    return (wrk != NULL) && (wrk->isok());
}


WRK_DEFINE(SEND_TRACK) {
    private:
        bool m_isok;
        bool m_useext;
        trksrch_t m_srch;
        bool m_sndbeg;
        FileTrack m_tr;
        
        BinProto *m_pro;
    
    public:
        uint32_t    m_sz, m_snd;
        bool isok() const { return m_isok; }
    
    WRK_CLASS(SEND_TRACK)(BinProto *pro, const trksrch_t &srch, bool noremove = false) :
        m_srch(srch),
        m_isok(false),
        m_sndbeg(false),
        m_pro(pro),
        m_sz(0),
        m_snd(0)
    {
        CONSOLE("Requested fnum: %d", m_srch.fnum);
        if (noremove)
            optset(O_NOREMOVE);
    }
    
    WRK_PROCESS
        if (m_pro == NULL)
            WRK_RETURN_ERR("pro is NULL");
#ifdef USE_SDCARD
        m_useext = fileExtInit();
#else
        m_useext = false;
#endif
        CONSOLE("useext: %d", m_useext);
        
        // Сначала попытаемся найти нужный трек.
        // Начнём с номера, присланного в srch
        if ((m_srch.fnum < 1) || (m_srch.fnum > 99))
            WRK_RETURN_ERR("fnum not valid: %d", m_srch.fnum);
    
    WRK_BREAK_RUN
        if (!m_tr.open(m_srch.fnum))
            WRK_RETURN_ERR("Can't open track fnum=%d", m_srch.fnum);
        
        FileTrack::head_t th;
        if (!m_tr.get(th))
            WRK_RETURN_ERR("Can't get head num: %d", m_srch.fnum);
        
        if ((th.id      != m_srch.id) ||
            (th.jmpnum  != m_srch.jmpnum) ||
            (th.jmpkey  != m_srch.jmpkey) ||
            (th.tmbeg   != m_srch.tmbeg)) {
            CONSOLE("Track fnum=%d, header not equal", m_srch.fnum);
            m_tr.close();
            // Возможно, файл уже перенумерован дальше, надо тоже проверить
            m_srch.fnum++;
            if (m_srch.fnum > 99)
                WRK_RETURN_ERR("track not found");
            
            WRK_RETURN_RUN;
        }
        
        m_sz = m_tr.sizebin();
    
    // WRK_BREAK_RUN тут нельзя прерывать, т.к. теряется область видимости для th
        struct __attribute__((__packed__)) {
            uint32_t id;
            uint32_t flags;
            uint32_t jmpnum;
            uint32_t jmpkey;
            tm_t     tmbeg;
            uint32_t fsize;
            FileTrack::chs_t    chksum;
        } h = {
            .id         = th.id,
            .flags      = th.flags,
            .jmpnum     = th.jmpnum,
            .jmpkey     = th.jmpkey,
            .tmbeg      = th.tmbeg,
            .fsize      = m_tr.sizebin(),
            .chksum     = m_tr.chksum()
        };
        SEND(0x54, "NNNNTNH", h);
        m_sndbeg = true;
    
    WRK_BREAK_RUN
        if (m_tr.available() >= m_tr.sizeitem()) {
            FileTrack::item_t ti;
            if (!m_tr.get(ti))
                CONSOLE("Can't get track data fnum=%d", m_srch.fnum);
            SEND(0x55, LOG_PK, ti);
            
            m_snd += m_tr.sizeitem();
            
            WRK_RETURN_RUN;
        }
        
        m_isok = true;
    WRK_END
        
    void end() {
        m_tr.close();
        if (m_pro && m_sndbeg)
            m_pro->send(0x56);
        
        if (m_useext)
            fileExtStop();
    }
};

WrkProc::key_t sendTrack(BinProto *pro, const trksrch_t &srch, bool noremove) {
    if (pro == NULL)
        return WRKKEY_NONE;
    
    if (!noremove)
        return wrkRand(SEND_TRACK, pro, srch, noremove);
    
    wrkRun(SEND_TRACK, pro, srch, noremove);
    return WRKKEY_SEND_TRACK;
}

bool isokTrack(const WrkProc *_wrk) {
    const auto wrk = 
        _wrk == NULL ?
            wrkGet(SEND_TRACK) :
            reinterpret_cast<const WRK_CLASS(SEND_TRACK) *>(_wrk);
    
    return (wrk != NULL) && (wrk->isok());
}

cmpl_t cmplTrack(const WrkProc *_wrk) {
    const auto wrk = 
        _wrk == NULL ?
            wrkGet(SEND_TRACK) :
            reinterpret_cast<const WRK_CLASS(SEND_TRACK) *>(_wrk);
    
    if (wrk == NULL)
        return { 0, 0 };
    return { wrk->m_snd, wrk->m_sz };
}

/* =========================================================================================== */
/* =========================================================================================== */
/* =========================================================================================== */



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
        
        struct __attribute__((__packed__)) {
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
                struct __attribute__((__packed__)) {
                    uint16_t    sz;
                    uint8_t     buf[1000];
                } d;
                RECV("nB1000", d);
                
                auto sz = Update.write(d.buf, d.sz);
                if ((sz == 0) || (sz != d.sz))
                    WRK_RETURN_ERR("Burn sz wrong: %d <-> %u", sz, d.sz);
                
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

/* ------------------------------------------------------------------------------------------- *
 *  Взаимодействие с приложением
 * ------------------------------------------------------------------------------------------- */
WRK_DEFINE(NET_APP) {
    private:
        uint16_t m_timeout;
        uint16_t m_code;
        
        BinProto *m_pro;
    
    public:
        state_t chktimeout() {
            if (m_timeout == 0)
                WRK_RETURN_WAIT;
            
            m_timeout--;
            if (m_timeout > 0)
                WRK_RETURN_WAIT;
            
            WRK_RETURN_ERR("Wait timeout");
        }
        
        state_t every() {
            m_pro->rcvprocess();
            switch (m_pro->rcvstate()) {
                case BinProto::RCV_ERROR:
                    WRK_RETURN_ERR("recv proto fail");
                case BinProto::RCV_DISCONNECTED:
                    WRK_RETURN_ERR("disconnected");
            }
        
            WRK_RETURN_RUN;
        }
    
    WRK_CLASS(NET_APP)(NetSocket *sock, bool noremove = false) :
        m_timeout(0),
        m_code(0),
        m_pro(new BinProto(sock))
    {
        if (noremove)
            optset(O_NOREMOVE);
    }
    
    WRK_PROCESS
        
        m_timeout = 200;
    WRK_BREAK_RECV
        // hello/init
        if (m_pro->rcvcmd() != 0x02)
            WRK_RETURN_ERR("Recv wrong cmd=0x%02x begin", m_pro->rcvcmd());
        RECVNEXT();  // Эта команда без данных,
        SEND(0x02); // init ok
        m_code = static_cast<uint16_t>(esp_random());
        CONSOLE("Auth code: 0x%04x", m_code);
        setViewNetAuth(m_code);
        
        m_timeout = 0;
    WRK_BREAK_RUN
        // wait auth code
        if (!viewIsNetAuth())
            WRK_RETURN_ERR("Recv auth fail: canceled");
        
        CHK_RECV
        // auth
        if (m_pro->rcvcmd() != 0x03)
            WRK_RETURN_ERR("Recv wrong cmd=0x%02x begin", m_pro->rcvcmd());
        uint16_t code = 0;
        RECV("n", code);
        uint8_t err = code == m_code ? 0 : 1;
        SEND(0x03, "C", err);
        if (code != m_code)
            WRK_RETURN_ERR("Recv auth fail: wrong code 0x%04x != 0x%04x", code, m_code);
        CONSOLE("Recv code: 0x%04x = auth ok", code);
        m_code = 0;
        setViewMain();
        
        m_timeout = 0;
    WRK_BREAK_RECV
        
        switch (m_pro->rcvcmd()) {
            case 0x31: { // logbook
                posi_t posi = { 10, 10 };
                RECV("NN", posi);
                sendLogBook(m_pro, posi);
                break;
            }
            case 0x51: { // trklist
                m_pro->rcvnext(); // нет данных
                sendTrackList(m_pro);
                break;
            }
            case 0x54: { // track request
                trksrch_t srch = { 0 };
                RECV("NNNTC", srch);

                sendTrack(m_pro, srch);
                break;
            }
        }
        
        WRK_RETURN_WAIT;
    WRK_END
    
    void end() {
        if (m_pro != NULL) {
            auto sock = m_pro->sock();
            if (sock != NULL) {
                sock->close();
                delete sock;
            }
            delete m_pro;
            m_pro = NULL;
        }
        if ((m_code > 0) && viewIsNetAuth())
            setViewMain();
    }
};

WrkProc::key_t netApp(NetSocket *sock, bool noremove) {
    if (sock == NULL)
        return WRKKEY_NONE;
    
    if (!noremove)
        return wrkRand(NET_APP, sock, noremove);
    
    wrkRun(NET_APP, sock, noremove);
    return WRKKEY_NET_APP;
}
