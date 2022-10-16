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

#define RETURN_ERR              WRK_RETURN_END
#define SEND(cmd, ...)          if (!m_pro || !m_pro->send(cmd, ##__VA_ARGS__)) RETURN_ERR

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
            RETURN_ERR;
    
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
                RETURN_ERR;
            if (!m_lb.seekto(m_pos))
                RETURN_ERR;
        }
        else
            m_fn = m_lb.count();
        
        if (m_fn < 0)       // ошибка поиска
            RETURN_ERR;
        
    WRK_BREAK_RUN
        SEND(0x31);
        m_pos = 0;
        m_begsnd = true;
        
    WRK_BREAK_RUN
        if (m_fn > 0) {
            if (!m_lb && !m_lb.open(m_fn))
                RETURN_ERR;
            CONSOLE("logbook num=%d, pos=%d, avail=%d", m_fn, m_lb.pos(), m_lb.avail());
            if (m_lb.avail() > 0) {
                FileLogBook::item_t jmp;
                if (!m_lb.get(jmp))
                    RETURN_ERR;
                
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
    
    return pro->send( 0x3f, "XXCCCaC   a32", d ); // datafin
}
