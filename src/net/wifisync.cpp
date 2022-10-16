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

/*



/////////

        
        void settimer(uint32_t val);
        void clrtimer();
        void dectimer();
        bool istimeout() const { return m_timer == 1; }
        uint32_t timer() const { return m_timer > 0 ? m_timer-1 : 0; }
        
        uint32_t op() const { return m_op; }
        void setop(uint32_t op);
        void next();
        void next(uint32_t timer);

void WorkerProc::settimer(uint32_t val) {
    m_timer = val+1;
}

void WorkerProc::clrtimer() {
    m_timer = 0;
}

void WorkerProc::dectimer() {
    if (m_timer > 1)
        m_timer --;
}

void WorkerProc::setop(uint32_t op) {
    m_op = op;
}

void WorkerProc::next() {
    m_op ++;
}

void WorkerProc::next(uint32_t timer) {
    next();
    
    if (timer > 0)
        settimer(timer);
    else
    if (m_timer > 0)
        clrtimer();
}

/////////


#define ERR(st)             err(err ## st)
#define SOCK                BinProtoSend(m_sock, '%')
#define SEND(cmd, ...)      if (!SOCK.send(cmd, ##__VA_ARGS__)) RETURN_ERR(SendData);


WorkerWiFiSync::WorkerWiFiSync(const char *ssid, const char *pass) :
    m_sock(NULL),
    m_pro(NULL),
    m_st(stRun),
    m_wrk(0)
{
    CONSOLE("[%08x] create", this);
    
    setop(opWiFiConnect);
    settimer(300);
    
    if (!wifiStart()) {
        ERR(WiFiInit);
        return;
    }
    if (!wifiConnect(ssid, pass)) {
        ERR(WiFiConnect);
        return;
    }
}
WorkerWiFiSync::~WorkerWiFiSync() {
    CONSOLE("[%08x] destroy", this);
    if (m_pro != NULL)
        delete m_pro;
    if (m_sock != NULL)
        delete m_sock;
}

void WorkerWiFiSync::err(st_t st) {
    m_st = st;
    stop();
}

void WorkerWiFiSync::stop() {
    setop(isrun() ? opClose : opExit);
    clrtimer();
}

void WorkerWiFiSync::initpro() {
    if ((m_sock == NULL) || (m_pro != NULL))
        return;
    
    m_pro = new BinProtoRecv(m_sock, '#');
    // rejoin
    m_pro->add( 0x10, PSTR("N") );
    // accept
    m_pro->add( 0x20, PSTR("XXXXNH") );
    
    //join accept
    m_pro->add( 0x13, PSTR("NN") );
}

void WorkerWiFiSync::end() {
    if (m_pro != NULL) {
        CONSOLE("delete m_pro");
        delete m_pro;
        m_pro = NULL;
    }
    if (m_sock != NULL) {
        m_sock->disconnect();
        CONSOLE("delete m_sock");
        delete m_sock;
        m_sock = NULL;
    }
    wifiStop();
}

WorkerWiFiSync::state_t
WorkerWiFiSync::process() {
    if (istimeout())
        ERR(Timeout);

#define RETURN_ERR(e)       { ERR(e); return STATE_RUN; }
#define RETURN_NEXT(tmr)    { next(tmr); return STATE_RUN; }
    
    // приём входных данных
    if (isrun() && (m_pro != NULL))
        switch (m_pro->process()) {
            case BinProtoRecv::STATE_OK:
                settimer(100);
                return STATE_RUN;
            
            case BinProtoRecv::STATE_CMD:
                {
                    uint8_t cmd;
                    if (!m_pro->recv(cmd, d))
                        RETURN_ERR(RecvData);
                    // обработка входных данных
                    if (!recvdata(cmd))
                        RETURN_ERR(RcvCmdUnknown);
                }
                return STATE_RUN;
                
            case BinProtoRecv::STATE_ERROR:
                RETURN_ERR(RecvData);
        }
    
    // проверка подключения к серверу
    if ((m_sock != NULL) && (!m_sock->connected()))
        RETURN_ERR(SrvDisconnect);
    
    // выполнение плановых процедур
    switch (op()) {
        case opExit:
            return STATE_END;
        
        case opClose:
            end();
            if (m_st == stRun)
                m_st = stUserCancel;
            next(0);
            return STATE_WAIT;
        
        case opProfileJoin:
        // устройство не прикреплено ни к какому профайлу, ожидается привязка
            if ((timer() & 0xF) == 0)
                SEND(0x12, PSTR("N"), timer()*100);
            
            return STATE_WAIT;
        
        case opWiFiConnect:
        // ожидаем соединения по вифи
            switch (wifiStatus()) {
                case WIFI_STA_CONNECTED:
                    CONSOLE("wifi ok, try to server connect");
                    // вифи подключилось, соединяемся с сервером
                    RETURN_NEXT(100);
                
                case WIFI_STA_FAIL:
                    RETURN_ERR(WiFiConnect);
            }
            
            return STATE_WAIT;
        
        case opSrvConnect:
        // ожидаем соединения к серверу
            if (m_sock == NULL)
                m_sock = wifiCliCreate();
            if (!m_sock->connect())
                RETURN_ERR(SrvConnect);
            
            initpro();
            
            RETURN_NEXT(100);
        
        case opSrvAuth:
        // авторизируемся на сервере
            {
                ConfigWebJoin wjoin;
                if (!wjoin.load())
                    RETURN_ERR(JoinLoad);

                CONSOLE("[authStart] authid: %lu", wjoin.authid());
            
                SEND(0x01, PSTR("N"), wjoin.authid());
            }
            
            RETURN_NEXT(200);
        
        case opSndConfig:
        // отправка основного конфига
            if ((d.acc.ckscfg == 0) || (d.acc.ckscfg != cfg.chksum()))
                if (!sendCfgMain(m_sock))
                    RETURN_ERR(SendData);
            
            RETURN_NEXT(10);
        
        case opSndJumpCount:
        // отправка количества прыжков
            if ((d.acc.cksjmp == 0) || (d.acc.cksjmp != jmp.chksum()))
                if (!sendJmpCount(m_sock))
                    RETURN_ERR(SendData);
            
            RETURN_NEXT(10);
        
        case opSndPoint:
        // отправка Navi-точек
            if ((d.acc.ckspnt == 0) || (d.acc.ckspnt != pnt.chksum()))
                if (!sendPoint(m_sock))
                    RETURN_ERR(SendData);
            
            RETURN_NEXT(10);
        
        case opSndLogBook:
        // отправка LogBook
            if (!sendLogBook(m_sock, d.acc.ckslog, d.acc.poslog))
                RETURN_ERR(SendData);
            
            RETURN_NEXT(10);
        
        case opSndTrack:
        // отправка треков - в отдельном worker
            //if (m_wrk == 0) {
            //    m_wrk = trkSend(m_sock);
            //    if (m_wrk == 0)
            //        RETURN_ERR(Worker);
            //}
            //else
            //if (wrkExists(m_wrk))
            //    return STATE_WAIT;
            RETURN_NEXT(10);
        
        case opSndDataFin:
        // завершение отправки данных, теперь будем получать обновления с сервера
            if (!sendDataFin(m_sock))
                RETURN_ERR(SendData);
            
            RETURN_NEXT(100);
    }

#undef RETURN_ERR
#undef RETURN_NEXT
    
    return STATE_WAIT;
}

bool WorkerWiFiSync::recvdata(uint8_t cmd) {

#define RETURN_ERR(e)           { ERR(e); return true; }
#define RETURN_OP(o,tmr)        { setop(op ## o); settimer(tmr); return true; }
#define RETURN_NEXT(tmr)        { next(tmr); return true; }

    switch (op()) {
    
        case opWaitAuth:
            switch (cmd) {
                case 0x10: // rejoin
                    RETURN_OP(ProfileJoin, 1200);
                    
                case 0x20: // accept
                    CONSOLE("recv ckstrack: %04x %04x %08x", d.acc.ckstrack.csa, d.acc.ckstrack.csb, d.acc.ckstrack.sz);
                    RETURN_NEXT(10);
            }
            return false;
        
        case opProfileJoin:
            if (cmd == 0x13) {
                CONSOLE("[waitJoin] cmd: %02x (%lu, %lu)", cmd, d.join.authid, d.join.secnum);
                ConfigWebJoin wjoin(d.join.authid, d.join.secnum);
                if (!wjoin.save())
                    RETURN_ERR(JoinSave);
                
                SEND(0x14);
                
                RETURN_OP(SrvAuth, 100);
            }
            return false;
    }
    
#undef RETURN_ERR
#undef RETURN_OP
#undef RETURN_NEXT
    
    return false;
}

WorkerWiFiSync * wifiSyncProc() {
    return reinterpret_cast<WorkerWiFiSync *>(wrkGet(WORKER_WIFI_SYNC));
}
*/


