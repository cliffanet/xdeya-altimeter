/*
    WiFi functions
*/

#include "wifisync.h"
#include "../log.h"
#include "../core/workerloc.h"
#include "../core/worker.h"
#include "../jump/track.h"
#include "wifi.h"
#include "binproto.h"
#include "netsync.h"
#include "../cfg/webjoin.h"
#include "../cfg/point.h"


#define ERR(s)              err(err ## s)
#define STATE(s)            m_st = st ## s; CONSOLE("state: %d, timeout: %d", m_st, m_timeout);
#define TIMEOUT(s,n)        m_timeout = n; STATE(s);
#define WPRC_TIMEOUT(s,n)   TIMEOUT(s,n);   WPRC_RUN
#define WPRC_RECV           WPRC_BREAK \
                                if (!m_sock || !m_pro.rcvvalid()) return ERR(RecvData); \
                                if (m_pro.rcvstate() < BinProto::RCV_COMPLETE) return chktimeout();

#define SEND(cmd, ...)      if (!m_sock || !m_pro.send(cmd, ##__VA_ARGS__)) return ERR(SendData);
#define RECV(pk, ...)       if (!m_sock || !m_pro.rcvdata(PSTR(pk), ##__VA_ARGS__)) return ERR(SendData);

using namespace wSync;

class _wifiSync : public Wrk {
    public:
        st_t        m_st;
        uint16_t    m_timeout;
        const char  *m_ssid, *m_pass;
        NetSocket   *m_sock;
        BinProto    m_pro;
        WrkProc<WrkNet> m_wrk;

        uint32_t    m_joinnum;
        struct __attribute__((__packed__)) {
            uint32_t ckscfg;
            uint32_t cksjmp;
            uint32_t ckspnt;
            uint32_t ckslog;
            uint32_t poslog;
            FileTrack::chs_t ckstrack;
        } m_accept;
        uint8_t m_fwupd;

        struct __attribute__((__packed__)) {
            uint8_t  count;
            uint32_t fsize;
        } m_trklist;
        uint32_t m_trksnd;
        
        _wifiSync(const char *ssid, const char *pass) :
            m_st(stWiFiInit),
            m_timeout(0),
            m_sock(NULL),
            m_pro(NULL, '%', '#'),
            m_joinnum(0),
            m_accept({ 0 }),
            m_fwupd(0),
            m_trklist({ 0 }),
            m_trksnd(0)
        {
            // Приходится копировать, т.к. к моменту,
            // когда мы этими строками воспользуемся, их источник будет удалён.
            // Пробовал в конструкторе делать wifi connect,
            // но эти процессы не быстрые, лучше их оставить воркеру.
            m_ssid = strdup(ssid);
            m_pass = pass != NULL ? strdup(pass) : NULL;
            //optset(O_NOREMOVE); не требуется
        }
    
    ~_wifiSync() {
        if (m_fwupd && (m_st == stFinOk)) {
            CONSOLE("reboot after firmware update");
            ESP.restart();
        }
        CONSOLE("_wifiSync(0x%08x) destroy", this);
    }
    
        state_t err(st_t st) {
            m_st = st;
            CONSOLE("err: %d", st);
            return END;
        }
        state_t chktimeout() {
            if (m_timeout == 0)
                return DLY;
            
            m_timeout--;
            if (m_timeout > 0)
                return DLY;
            
            return err(errTimeout);
        }
        
        void cancel() {
            m_st = stUserCancel;
            m_timeout = 0;
        }
        
        WrkNet::cmpl_t complete() const {
            if (!m_wrk.valid())
                return { 0, 0 };
            
            switch (m_wrk->id()) {
                case netSendTrack:
                    return { m_trksnd + m_wrk->cmpl().val, m_trklist.fsize };
                case netRecvFirmware:
                    return m_wrk->cmpl();
            }

            return { 0, 0 };
        }
    
