/*
    WiFi functions
*/

#include "wifisync.h"
#include "../log.h"
#include "../core/workerloc.h"
#include "wifi.h"
#include "binproto.h"
#include "netsync.h"
#include "../cfg/webjoin.h"
#include "../cfg/point.h"


#define ERR(st)             err(err ## st)
#define SOCK                BinProtoSend(m_sock, '%')
#define SEND(cmd, ...)      if (!SOCK.send(cmd, ##__VA_ARGS__)) RETURN_ERR(SendData);


WorkerWiFiSync::WorkerWiFiSync(const char *ssid, const char *pass) :
    m_sock(NULL),
    m_pro(NULL),
    m_st(stRun)
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


/* ------------------------------------------------------------------------------------------- *
 *  Инициализация
 * ------------------------------------------------------------------------------------------- */
void wifiSyncBegin(const char *ssid, const char *pass) {
    if (wrkExists(WORKER_WIFI_SYNC))
        return;
    wrkAdd(WORKER_WIFI_SYNC, new WorkerWiFiSync(ssid, pass));
    CONSOLE("begin");
}

WorkerWiFiSync * wifiSyncProc() {
    return reinterpret_cast<WorkerWiFiSync *>(wrkGet(WORKER_WIFI_SYNC));
}
