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


/* ------------------------------------------------------------------------------------------- *
 *  Команды протокола передачи
 *  
 *  Обрабатывает прибор:
 *  0x02 - [приложение] принимает hello от клиента, может содержать autokey, приложение отвечает 0x02
 *  0x03 - [приложение] авторизация приложения в приборе, должна содержать authnum
 *  0x0a - [приложение] запрос данных приложением, содержит код требуемых данных
 *  0x10 - [wifisync стадия подтверждения привязки] - требуется rejoin, нет привязки, присылает код на экран
 *  0x13 - [wifisync стадия ожидания rejoin] - сервер присылает новый authid (uid и secnum)
 *  0x20 - [wifisync стадия подтверждения привязки] - прибор опознан сервером, можно начинать передачу,
 *          тут сервер передаёт всякие контрольные суммы и позиции с предыдущей синхнонизации
 *  0x41 - сохранение в приборе: recvWiFiPass(начало списка)
 *  0x42 - сохранение в приборе: recvWiFiPass(данные о сети)
 *  0x43 - сохранение в приборе: recvWiFiPass(конец списка)
 *  0x44 - сохранение в приборе: recvVerAvail(начало списка)
 *  0x45 - сохранение в приборе: recvVerAvail(данные о версии)
 *  0x46 - сохранение в приборе: recvVerAvail(конец списка)
 *  0x47 - сохранение в приборе: recvFirmware(инициализация передачи)
 *  0x48 - сохранение в приборе: recvFirmware(размер прошивки и md5-сумма)
 *  0x49 - сохранение в приборе: recvFirmware(блок данных)
 *  0x4a - сохранение в приборе: recvFirmware(окончание данных)
 *  0x5a - сохранение в приборе: recvFiles(начало списка)
 *  0x5b - сохранение в приборе: recvFiles(начало файла)
 *  0x5c - сохранение в приборе: recvFiles(блок данных файла)
 *  0x5d - сохранение в приборе: recvFiles(конец файла)
 *  0x5e - сохранение в приборе: recvFiles(конец списка)
 *  
 *  Обрабатывает приложение или сервер:
 *  0x01 - [wifisync] передача на сервер authid, по которому привязывается прибор к аккаунту
 *          В ответ сервер отправит прибору: 0x10 или 0x20
 *  0x02 - [приложение] ответ прибора на hello, если требуется авторизация
 *  0x03 - [приложение] прибор отвечает на авторизацию, содержит код ошибки или 0 и autokey для следующей авторизации
 *  0x10 - [приложение] прибор подтверждает завершение приёма данных, содержит код данных и статус
 *  0x12 - [wifisync] idle - прибор отправляет периодически на сервер,
 *          пока ожидается привязка к аккаунту, чтобы не сработал таймаут подключения на сервере
 *  0x21 - прибор отправляет: sendCfgMain
 *  0x22 - прибор отправляет: sendJmpCount
 *  0x23 - прибор отправляет: sendPoint(конец списка), содержит контрольную сумму
 *  0x24 - прибор отправляет: sendPoint(каждая точка)
 *  0x31 - прибор отправляет: sendLogBook(начало списка), содержит позицию начала передачи и ожидаемое количестко
 *  0x32 - прибор отправляет: sendLogBook(инфо о прыге)
 *  0x33 - прибор отправляет: sendLogBook(конец списка), содержит контрольную сумму и позицию
 *  0x3f - [wifisync] Завершение передачи на сервер, содержит текущие контрольные суммы всех файлов
 *  0x37 - прибор отправляет: sendWiFiPass(начало списка), содержит размер файла с паролями
 *  0x38 - прибор отправляет: sendWiFiPass(данные о сети), содержит текущую позицию в файле паролей
 *  0x39 - прибор отправляет: sendWiFiPass(конец списка)
 *  0x4a - [wifisync] прибор подтверждает завершения приёма WiFiPass, содержит контрольную сумму
 *  0x4b - [wifisync] прибор подтверждает завершения приёма WiFiPass, содержит контрольную сумму
 *  0x4c - [wifisync] прибор подтверждает завершения приёма WiFiPass
 *  0x51 - прибор отправляет: sendTrackList(начало списка), тут трудно узнать кол-во, поэтому оно не передаётся
 *  0x52 - прибор отправляет: sendTrackList(данные о треке)
 *  0x53 - прибор отправляет: sendTrackList(конец списка)
 *  0x5a - прибор отправляет: sendFiles(начало списка)
 *  0x5b - прибор отправляет: sendFiles(начало файла)
 *  0x5c - прибор отправляет: sendFiles(блок данных файла)
 *  0x5d - прибор отправляет: sendFiles(конец файла)
 *  0x5e - прибор отправляет: sendFiles(конец списка)
 * ------------------------------------------------------------------------------------------- */

