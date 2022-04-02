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


#define ERR(st)             err(err ## st)


WorkerWiFiSync::WorkerWiFiSync(const char *ssid, const char *pass) :
    m_sock(NULL),
    m_proo(NULL),
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
    if (m_proo != NULL)
        delete m_proo;
    if (m_proi != NULL)
        delete m_proi;
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
    if ((m_sock == NULL) || (m_proo != NULL))
        return;
    
    m_proo = new BinProtoSend(m_sock, '%');
    // auth
    m_proo->add( 0x01, PSTR("N") );
    
    // pntcs
    m_proo->add( 0x23, PSTR("X") );
    // pnt
    m_proo->add( 0x24, PSTR("CCDD") );
    // logbookbeg
    m_proo->add( 0x31, PSTR("") );
    // logbook
    m_proo->add( 0x32, PSTR("NNT") );
    // logbookend
    m_proo->add( 0x33, PSTR("XN") );
    // trackbeg
    m_proo->add( 0x34, PSTR("        NNT") );
    // track
    m_proo->add( 0x35, PSTR("NnaaiiNNNiiNNC nNNNNNN") );
    // trackend
    m_proo->add( 0x36, PSTR("H") );
    // datafin
    m_proo->add( 0x3f, PSTR("XXCCCaC   a32") );
    
    m_proi = new BinProtoRecv(m_sock, '#');
    // rejoin
    m_proi->add( 0x10, PSTR("N") );
    // accept
    m_proi->add( 0x20, PSTR("XXXXNH") );
}

void WorkerWiFiSync::end() {
    if (m_proo != NULL) {
        CONSOLE("delete m_proo");
        delete m_proo;
        m_proo = NULL;
    }
    if (m_proi != NULL) {
        CONSOLE("delete m_proi");
        delete m_proi;
        m_proi = NULL;
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
    
    if (isrun() && (m_proi != NULL))
        switch (m_proi->process()) {
            case BinProtoRecv::STATE_OK:
                settimer(100);
                return STATE_RUN;
            
            case BinProtoRecv::STATE_CMD:
                {
                    uint8_t cmd;
                    if (!m_proi->recv(cmd, d))
                        RETURN_ERR(RecvData);
                    if (!recvdata(cmd))
                        RETURN_ERR(RcvCmdUnknown);
                }
                return STATE_RUN;
                
            case BinProtoRecv::STATE_ERROR:
                RETURN_ERR(RecvData);
        }
    
    switch (op()) {
        case opExit:
            return STATE_END;
        
        case opClose:
            end();
            if (m_st == stRun)
                m_st = stUserCancel;
            next(0);
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
            
                if (!m_proo->send(0x01, wjoin.authid()))
                    RETURN_ERR(SendData);
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
    }

#undef RETURN_ERR
#undef RETURN_NEXT
#undef SEND
    
    return STATE_WAIT;
}

bool WorkerWiFiSync::recvdata(uint8_t cmd) {
    
#define RETURN_NEXT(tmr)        { next(tmr); return true; }

    switch (op()) {
    
        case opWaitAuth:
            switch (cmd) {
                case 0x10: // rejoin
                    //RET_NEXT(SENDCONFIG, 10);
                    return true;
                case 0x20: // accept
                    CONSOLE("recv ckstrack: %04x %04x %08x", d.acc.ckstrack.csa, d.acc.ckstrack.csb, d.acc.ckstrack.sz);
                    RETURN_NEXT(10);
            }
            return false;
    }
    
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