#define ERR(s)                  err(err ## s)
#define RETURN_ERR(s)           return ERR(s)
#define STATE(s)                m_st = st ## s; CONSOLE("state: %d", m_st);
#define TIMEOUT(s,n)            STATE(s); m_timeout = n
#define WRK_BREAK_STATE(s)      STATE(s);       WRK_BREAK_RUN
#define WRK_BREAK_TIMEOUT(s,n)  TIMEOUT(s,n);   WRK_BREAK_RUN
#define WRK_BREAK_RECV          WRK_BREAK \
                                if (!m_sock || !m_pro.rcvvalid()) RETURN_ERR(RecvData); \
                                if (m_pro.rcvstate() < BinProto::RCV_COMPLETE) return chktimeout();

#define SEND(cmd, ...)          if (!m_sock || !m_pro.send(cmd, ##__VA_ARGS__)) RETURN_ERR(SendData);
#define RECV(pk, data)          if (!m_sock || !m_pro.rcvdata(PSTR(pk), data)) RETURN_ERR(SendData);

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
        
        WRK_CLASS(WIFI_SYNC)(const char *ssid, const char *pass) :
            m_st(stWiFiInit),
            m_timeout(0),
            m_sock(NULL),
            m_pro(NULL, '%', '#'),
            m_wrk(WRKKEY_NONE),
            m_joinnum(0),
            m_accept({ 0 })
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
            if (isrun())
                m_st = stUserCancel;
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
                    false;
            CONSOLE("worker[key=%d] finished isok: %d", m_wrk, isok);
            
            // Удаляем завершённый процесс
            _wrkDel(m_wrk);
            m_wrk = WRKKEY_NONE;
            
            if (!isok) // Если процесс завершился с ошибкой, завершаем и себя тоже с ошибкой
                RETURN_ERR(Worker);
        }
        
        if (m_st == stUserCancel)
            WrkProc::STATE_FINISH;
        
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
            m_sock = wifiCliCreate();
        if (!m_sock->connect())
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
    
    WRK_BREAK_TIMEOUT(SendDataFin, 300)
        // завершение отправки данных, теперь будем получать обновления с сервера
        if (!sendDataFin(&m_pro))
            RETURN_ERR(SendData);
    
    WRK_BREAK_RECV
        CONSOLE("[waitFin] cmd: %02x", m_pro.rcvcmd());
        
        TIMEOUT(FinOk, 0);
    WRK_FINISH
        
    void end() {
        if (m_wrk != WRKKEY_NONE) {
            _wrkDel(m_wrk);
            m_wrk = WRKKEY_NONE;
        }
        
        m_pro.sock_clear();
        
        if (m_sock != NULL) {
            m_sock->disconnect();
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

void wifiSyncBegin(const char *ssid, const char *pass) {
    if (wrkExists(WIFI_SYNC))
        return;
    wrkRun(WIFI_SYNC, ssid, pass);
    CONSOLE("begin");
}

wSync::st_t wifiSyncState(wSync::info_t &inf) {
    auto proc = wrkGet(WIFI_SYNC);
    if (proc == NULL)
        return wSync::stNotRun;
    
    inf.joinnum = proc->m_joinnum;
    inf.timeout = proc->m_timeout;
    
    return proc->m_st;
}

bool wifiSyncIsRun() {
    auto proc = wrkGet(WIFI_SYNC);
    return (proc != NULL) && proc->isrun();
}

bool wifiSyncStop() {
    auto proc = wrkGet(WIFI_SYNC);
    if ((proc == NULL) || !proc->isrun())
        return false;
    
    proc->cancel();
    return true;
}


bool wifiSyncDelete() {
    return wrkStop(WIFI_SYNC);
}
