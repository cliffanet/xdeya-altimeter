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


#define ERR(s)                  err(err ## s)
#define RETURN_ERR(s)           return ERR(s)
#define STATE(s)                m_st = st ## s; CONSOLE("state: %d, timeout: %d", m_st, m_timeout);
#define TIMEOUT(s,n)            m_timeout = n; STATE(s);
#define WRK_BREAK_STATE(s)      STATE(s);       WRK_BREAK_RUN
#define WRK_BREAK_TIMEOUT(s,n)  TIMEOUT(s,n);   WRK_BREAK_RUN
#define WRK_BREAK_RECV          WRK_BREAK \
                                if (!m_sock || !m_pro.rcvvalid()) RETURN_ERR(RecvData); \
                                if (m_pro.rcvstate() < BinProto::RCV_COMPLETE) return chktimeout();

#define SEND(cmd, ...)          if (!m_sock || !m_pro.send(cmd, ##__VA_ARGS__)) RETURN_ERR(SendData);
#define RECV(pk, ...)           if (!m_sock || !m_pro.rcvdata(PSTR(pk), ##__VA_ARGS__)) RETURN_ERR(SendData);

using namespace wSync;

WRK_DEFINE(WIFI_SYNC) {
    public:
        st_t        m_st;
        uint16_t    m_timeout;
        const char  *m_ssid, *m_pass;
        NetSocket   *m_sock;
        BinProto    m_pro;
        WrkProc::key_t m_wrk;

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
        
        WRK_CLASS(WIFI_SYNC)(const char *ssid, const char *pass) :
            m_st(stWiFiInit),
            m_timeout(0),
            m_sock(NULL),
            m_pro(NULL, '%', '#'),
            m_wrk(WRKKEY_NONE),
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
    
        state_t err(st_t st) {
            m_st = st;
            CONSOLE("err: %d", st);
            WRK_RETURN_FINISH;
        }
        state_t chktimeout() {
            if (m_timeout == 0)
                WRK_RETURN_WAIT;
            
            m_timeout--;
            if (m_timeout > 0)
                WRK_RETURN_WAIT;
            
            return err(errTimeout);
        }
        
        void cancel() {
            if (isrun()) {
                m_st = stUserCancel;
                m_timeout = 0;
            }
        }
        
        cmpl_t complete() {
            if (m_wrk == WRKKEY_RECV_FIRMWARE)
                return cmplFirmware();
            if (m_wrk == WRKKEY_SEND_TRACK) {
                auto c = cmplTrack();
                return { m_trksnd + c.val, m_trklist.fsize };
            }
            
            return { 0, 0 };
        }
    
    // Это выполняем всегда перед входом в process
    state_t every() {
        if (m_wrk != WRKKEY_NONE) {
            // Ожидание выполнения дочернего процесса
            auto wrk = _wrkGet(m_wrk);
            if (wrk == NULL) {
                CONSOLE("worker[key=%d] finished unexpectedly", m_wrk);
                RETURN_ERR(Worker);
            }
            if (wrk->isrun())
                WRK_RETURN_WAIT;
            
            // Дочерний процесс завершился, проверяем его статус
            bool isok =
                m_wrk == WRKKEY_SEND_LOGBOOK ?
                    isokLogBook(wrk) :
                m_wrk == WRKKEY_SEND_TRACKLIST ?
                    isokTrackList(wrk) :
                m_wrk == WRKKEY_SEND_TRACK ?
                    isokTrack(wrk) :
                m_wrk == WRKKEY_RECV_WIFIPASS ?
                    isokWiFiPass(wrk) :
                m_wrk == WRKKEY_RECV_VERAVAIL ?
                    isokVerAvail(wrk) :
                m_wrk == WRKKEY_RECV_FIRMWARE ?
                    isokFirmware(wrk) :
                    false;
            CONSOLE("worker[key=%d] finished isok: %d", m_wrk, isok);
            
            if (m_wrk == WRKKEY_SEND_TRACK) {
                // обновляем m_trksnd - суммарный размер уже полностью отправленных треков
                auto c = cmplTrack();
                m_trksnd += c.val;
            }
            
            // Удаляем завершённый процесс
            _wrkDel(m_wrk);
            m_wrk = WRKKEY_NONE;
            
            if (!isok) // Если процесс завершился с ошибкой, завершаем и себя тоже с ошибкой
                RETURN_ERR(Worker);
            
            // Обновляем таймаут после завершения процесса на случай, если это был приём данных
            // и следующим циклом ожидается приём уже в основной процесс
            if (m_timeout == 0)
                m_timeout = 200;
        }
        
        if (m_st == stUserCancel)
            WRK_RETURN_FINISH;
        
        if ((m_sock != NULL) && m_sock->connected()) {
            m_pro.rcvprocess();
            if (!m_pro.rcvvalid())
                RETURN_ERR(RecvData);
        }
        
        WRK_RETURN_RUN;
    }
    
    WRK_PROCESS
        if (!wifiStart())
            RETURN_ERR(WiFiInit);
    
    WRK_BREAK_TIMEOUT(WiFiConnect, 400)
        CONSOLE("wifi to: %s; pass: %s", m_ssid, m_pass == NULL ? "-no-" : m_pass);
        if (!wifiConnect(m_ssid, m_pass))
            RETURN_ERR(WiFiConnect);
    
    WRK_BREAK_RUN
        // ожидаем соединения по вифи
        switch (wifiStatus()) {
            case WIFI_STA_CONNECTED:
                CONSOLE("wifi ok, try to server connect");
                // вифи подключилось, соединяемся с сервером
                break;
            
            case WIFI_STA_FAIL:
                RETURN_ERR(WiFiConnect);
            
            default:
                return chktimeout();
        }
        
    WRK_BREAK_TIMEOUT(SrvConnect, 0)
        // ожидаем соединения к серверу
        if (m_sock == NULL)
            m_sock = wifiCliConnect();
        if (m_sock == NULL)
            RETURN_ERR(SrvConnect);
        m_pro.sock_set(m_sock);
    
    WRK_BREAK_TIMEOUT(SrvAuth, 200)
        // авторизируемся на сервере
        ConfigWebJoin wjoin;
        if (!wjoin.load())
            RETURN_ERR(JoinLoad);
        CONSOLE("[authStart] authid: %lu", wjoin.authid());
        SEND(0x01, PSTR("N"), wjoin.authid());
    
    WRK_BREAK
        // из-за непоследовательности операций при авторизации
        // тут вместо макроса WRK_BREAK_RECV
        // делаем приём ответа от сервера вручную
        if (!m_pro.rcvvalid()) RETURN_ERR(RecvData);
        
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
                    WRK_RETURN_RUN;

                case 0x20: // accept
                    // стартуем передачу данных на сервер
                    RECV("XXXXNH", m_accept);
                    CONSOLE("recv ckstrack: %04x %04x %08x", m_accept.ckstrack.csa, m_accept.ckstrack.csb, m_accept.ckstrack.sz);
                    break;
                    
                default:
                    RETURN_ERR(RcvCmdUnknown);
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
                RETURN_ERR(RcvCmdUnknown);
            struct __attribute__((__packed__)) {
                uint32_t id;
                uint32_t secnum;
            } auth;
            RECV("NN", auth);
            CONSOLE("[waitJoin] confirm: %lu, %lu", auth.id, auth.secnum);
            
            ConfigWebJoin wjoin(auth.id, auth.secnum);
            if (!wjoin.save())
                RETURN_ERR(JoinSave);

            m_joinnum = 0;
            SEND(0x14);
            TIMEOUT(SrvAuth, 200);
            // Заного отправляем authid
            SEND(0x01, PSTR("N"), auth.id);

            WRK_RETURN_RUN;
        }
    
    WRK_BREAK_TIMEOUT(SendConfig, 0)
        // отправка основного конфига
        if ((m_accept.ckscfg == 0) || (m_accept.ckscfg != cfg.chksum()))
            if (!sendCfgMain(&m_pro))
                RETURN_ERR(SendData);
    
    WRK_BREAK_TIMEOUT(SendJumpCount, 0)
        if ((m_accept.cksjmp == 0) || (m_accept.cksjmp != jmp.chksum()))
            if (!sendJmpCount(&m_pro))
                RETURN_ERR(SendData);
    
    WRK_BREAK_TIMEOUT(SendPoint, 0)
        if ((m_accept.ckspnt == 0) || (m_accept.ckspnt != pnt.chksum()))
            if (!sendPoint(&m_pro))
                RETURN_ERR(SendData);
    
    WRK_BREAK_TIMEOUT(SendPoint, 0)
        if ((m_accept.ckspnt == 0) || (m_accept.ckspnt != pnt.chksum()))
            if (!sendPoint(&m_pro))
                RETURN_ERR(SendData);
    
    WRK_BREAK_TIMEOUT(SendLogBook, 0)
        m_wrk = sendLogBook(&m_pro, m_accept.ckslog, m_accept.poslog, true);
        if (m_wrk == WRKKEY_NONE)
            RETURN_ERR(SendData);
    
    WRK_BREAK_TIMEOUT(SendTrackList, 0)
        m_wrk = sendTrackList(&m_pro, true);
        if (m_wrk == WRKKEY_NONE)
            RETURN_ERR(SendData);
        
    WRK_BREAK_RUN // Надо запомнить точку и выйти, но не менять m_st
        TIMEOUT(SendDataFin, 300);
        // завершение отправки данных, теперь будем получать обновления с сервера
        if (!sendDataFin(&m_pro))
            RETURN_ERR(SendData);
    
    WRK_BREAK_RECV
        CONSOLE("[waitFin] cmd: %02x", m_pro.rcvcmd());
        switch (m_pro.rcvcmd()) {
            case 0x41: // wifi beg
                TIMEOUT(RecvWiFiPass, 0);
                m_wrk = recvWiFiPass(&m_pro, true);
                if (m_wrk == WRKKEY_NONE)
                    RETURN_ERR(RecvData);
                WRK_RETURN_RUN;
            
            case 0x44: // veravail beg
                TIMEOUT(RecvVerAvail, 0);
                m_wrk = recvVerAvail(&m_pro, true);
                if (m_wrk == WRKKEY_NONE)
                    RETURN_ERR(RecvData);
                WRK_RETURN_RUN;
            
            case 0x47: // firmware update beg
                TIMEOUT(RecvFirmware, 0);
                m_wrk = recvFirmware(&m_pro, true);
                if (m_wrk == WRKKEY_NONE)
                    RETURN_ERR(RecvData);
                m_fwupd = 1;
                WRK_RETURN_RUN;
            
            case 0x50: // track list summary
                TIMEOUT(SendTrack, 100);
                // команды выше распаковывают данные внутри подворкеров,
                // а нам надо распаковать тут
                RECV("C   N", m_trklist);
                CONSOLE("request %d tracks; fsize: %lu", m_trklist.count, m_trklist.fsize);
                WRK_RETURN_RUN;
            
            case 0x54: { // track request
                // Сделаем тут таймаут побольше, считаться он будет только после завершения процесса передачи.
                // Большой таймаут нужен, чтобы после отправки данных нужно время, чтобы принимающая сторона
                // доприняла все даныне и отправила запрос на следующий трек.
                TIMEOUT(SendTrack, 300);
                // команды выше распаковывают данные внутри подворкеров,
                // а нам надо распаковать тут
                trksrch_t srch;
                RECV("NNNTC", srch);

                m_wrk = sendTrack(&m_pro, srch, true);
                if (m_wrk == WRKKEY_NONE)
                    RETURN_ERR(RecvData);
                WRK_RETURN_RUN;
            }
            
            case 0x0f: // bye
                TIMEOUT(FinOk, 0);
                break;
        };
        
    WRK_FINISH
        
    void end() {
        if (m_wrk != WRKKEY_NONE) {
            _wrkDel(m_wrk);
            m_wrk = WRKKEY_NONE;
        }
        
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
    
    void remove() {
        if (m_fwupd && (m_st == stFinOk)) {
            CONSOLE("reboot after firmware update");
            ESP.restart();
        }
    }
};

/* ------------------------------------------------------------------------------------------- *
 *  Инициализация
 * ------------------------------------------------------------------------------------------- */

void wifiSyncBegin(const char *ssid, const char *pass) {
    if (wrkExists(WIFI_SYNC))
        return;
    wrkRun(WIFI_SYNC, ssid, pass);
    CONSOLE("begin");
}

wSync::st_t wifiSyncState(wSync::info_t &inf) {
    auto wrk = wrkGet(WIFI_SYNC);
    if (wrk == NULL)
        return wSync::stNotRun;
    
    inf.joinnum = wrk->m_joinnum;
    inf.timeout = wrk->m_timeout;
    auto cmpl   = wrk->complete();
    inf.cmplval = cmpl.val;
    inf.cmplsz  = cmpl.sz;
    
    return wrk->m_st;
}

bool wifiSyncIsRun() {
    auto wrk = wrkGet(WIFI_SYNC);
    return (wrk != NULL) && wrk->isrun();
}

bool wifiSyncStop() {
    auto wrk = wrkGet(WIFI_SYNC);
    if ((wrk == NULL) || !wrk->isrun())
        return false;
    
    wrk->cancel();
    return true;
}


bool wifiSyncDelete() {
    return wrkStop(WIFI_SYNC);
}