    state_t run() {
        if (m_st == stUserCancel)
            return END;
        
        // Ожидание выполнения дочернего процесса
        if (m_wrk.isrun())
            return DLY;
        else
        if (m_wrk.valid()) {
            CONSOLE("wrk(%08x) finished isok: %d", m_wrk.key(), m_wrk->isok());
            if (!m_wrk->isok())
                return ERR(Worker);
            
            if (m_wrk->id() == netSendTrack)
                // обновляем m_trksnd - суммарный размер уже полностью отправленных треков
                m_trksnd += m_wrk->cmpl().val;
            
            m_wrk.reset();

            // Обновляем таймаут после завершения процесса на случай, если это был приём данных
            // и следующим циклом ожидается приём уже в основной процесс
            if (m_timeout == 0)
                m_timeout = 200;
        }
        
        if ((m_sock != NULL) && m_sock->connected()) {
            m_pro.rcvprocess();
            if (!m_pro.rcvvalid())
                return ERR(RecvData);
        }
        
    WPROC
        if (!wifiStart())
            return ERR(WiFiInit);
    
    WPRC_TIMEOUT(WiFiConnect, 400)
        CONSOLE("wifi to: %s; pass: %s", m_ssid, m_pass == NULL ? "-no-" : m_pass);
        if (!wifiConnect(m_ssid, m_pass))
            return ERR(WiFiConnect);
    
    WPRC_RUN
        // ожидаем соединения по вифи
        switch (wifiStatus()) {
            case WIFI_STA_CONNECTED:
                CONSOLE("wifi ok, try to server connect");
                // вифи подключилось, соединяемся с сервером
                break;
            
            case WIFI_STA_FAIL:
                return ERR(WiFiConnect);
            
            default:
                return chktimeout();
        }
        
    WPRC_TIMEOUT(SrvConnect, 0)
        // ожидаем соединения к серверу
        if (m_sock == NULL)
            m_sock = wifiCliConnect();
        if (m_sock == NULL)
            return ERR(SrvConnect);
        m_pro.sock_set(m_sock);
    
    WPRC_TIMEOUT(SrvAuth, 200)
        // авторизируемся на сервере
        ConfigWebJoin wjoin;
        if (!wjoin.load())
            return ERR(JoinLoad);
        CONSOLE("[authStart] authid: %lu", wjoin.authid());
        SEND(0x01, PSTR("N"), wjoin.authid());
    
    WPRC_BREAK
        // из-за непоследовательности операций при авторизации
        // тут вместо макроса WPRC_RECV
        // делаем приём ответа от сервера вручную
        if (!m_pro.rcvvalid()) return ERR(RecvData);
        
        if (m_st == stSrvAuth) {
            if (m_pro.rcvstate() < BinProto::RCV_COMPLETE) return chktimeout();
            // Ожидаем ответ на наш запрос авторизации
            CONSOLE("[waitHello] cmd: %02x", m_pro.rcvcmd());
            switch (m_pro.rcvcmd()) {
                case 0x10: // rejoin
                    // требуется привязка к аккаунту
                    RECV("N", m_joinnum);
                    // После приёма этой команды ожидается приём следующей,
                    // это будет означать, что пользователь ввёл цифры на приборе
                    // на сервере, а сервер отправил новые данные привязки
                    TIMEOUT(ProfileJoin,1200);
                    return RUN;

                case 0x20: // accept
                    // стартуем передачу данных на сервер
                    RECV("XXXXNH", m_accept);
                    CONSOLE("recv ckstrack: %04x %04x %08x", m_accept.ckstrack.csa, m_accept.ckstrack.csb, m_accept.ckstrack.sz);
                    break;
                    
                default:
                    return ERR(RcvCmdUnknown);
            }
        }
        else
        if (m_st == stProfileJoin) {
            // устройство не прикреплено ни к какому профайлу, ожидается привязка
            if (m_pro.rcvstate() < BinProto::RCV_COMPLETE) {
                // Собственно, тут и есть причина того, что нам приходится вручную
                // разбирать приём команды с сервера.
                // Отправляем idle- чтобы на сервере не сработал таймаут передачи
                if ((m_timeout & 0xF) == 0) {
                    CONSOLE("send idle");
                    SEND(0x12);
                }
                return chktimeout();
            }
            
            // в этой команде должны прийти данные с привязкой к вебу
            if (m_pro.rcvcmd() != 0x13)
                return ERR(RcvCmdUnknown);
            struct __attribute__((__packed__)) {
                uint32_t id;
                uint32_t secnum;
            } auth;
            RECV("NN", auth);
            CONSOLE("[waitJoin] confirm: %lu, %lu", auth.id, auth.secnum);
            
            ConfigWebJoin wjoin(auth.id, auth.secnum);
            if (!wjoin.save())
                return ERR(JoinSave);

            m_joinnum = 0;
            SEND(0x14);
            TIMEOUT(SrvAuth, 200);
            // Заного отправляем authid
            SEND(0x01, PSTR("N"), auth.id);

            return RUN;
        }
    
    WPRC_TIMEOUT(SendConfig, 0)
        // отправка основного конфига
        if ((m_accept.ckscfg == 0) || (m_accept.ckscfg != cfg.chksum()))
            if (!sendCfgMain(&m_pro))
                return ERR(SendData);
    
    WPRC_TIMEOUT(SendJumpCount, 0)
        if ((m_accept.cksjmp == 0) || (m_accept.cksjmp != jmp.chksum()))
            if (!sendJmpCount(&m_pro))
                return ERR(SendData);
    
    WPRC_TIMEOUT(SendPoint, 0)
        if ((m_accept.ckspnt == 0) || (m_accept.ckspnt != pnt.chksum()))
            if (!sendPoint(&m_pro))
                return ERR(SendData);
    
    WPRC_TIMEOUT(SendPoint, 0)
        if ((m_accept.ckspnt == 0) || (m_accept.ckspnt != pnt.chksum()))
            if (!sendPoint(&m_pro))
                return ERR(SendData);
    
    WPRC_TIMEOUT(SendLogBook, 0)
        m_wrk = sendLogBook(&m_pro, m_accept.ckslog, m_accept.poslog);
    
    WPRC_TIMEOUT(SendTrackList, 0)
        m_wrk = sendTrackList(&m_pro);
        
    WPRC_RUN // Надо запомнить точку и выйти, но не менять m_st
        TIMEOUT(SendDataFin, 300);
        // завершение отправки данных, теперь будем получать обновления с сервера
        if (!sendDataFin(&m_pro))
            return ERR(SendData);
    
    WPRC_RECV
        CONSOLE("[waitFin] cmd: %02x", m_pro.rcvcmd());
        switch (m_pro.rcvcmd()) {
            case 0x41: // wifi beg
                TIMEOUT(RecvWiFiPass, 0);
                m_wrk = recvWiFiPass(&m_pro);
                return RUN;
            
            case 0x44: // veravail beg
                TIMEOUT(RecvVerAvail, 0);
                m_wrk = recvVerAvail(&m_pro);
                return RUN;
            
            case 0x47: // firmware update beg
                TIMEOUT(RecvFirmware, 0);
                m_wrk = recvFirmware(&m_pro);
                m_fwupd = 1;
                return RUN;
            
            case 0x50: // track list summary
                TIMEOUT(SendTrack, 100);
                // команды выше распаковывают данные внутри подворкеров,
                // а нам надо распаковать тут
                RECV("C   N", m_trklist);
                CONSOLE("request %d tracks; fsize: %lu", m_trklist.count, m_trklist.fsize);
                return RUN;
            
            case 0x54: { // track request
                // Сделаем тут таймаут побольше, считаться он будет только после завершения процесса передачи.
                // Большой таймаут нужен, чтобы после отправки данных нужно время, чтобы принимающая сторона
                // доприняла все даныне и отправила запрос на следующий трек.
                TIMEOUT(SendTrack, 300);
                // команды выше распаковывают данные внутри подворкеров,
                // а нам надо распаковать тут
                trksrch_t srch;
                RECV("NNNTC", srch);

                m_wrk = sendTrack(&m_pro, srch);
                return DLY;
            }
            
            case 0x0f: // bye
                TIMEOUT(FinOk, 0);
                break;
        };
        
    WPRC(END)
    }
        