#define WPRC_ERR(s, ...)    do { CONSOLE(s, ##__VA_ARGS__); return END; } while (0)
#define SND(cmd, ...)       if (!m_pro || !m_pro->send(cmd, ##__VA_ARGS__)) WPRC_ERR("send fail")
#define RCV(pk, ...)        if (!m_pro || !m_pro->rcvdata(PSTR(pk), ##__VA_ARGS__)) WPRC_ERR("recv data fail")
#define RCVRAW(data)        m_pro ? m_pro->rcvraw(data, sizeof(data)) : -1
#define RCVNEXT()           if (!m_pro || !m_pro->rcvnext()) WPRC_ERR("recv data fail")
#define CHK_RCV             if (!m_pro || !m_pro->rcvvalid()) WPRC_ERR("recv not valid"); \
                            if (m_pro->rcvstate() < BinProto::RCV_COMPLETE) return chktimeout();
#define WPRC_RCV(timeout)   m_timeout = timeout; WPRC_BREAK CHK_RCV

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
class _sendLogBook : public WrkNet {
        int m_fn;
        bool m_begsnd;
        FileLogBook m_lb;
        
        uint32_t m_cks, m_beg, m_cnt, m_pos;
        enum {
            byCks,
            byBeg
        } m_meth;
    
public:
    _sendLogBook(BinProto *pro, uint32_t cks, uint32_t pos) :
        WrkNet(pro, netSendLogBook),
        m_begsnd(false),
        m_cks(cks),
        m_beg(pos),
        m_cnt(0),
        m_pos(0),
        m_meth(byCks)
    {
    }
    
    _sendLogBook(BinProto *pro, const posi_t &posi) :
        // struct в аргументах, чтобы вычленить нужный метод по аргументам, иначе они такие же
        WrkNet(pro, netSendLogBook),
        m_fn(0),
        m_begsnd(false),
        m_cks(0),
        m_beg(posi.beg),
        m_cnt(posi.count),
        m_pos(0),
        m_meth(byBeg)
    {
    }
#ifdef FWVER_DEBUG
    ~_sendLogBook() {
        CONSOLE("wrk(0x%08x) destroy", this);
    }
#endif
    
    state_t run() {
    WPROC
        CONSOLE("sendLogBook: chksum: %08x, beg: %lu, count: %lu", m_cks, m_beg, m_cnt);
        if (!isnetok())
            WPRC_ERR("pro is NULL");
    
    WPRC_RUN
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
                    WPRC_ERR("Can't open logbook num=%d", m_fn);
                if (!m_lb.seekto(m_beg))
                    WPRC_ERR("Can't logbook num=%d seek to pos=%lu", m_fn, m_beg);
            }
            else
                m_fn = m_lb.count();
        
            if (m_fn < 0)       // ошибка поиска
                WPRC_ERR("Fail find file by cks=%08x", m_cks);
        }
        else
        if (m_meth == byBeg) {
            if ((m_fn < 99) && (m_lb.exists(m_fn + 1))) {
                m_fn++;
                if (m_lb)
                    m_lb.close();
                if (!m_lb.open(m_fn))
                    WPRC_ERR("Can't open logbook num=%d", m_fn);
                auto cnt = m_lb.sizefile();
                if (cnt < 1)
                    WPRC_ERR("Empty logbook file num=%d", m_fn);
                CONSOLE("logbook file num=%d; all rec count=%d", m_fn, cnt);
                m_pos += cnt;
                if (m_pos < m_beg) // ещё не нашли нужной позиции, пойдём к следующему файлу
                     return RUN;
                m_beg = m_pos-m_beg;
                if ((m_beg > 0) && !m_lb.seekto(m_beg))
                    WPRC_ERR("Can't logbook num=%d seek to pos=%lu", m_fn, m_beg);
                m_pos -= m_beg;
            }
            else
                m_beg = 0;
            
            CONSOLE("Begin send from num=%d; start by pos: %lu", m_fn, m_beg);
        }
        else
            WPRC_ERR("Unknown meth: %d", m_meth);
        
    WPRC_RUN
        struct {
            uint32_t beg, count;
        } d = { m_pos, m_cnt };
        SND(0x31, "NN", d);
        m_begsnd = true;
        
        if (m_fn == 0)
            m_isok = true;
        m_cmpl.sz = m_cnt;
        
    WPRC_RUN
        if (m_fn > 0) {
            if (!m_lb && !m_lb.open(m_fn))
                WPRC_ERR("Can't open logbook num=%d", m_fn);
            CONSOLE("logbook num=%d, pos=%d, avail=%d", m_fn, m_lb.pos(), m_lb.avail());
            if (m_lb.avail() > 0) {
                FileLogBook::item_t jmp;
                if (!m_lb.get(jmp))
                    WPRC_ERR("Can't get item jump");
                
                SND(0x32, "NNT" LOG_PK LOG_PK LOG_PK LOG_PK, jmp);
                m_cmpl.val++;
                if (m_cnt > 0) {
                    m_cnt--;
                    if (m_cnt == 0) {
                        // отправили нужное количество
                        CONSOLE("logbook need count finished");
                        return ok();
                    }
                }
            }
            
            if (m_lb.avail() <= 0) {
                if (m_fn > 1) {
                    m_lb.close();
                    CONSOLE("logbook closed num=%d", m_fn);
                }
                else {// Завершился процесс
                    CONSOLE("logbook finished");
                    return ok();
                }
                m_fn--;
            }
            return RUN;
        }
    WPRC(END)
    }
        
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