    void end() {
        if (m_wrk.isrun())
            m_wrk.stop();
        if (m_wrk.valid())
            m_wrk.reset();
        
        m_pro.sock_clear();
        
        if (m_sock != NULL) {
            m_sock->close();
            CONSOLE("delete m_sock");
            delete m_sock;
            m_sock = NULL;
        }
        
        wifiStop();
        
        if (m_ssid != NULL) {
            delete m_ssid;
            m_ssid = NULL;
        }
        if (m_pass != NULL) {
            delete m_pass;
            m_pass = NULL;
        }
    }
};

/* ------------------------------------------------------------------------------------------- *
 *  Инициализация
 * ------------------------------------------------------------------------------------------- */
static WrkProc<_wifiSync> _wfs;

void wifiSyncBegin(const char *ssid, const char *pass) {
    if (_wfs.isrun())
        return;
    _wfs = wrkRun<_wifiSync>(ssid, pass);
    CONSOLE("begin");
}

wSync::st_t wifiSyncState(wSync::info_t &inf) {
    if (!_wfs.valid())
        return wSync::stNotValid;
    
    inf.joinnum = _wfs->m_joinnum;
    inf.timeout = _wfs->m_timeout;
    inf.cmplval = _wfs->complete().val;
    inf.cmplsz  = _wfs->complete().sz;
    
    return _wfs->m_st;
}

bool wifiSyncIsRun() {
    return _wfs.isrun();
}

bool wifiSyncStop() {
    if (!_wfs.isrun())
        return false;
    
    _wfs->cancel();
    return true;
}


void wifiSyncDelete() {
    _wfs.reset();
}