WrkProc<WrkNet> sendLogBook(BinProto *pro, uint32_t cks, uint32_t pos) {
    return wrkRun<_sendLogBook, WrkNet>(pro, cks, pos);
}
WrkProc<WrkNet> sendLogBook(BinProto *pro, const posi_t &posi) {
    return wrkRun<_sendLogBook, WrkNet>(pro, posi);
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
 *  Треки: Отправка wifi-паролей
 * ------------------------------------------------------------------------------------------- */
class _sendWiFiPass : public WrkNet {
        FileTxt fh;
        const char *m_fname;
    
public:
    _sendWiFiPass(BinProto *pro) :
        WrkNet(pro, netSendWiFiPass),
        m_fname(PSTR(WIFIPASS_FILE))
    {
    }
#ifdef FWVER_DEBUG
    ~_sendWiFiPass() {
        CONSOLE("wrk(0x%08x) destroy", this);
    }
#endif
    
    state_t run() {
    WPROC
        if (!isnetok())
            WPRC_ERR("pro is NULL");
    
    WPRC_RUN
        if (fileExists(m_fname)) {
            if (!fh.open_P(m_fname))
                WPRC_ERR("Can't open wifi-file for read");
            m_cmpl.sz = fh.size();
        }
        
        SND(0x37, "N", m_cmpl.sz);
        
        if (!fh)
            return ok();
    
    WPRC_RUN
        if (!fh)
            WPRC_ERR("FileHandle closed");
        if (!fh.find_param(PSTR("ssid")))
            return ok();
        
        struct __attribute__((__packed__)) {
            char ssid[BINPROTO_STRSZ];
            char pass[BINPROTO_STRSZ];
            uint32_t pos;
        } d;
        if (!fh.read_line(d.ssid, sizeof(d.ssid)))
            WPRC_ERR("Can't wifi-file read");
        
        char param[30];
        if (!fh.read_param(param, sizeof(param)))
            WPRC_ERR("Can't wifi-file read");
        if (strcmp_P(param, PSTR("pass")) == 0)
            fh.read_line(d.pass, sizeof(d.pass));
        else
            d.pass[0] = '\0';
        
        d.pos = fh.position();
        
        SND(0x38, "ssN", d);
        m_cmpl.val = fh.position();
    WPRC(RUN)
    }
        
    void end() {
        if (fh)
            fh.close();
        if (m_pro)
            m_pro->send(0x39);
    }
};

WrkProc<WrkNet> sendWiFiPass(BinProto *pro) {
    return wrkRun<_sendWiFiPass, WrkNet>(pro);
}


/* ------------------------------------------------------------------------------------------- *
 *  Треки: Отправка списка треков (новый формат пересылки треков)
 * ------------------------------------------------------------------------------------------- */
class _sendTrackList : public WrkNet {
        uint8_t m_fn;
        bool m_useext;
    
public:
    _sendTrackList(BinProto *pro) :
        WrkNet(pro, netSendTrackList),
        m_fn(0)
    {
    }
#ifdef FWVER_DEBUG
    ~_sendTrackList() {
        CONSOLE("wrk(0x%08x) destroy", this);
    }
#endif
    
    state_t run() {
    WPROC
        if (!isnetok())
            WPRC_ERR("pro is NULL");
    
    WPRC_RUN
#ifdef USE_SDCARD
        m_useext = fileExtInit();
#else
        m_useext = false;
#endif
        CONSOLE("useext: %d", m_useext);
        SND(0x51);
    
    WPRC_RUN
        m_fn++;
        FileTrack tr;
        if ((m_fn > 99) || !tr.exists(m_fn)) {
            CONSOLE("files: %d, terminate", m_fn-1);
            return ok();
        }

        if (!tr.open(m_fn))
            WPRC_ERR("Can't open num: %d", m_fn);
        
        FileTrack::head_t th;
        if (!tr.get(th))
            WPRC_ERR("Can't get head num: %d", m_fn);

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
        SND(0x52, "NNNNTNC", h);
        tr.close();
    WPRC(RUN)
    }
        
    void end() {
        if (m_pro)
            m_pro->send(0x53);
        
        if (m_useext)
            fileExtStop();
    }
};

WrkProc<WrkNet> sendTrackList(BinProto *pro) {
    return wrkRun<_sendTrackList, WrkNet>(pro);
}


class _sendTrack : public WrkNet {
        bool m_useext;
        trksrch_t m_srch;
        bool m_sndbeg;
        FileTrack m_tr;
    
public:
    _sendTrack(BinProto *pro, const trksrch_t &srch) :
        WrkNet(pro, netSendTrack),
        m_srch(srch),
        m_sndbeg(false)
    {
        CONSOLE("Requested fnum: %d", m_srch.fnum);
    }
#ifdef FWVER_DEBUG
    ~_sendTrack() {
        CONSOLE("wrk(0x%08x) destroy", this);
    }
#endif
    
    state_t run() {
    WPROC
        if (!isnetok())
            WPRC_ERR("pro is NULL");
    
    WPRC_RUN
#ifdef USE_SDCARD
        m_useext = fileExtInit();
#else
        m_useext = false;
#endif
        CONSOLE("useext: %d", m_useext);
        
        // Сначала попытаемся найти нужный трек.
        // Начнём с номера, присланного в srch
        if ((m_srch.fnum < 1) || (m_srch.fnum > 99))
            WPRC_ERR("fnum not valid: %d", m_srch.fnum);
    
    WPRC_RUN
        if (!m_tr.open(m_srch.fnum))
            WPRC_ERR("Can't open track fnum=%d", m_srch.fnum);
        
        FileTrack::head_t th;
        if (!m_tr.get(th))
            WPRC_ERR("Can't get head num: %d", m_srch.fnum);
        
        if ((th.id      != m_srch.id) ||
            (th.jmpnum  != m_srch.jmpnum) ||
            (th.jmpkey  != m_srch.jmpkey) ||
            (th.tmbeg   != m_srch.tmbeg)) {
            CONSOLE("Track fnum=%d, header not equal", m_srch.fnum);
            m_tr.close();
            // Возможно, файл уже перенумерован дальше, надо тоже проверить
            m_srch.fnum++;
            if (m_srch.fnum > 99)
                WPRC_ERR("track not found");
            
            return RUN;
        }
        
        m_cmpl.sz = m_tr.sizebin();
    
    // WPRC_RUN тут нельзя прерывать, т.к. теряется область видимости для th
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
        SND(0x54, "NNNNTNH", h);
        m_sndbeg = true;
    
    WPRC_RUN
        if (m_tr.available() < m_tr.sizeitem())
            return ok();
        
        FileTrack::item_t ti;
        if (!m_tr.get(ti))
            CONSOLE("Can't get track data fnum=%d", m_srch.fnum);
        SND(0x55, LOG_PK, ti);
        
        m_cmpl.val += m_tr.sizeitem();
    WPRC(RUN)
    }
        
    void end() {
        m_tr.close();
        if (m_pro && m_sndbeg)
            m_pro->send(0x56);
        
        if (m_useext)
            fileExtStop();
    }
};

WrkProc<WrkNet> sendTrack(BinProto *pro, const trksrch_t &srch) {
    return wrkRun<_sendTrack, WrkNet>(pro, srch);
}


/* ------------------------------------------------------------------------------------------- *
 *  Отправка всех файлов на SPIFFS
 * ------------------------------------------------------------------------------------------- */
class _sendFiles : public WrkNet {
    File m_dh;
    File m_fh;
    uint16_t m_cnt;
public:
    _sendFiles(BinProto *pro) :
        WrkNet(pro, netSendFiles),
        m_cnt(0)
    {
    }
#ifdef FWVER_DEBUG
    ~_sendFiles() {
        CONSOLE("wrk(0x%08x) destroy", this);
    }
#endif
    
    state_t run() {
    WPROC
        if (!isnetok())
            WPRC_ERR("pro is NULL");
        
        m_dh = dirIntOpen();
        if (!m_dh)
            WPRC_ERR("Can't open root of SPIFFS");
        if (!m_dh.isDirectory())
            WPRC_ERR("SPIFFS[\"%s\"] is not directory", m_dh.name());
        
    WPRC_RUN
        File file = m_dh.openNextFile();
        if (file) {
            if (!file.isDirectory()) {
                CONSOLE("file[%s]\t= %d bytes", file.name(), file.size());
                m_cmpl.sz += file.size();
                m_cnt ++;
            }
            return RUN;
        }
        CONSOLE("size: %d, count: %d", m_cmpl.sz, m_cnt);

    WPRC_RUN
        m_dh = dirIntOpen();
        struct __attribute__((__packed__)) {
            uint16_t cnt;
            uint32_t sz;
        } h = { m_cnt, m_cmpl.sz };
        SND(0x5a, "nN", h);

    WPRC_RUN
        if (!m_fh) {
            m_fh = m_dh.openNextFile();
            if (!m_fh) {
                return ok();
            }
            if (m_fh.isDirectory()) {
                m_fh.close();
                return RUN;
            }

            struct __attribute__((__packed__)) {
                uint32_t sz;
                char name[BINPROTO_STRSZ];
            } h = { m_fh.size() };
            strncpy(h.name, m_fh.name(), sizeof(h.name));
            SND(0x5b, "Ns", h);
            CONSOLE("file beg: %s (%d bytes)", h.name, h.sz);
            
            return RUN;
        }

        if (m_fh.available() <= 0) {
            SND(0x5d);
            CONSOLE("file end");
            m_fh.close();
            return RUN;
        }

        struct __attribute__((__packed__)) {
            uint16_t sz;
            uint8_t buf[256];
        } d;
        size_t sz = m_fh.read(d.buf, sizeof(d.buf));
        if (sz <= 0)
            WPRC_ERR("file(%s) read err on pos: %d", m_fh.name(), m_fh.position());
        d.sz = sz;
        SND(0x5c, "B256", d);
        CONSOLE("file data: %d bytes", d.sz);

    WPRC(RUN)
    }
        
    void end() {
        if (m_pro)
            m_pro->send(0x5e);
        CONSOLE("file finish");
    }
};


/* =========================================================================================== */
/* =========================================================================================== */
/* =========================================================================================== */


class WrkRecv : public WrkNet {
    protected:
        uint16_t m_timeout;
        state_t chktimeout() {
            if (m_timeout == 1)
                WPRC_ERR("Wait timeout");
            
            return DLY;
        }
    public:
        WrkRecv(BinProto *_pro, uint16_t _id = 0) :
            WrkNet(_pro, _id),
            m_timeout(0)
        {}
        void timer() {
            if (m_timeout > 1)
                m_timeout--;
        }
};

/* ------------------------------------------------------------------------------------------- *
 *  Приём wifi-паролей
 * ------------------------------------------------------------------------------------------- */
class _recvWiFiPass : public WrkRecv {
        FileTxt fh;
        const char *m_fname;
    
public:
    _recvWiFiPass(BinProto *pro) :
        WrkRecv(pro, netRecvWiFiPass),
        m_fname(PSTR(WIFIPASS_FILE))
    {
    }
#ifdef FWVER_DEBUG
    ~_recvWiFiPass() {
        CONSOLE("wrk(0x%08x) destroy", this);
    }
#endif
    
    state_t run() {
        m_pro->rcvprocess();
        if (!m_pro->rcvvalid())
            WPRC_ERR("recv data fail");
    
    WPROC
        if ( fileExists(m_fname) &&
            !fileRemove(m_fname))
            WPRC_ERR("Can't remove prev wifi-file");
    
    WPRC_RUN
        if (!fh.open_P(m_fname, FileMy::MODE_APPEND))
            WPRC_ERR("Can't open wifi-file for write");
        
    WPRC_RCV(200)
        if (m_pro->rcvcmd() != 0x41)
            WPRC_ERR("Recv wrong cmd=0x%02x", m_pro->rcvcmd());
        RCVNEXT();  // Эта команда без данных,
                            // нам надо перейти к приёму следующей команды
    
    WPRC_RCV(200)
        switch (m_pro->rcvcmd()) {
            case 0x42: { // wifi net
                struct __attribute__((__packed__)) {
                    char ssid[BINPROTO_STRSZ];
                    char pass[BINPROTO_STRSZ];
                } d;
                RCV("ss", d);
                
                CONSOLE("add wifi: {%s}, {%s}", d.ssid, d.pass);
                if (
                        !fh.print_param(PSTR("ssid"), d.ssid) ||
                        !fh.print_param(PSTR("pass"), d.pass)
                    )
                    WPRC_ERR("Fail write ssid/pass");
                m_timeout = 200;
                return RUN;
            }

            case 0x43: { // wifi end
                RCVNEXT();  // Эта команда без данных
                
                fh.close(); // надо переоткрыть файл, иначе из него нельзя прочитать
                if (!fh.open_P(m_fname))
                    WPRC_ERR("Can't open wifi-file for chksum");
                uint32_t cks = fh.chksum();
                fh.close();
                
                SND(0x4a, "N", cks); // wifiok
                m_isok = true;
                break;
            }

            default: WPRC_ERR("Recv unknown cmd=0x%02x", m_pro->rcvcmd());
        }
        
    WPRC(END)
    }
};

WrkProc<WrkNet> recvWiFiPass(BinProto *pro) {
    return wrkRun<_recvWiFiPass, WrkNet>(pro);
}

/* ------------------------------------------------------------------------------------------- *
 *  Приём veravail - доступных версий прошивки
 * ------------------------------------------------------------------------------------------- */
class _recvVerAvail : public WrkRecv {
        FileTxt fh;
        const char *m_fname;
    
public:
    _recvVerAvail(BinProto *pro) :
        WrkRecv(pro, netRecvVerAvail),
        m_fname(PSTR(VERAVAIL_FILE))
    {
    }
#ifdef FWVER_DEBUG
    ~_recvVerAvail() {
        CONSOLE("wrk(0x%08x) destroy", this);
    }
#endif
    
    state_t run() {
        m_pro->rcvprocess();
        if (!m_pro->rcvvalid())
            WPRC_ERR("recv data fail");
    
    WPROC
        if ( fileExists(m_fname) &&
            !fileRemove(m_fname))
            WPRC_ERR("Can't remove prev veravail-file");
    
    WPRC_RUN
        if (!fh.open_P(m_fname, FileMy::MODE_APPEND))
            WPRC_ERR("Can't open veravail-file for write");
        
    WPRC_RCV(200)
        if (m_pro->rcvcmd() != 0x44)
            WPRC_ERR("Recv wrong cmd=0x%02x", m_pro->rcvcmd());
        RCVNEXT();  // Эта команда без данных,
                            // нам надо перейти к приёму следующей команды
    
    WPRC_RCV(200)
        switch (m_pro->rcvcmd()) {
            case 0x45: { // veravail item
                char ver[BINPROTO_STRSZ];
                // тут нужен вызов через два аргумента (ссылка на данные и sizeof()),
                // чтобы верно передалась ссылка и размер выделенной памяти
                RCV("s", ver, sizeof(ver));
                
                CONSOLE("add veravail: {%s}", ver);
                if ( !fh.print_line(ver) )
                    WPRC_ERR("Fail write version");
                m_timeout = 200;
                return RUN;
            }

            case 0x46: { // veravail end
                RCVNEXT();  // Эта команда без данных
                
                fh.close(); // надо переоткрыть файл, иначе из него нельзя прочитать
                if (!fh.open_P(m_fname))
                    WPRC_ERR("Can't open veravail-file for chksum");
                uint32_t cks = fh.chksum();
                fh.close();
                
                SND(0x4b, "N", cks); // veravail ok
                m_isok = true;
                break;
            }

            default: WPRC_ERR("Recv unknown cmd=0x%02x", m_pro->rcvcmd());
        }
        
    WPRC(END)
    }
};

WrkProc<WrkNet> recvVerAvail(BinProto *pro) {
    return wrkRun<_recvVerAvail, WrkNet>(pro);
}

/* ------------------------------------------------------------------------------------------- *
 *  Обновление прошивки по сети
 * ------------------------------------------------------------------------------------------- */
class _recvFirmware : public WrkRecv {
public:
    _recvFirmware(BinProto *pro) :
        WrkRecv(pro, netRecvFirmware)
    {
    }
#ifdef FWVER_DEBUG
    ~_recvFirmware() {
        CONSOLE("wrk(0x%08x) destroy", this);
    }
#endif
    
    state_t run() {
        m_pro->rcvprocess();
        if (!m_pro->rcvvalid())
            WPRC_ERR("recv data fail");
    
    WPROC
    WPRC_RCV(200)
        if (m_pro->rcvcmd() != 0x47)
            WPRC_ERR("Recv wrong cmd=0x%02x begin", m_pro->rcvcmd());
        RCVNEXT();  // Эта команда без данных,
        
    WPRC_RCV(200)
        if (m_pro->rcvcmd() != 0x48)
            WPRC_ERR("Recv wrong cmd=0x%02x info", m_pro->rcvcmd());
        
        struct __attribute__((__packed__)) {
            uint32_t    size;
            char        md5[37];
        } info;
        RCV("Na36", info);
        CONSOLE("recv fw info: size: %lu; md5: %s", info.size, info.md5);
        
        uint32_t freesz = ESP.getFreeSketchSpace();
        uint32_t cursz = ESP.getSketchSize();
        CONSOLE("current fw size: %lu, avail size for new fw: %lu", cursz, freesz);
        
        if (info.size > freesz)
            WPRC_ERR("FW size too big: %lu > %lu", info.size, freesz);

        // start burn
        if (!Update.begin(info.size, U_FLASH) || !Update.setMD5(info.md5))
            WPRC_ERR("Upd begin fail: errno=%d", Update.getError());
        
        m_cmpl.sz = info.size;
    
    WPRC_RCV(200)
        switch (m_pro->rcvcmd()) {
            case 0x49: { // fwupd data
                struct __attribute__((__packed__)) {
                    uint16_t    sz;
                    uint8_t     buf[1000];
                } d;
                RCV("B1000", d);
                
                auto sz = Update.write(d.buf, d.sz);
                if ((sz == 0) || (sz != d.sz))
                    WPRC_ERR("Burn sz wrong: %d <-> %u", sz, d.sz);
                
                m_cmpl.val += sz;
                
                m_timeout = 200;
                return RUN;
            }

            case 0x4a: { // fwupd end
                RCVNEXT();  // Эта команда без данных
                if (!Update.end())
                    WPRC_ERR("Finalize fail: errno=%d", Update.getError());
                
                cfg.set().fwupdind = 0;
                if (!cfg.save())
                    WPRC_ERR("Config save fail");
                
                SND(0x4c); // fwupd ok
                
                m_isok = true;
                break;
            }

            default: WPRC_ERR("Recv unknown cmd=0x%02x", m_pro->rcvcmd());
        }
        
    WPRC(END)
    }
};

WrkProc<WrkNet> recvFirmware(BinProto *pro) {
    return wrkRun<_recvFirmware, WrkNet>(pro);
}

/* ------------------------------------------------------------------------------------------- *
 *  Приём файлов
 * ------------------------------------------------------------------------------------------- */
bool file_exists(const char *fname, bool external = false);
bool file_remove(const char *fname, bool external = false);
File file_open(const char *fname, FileMy::mode_t mode, bool external = false);
class _recvFiles : public WrkRecv {
        File m_fh;
    
public:
    _recvFiles(BinProto *pro) :
        WrkRecv(pro, netRecvFiles)
    {
    }
#ifdef FWVER_DEBUG
    ~_recvFiles() {
        CONSOLE("wrk(0x%08x) destroy", this);
    }
#endif
    
    state_t run() {
        m_pro->rcvprocess();
        if (!m_pro->rcvvalid())
            WPRC_ERR("recv data fail");
        if (!m_pro->rcvcmpl())
            return chktimeout();
    
    WPROC
        if (m_pro->rcvcmd() != 0x5a) // file list begin
            WPRC_ERR("Recv wrong cmd=0x%02x", m_pro->rcvcmd());
        RCVNEXT();  // Эта команда без данных

    WPRC_RUN
        if (!m_fh) {
            switch (m_pro->rcvcmd()) {
                case 0x5b: { // file begin
                    struct __attribute__((__packed__)) {
                        uint32_t sz;
                        char name[BINPROTO_STRSZ];
                    } h;
                    RCV("Ns", h);

                    char name[BINPROTO_STRSZ] = { '/' };
                    strncpy(name+1, h.name, BINPROTO_STRSZ-2);
                    name[BINPROTO_STRSZ-1] = '\0';
                    CONSOLE("file beg: %s (%d bytes)", name, h.sz);

                    if ( file_exists(name) &&
                        !file_remove(name))
                        WPRC_ERR("Can't remove file: %s", name);
                    m_fh = file_open(name, FileMy::MODE_WRITE);
                    if (!m_fh || m_fh.isDirectory())
                        WPRC_ERR("Can't open for write: %s", name);
                    CONSOLE("file opened");
                    return RUN;
                }
                case 0x5e: // file list finish
                    RCVNEXT();
                    CONSOLE("file list finish");
                    return ok();
            }
        }
        else {
            switch (m_pro->rcvcmd()) {
                case 0x5c: { // file data
                    struct __attribute__((__packed__)) {
                        uint16_t sz;
                        uint8_t buf[256];
                    } d;
                    RCV("B256", d);
                    CONSOLE("file data: %d bytes", d.sz);
                    size_t sz = m_fh.write(d.buf, d.sz);
                    if (sz <= 0)
                        WPRC_ERR("file(%s) write err on pos: %d, sz: %d", m_fh.name(), m_fh.position(), d.sz);
                    return RUN;
                }
                case 0x5d: // file end
                    RCVNEXT();
                    m_fh.close();
                    CONSOLE("file end");
                    return RUN;
            }
        }
        
    WPRC(RUN)
    }

    void end() {
        if (m_fh)
            m_fh.close();
    }
};

/* ------------------------------------------------------------------------------------------- *
 *  Взаимодействие с приложением
 * ------------------------------------------------------------------------------------------- */
static RTC_DATA_ATTR uint16_t _autokey = 0;

class _netApp : public Wrk {
        uint16_t m_kalive;
        uint16_t m_timeout;
        uint16_t m_code;
        
        BinProto *m_pro;
        WrkProc<WrkNet> m_wrk;
    
public:
    _netApp(NetSocket *sock) :
        m_kalive(0),
        m_timeout(200),
        m_code(0),
        m_pro(new BinProto(sock))
    {
    }

#define CONFIRM(cmd, ...)   if (!confirm(cmd, ##__VA_ARGS__)) WPRC_ERR("send confirm fail")
        bool confirm(uint8_t cmd, uint8_t err = 0) {

            struct __attribute__((__packed__)) {
                uint8_t cmd;
                uint8_t err;
            } d = { cmd, err };
            return m_pro->send(0x10, "CC", d);
        }
        
        state_t chktimeout() {
            if (m_timeout == 1)
                WPRC_ERR("Wait timeout");
            
            return DLY;
        }

        void timer() {
            m_kalive ++;

            if (m_timeout > 1)
                m_timeout--;
        }
       
    state_t run() {
            // Ожидание выполнения дочернего процесса
            if (m_wrk.isrun())
                return DLY;
            if (m_wrk.valid()) {
                switch (m_wrk->id()) {
                    case netRecvWiFiPass:
                        CONFIRM(0x41, m_wrk->isok() ? 0 : 1);
                        break;
                    case netRecvFiles:
                        CONFIRM(0x5a, m_wrk->isok() ? 0 : 1);
                        break;
                }
            
                // Удаляем завершённый процесс
                m_wrk.reset();
            }

            if (m_kalive >= 200) {
                m_kalive = 0;
                SND(0x05);
            }
            
            m_pro->rcvprocess();
            switch (m_pro->rcvstate()) {
                case BinProto::RCV_ERROR:
                    WPRC_ERR("recv proto fail");
                case BinProto::RCV_DISCONNECTED:
                    WPRC_ERR("disconnected");
            }
        
    WPROC
        
        CHK_RCV
        // hello/init
        if (m_pro->rcvcmd() != 0x02)
            WPRC_ERR("Recv wrong cmd=0x%02x begin", m_pro->rcvcmd());
        uint16_t autokey = 0;
        RCV("n", autokey);  // Эта команда без данных,

        if ((autokey == 0) || (autokey != _autokey)) {
            // классическая авторизация
            if (autokey > 0)
                CONSOLE("auth by autokey fail: 0x%04x - 0x%04x", autokey, _autokey);
            SND(0x02); // init ok
            m_code = static_cast<uint16_t>(esp_random());
            CONSOLE("Auth code: 0x%04x", m_code);
            setViewNetAuth(m_code);
        }
        else {
            // успешно проведённая повторная авто-авторизация
            m_code = 0;
            CONSOLE("auth ok by autokey: 0x%04x", autokey);
        }
        
        m_timeout = 0;
    WPRC_RUN
        if (m_code > 0) {
            // Ожидание обычной авторизации

            // wait auth code
            if (!viewIsNetAuth())
                WPRC_ERR("Recv auth fail: canceled");
            
            CHK_RCV
            // keep-alive
            if (m_pro->rcvcmd() == 0x05) {
                RCVNEXT();
                CONSOLE("keep-alive before auth");
                return RUN;
            }
            // auth
            if (m_pro->rcvcmd() != 0x03)
                WPRC_ERR("Recv wrong cmd=0x%02x begin", m_pro->rcvcmd());
            uint16_t code = 0;
            RCV("n", code);
            if (code != m_code) {
                uint8_t err = 1;
                SND(0x03, "C", err);
                WPRC_ERR("Recv auth fail: wrong code 0x%04x != 0x%04x", code, m_code);
            }

            CONSOLE("Recv code: 0x%04x = auth ok", code);
            m_code = 0;
            setViewMain();
        }
        
        // Если любая (классическая или авто) авторизация прошли успешно,
        // генерируем новый ключ для авто-авторизации и отравляем его с сообщением
        // об успешной авторизации
        _autokey = static_cast<uint16_t>(esp_random());
        struct __attribute__((__packed__)) {
            uint8_t err;
            uint16_t autokey;
        } authrep = { 0, _autokey };
        SND(0x03, "Cn", authrep);
        CONSOLE("new autokey: 0x%04x", _autokey);
        
    WPRC_RCV(0)
        
        switch (m_pro->rcvcmd()) {
            case 0x05: { // keep-alive
                RCVNEXT();
                CONSOLE("keep-alive");
                break;
            }
            case 0x0a: { // request data
                // тут только данные без параметров, хотя можно попробовать
                // рискнуть принимать union по шаблону "NN..."
                struct __attribute__((__packed__)) {
                    uint8_t code;
                    union {
                        posi_t posi;
                        trksrch_t trk;
                    };
                } d = { 0 };
                RCV("CNNNTC", d);
                CONSOLE("request data: 0x%02x", d.code);
                switch (d.code) {
                    case 0x31:
                        if ((d.posi.beg == 0) || (d.posi.count == 0)) {
                            d.posi.beg = 10;
                            d.posi.count = 10;
                        }
                        sendLogBook(m_pro, d.posi);
                        break;
                    case 0x37: sendWiFiPass(m_pro);         break;
                    case 0x51: sendTrackList(m_pro);        break;
                    case 0x54: sendTrack(m_pro, d.trk);     break;
                    case 0x5a: wrkRun<_sendFiles>(m_pro);   break;
                }
                break;
            }
            case 0x31: { // logbook
                posi_t posi = { 10, 10 };
                RCV("NN", posi);
                sendLogBook(m_pro, posi);
                break;
            }
            case 0x37: { // wifilist
                m_pro->rcvnext(); // нет данных
                sendWiFiPass(m_pro);
                break;
            }
            case 0x41: { // save wifilist
                //m_pro->rcvnext(); // нет данных   // Приём данных этой команды делает сам воркер,
                                                    // со временем, когда уберём wifisync, надо это поправить
                m_wrk = recvWiFiPass(m_pro);
                break;
            }
            case 0x51: { // trklist
                m_pro->rcvnext(); // нет данных
                sendTrackList(m_pro);
                break;
            }
            case 0x54: { // track request
                trksrch_t srch = { 0 };
                RCV("NNNTC", srch);

                sendTrack(m_pro, srch);
                break;
            }
            case 0x5a: { // save files
                //m_pro->rcvnext(); // нет данных   // Приём данных этой команды делает сам воркер,
                                                    // со временем, когда уберём wifisync, надо это поправить
                m_wrk = wrkRun<_recvFiles, WrkNet>(m_pro);
                break;
            }
            default:
                CONSOLE("Unknown request: 0x%02x", m_pro->rcvcmd());
                RCVNEXT();
        }
        
    WPRC(DLY)
    }
    
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

void netApp(NetSocket *sock) {
    wrkRun<_netApp>(sock);
}
