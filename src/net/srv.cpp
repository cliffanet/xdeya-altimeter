/*
    Server base function
*/

#include "srv.h"

#include <lwip/def.h>      // htonl
#include <pgmspace.h>       // PSTR
#include <WiFiClient.h>
#include "../log.h"

#include "lwip/sockets.h"       // host -> ip
#include "lwip/sys.h"
#include "lwip/netdb.h"

static WiFiClient cli;

// Заголовок данных при сетевой передаче
typedef struct __attribute__((__packed__)) {
    char    mgc;
    uint8_t cmd;
    int16_t len;
} phdr_t;

// заголовок принимаемой команды - на время, пока будем принимать данные
static phdr_t phr = { .mgc = '\0', .cmd = 0 };

#define SRV_HOST    "gpstat.dev.cliffa.net"
#define SRV_PORT    9971

/* ------------------------------------------------------------------------------------------- *
 *  Блок из mode/netsync - для отображения ошибок при работе с сервером
 * ------------------------------------------------------------------------------------------- */
const char *last_err = NULL;
const char *srvErr() { return last_err; }

/* ------------------------------------------------------------------------------------------- *
 *  соединение с сервером
 * ------------------------------------------------------------------------------------------- */
bool srvConnect() {
    srvStop();
    
    char host[64];
    strcpy_P(host, PSTR(SRV_HOST));
    
    struct addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *res;
    int err = getaddrinfo(host, NULL, &hints, &res);
    if(err != 0 || res == NULL) {
        last_err = PSTR("DNS lookup failed");
        return false;
    }
    
    IPAddress ip(reinterpret_cast<struct sockaddr_in *>(res->ai_addr)->sin_addr.s_addr);
    CONSOLE("srvConnect host %s -> ip %d.%d.%d.%d", host, ip[0], ip[1], ip[2], ip[3]);
    
    return cli.connect(ip, SRV_PORT);
}

void srvStop() {
    cli.stop();
    phr = { mgc: '\0', cmd: 0 };
}

/* ------------------------------------------------------------------------------------------- *
 *  чтение инфы от сервера, возвращает true, если есть инфа
 * ------------------------------------------------------------------------------------------- */
static bool srvWait(size_t sz) {
    last_err = NULL;
    if (cli.available() >= sz)
        return true;
    
    if (!cli.connected())
        last_err = PSTR("server connect lost");
    
    return false;
}

static bool srvReadData(uint8_t *data, uint16_t sz) {
    last_err = NULL;
    if (sz == 0)
        return true;
    
    size_t sz1 = cli.read(data, sz);
    if (sz1 != sz) {
        last_err = PSTR("fail read from server");
        return false;
    }
    
    return true;
}

static bool srvWaitHdr(phdr_t &p) {
    if (!srvWait(sizeof(phdr_t)))
        return false;
    
    if (!srvReadData(reinterpret_cast<uint8_t *>(&p), sizeof(phdr_t)))
        return false;
    if ((p.mgc != '#') || (p.cmd == 0)) {
        last_err = PSTR("recv proto fail");
        return false;
    }
    
    p.len = ntohs(p.len);
    
    return true;
}

bool srvRecv(uint8_t &cmd, uint8_t *data, uint16_t sz) {
    if (data == NULL) sz = 0;
    
    // ожидание и чтение заголовка
    if ((phr.mgc == '\0') && (phr.cmd == 0)) {
        if (!srvWaitHdr(phr))
            return false;
        if (phr.len < sz) // стираем хвост data, который не будет переписан принимаемыми данными,
                        // Удобно, если после изменения протокола, данные по требуемой команде
                        // от сервера будут более короткими.
                        // Но так же это и лишняя процедура, если буфер подаётся с запасом
            bzero(data+phr.len, sz-phr.len);
    }
    
    cmd = phr.cmd;
    
    // Ожидание данных
    if ((phr.len > 0) && !srvWait(phr.len))
        return false;
    
    // чтение данных
    if (!srvReadData(data, phr.len <= sz ? phr.len : sz))
        return false;
    
    // И дочитываем буфер, если это требуется
    if (phr.len > sz) {
        uint16_t sz1 = phr.len - sz;
        uint8_t buf[sz1];
        if (!srvReadData(buf, sz1))
            return false;
    }
    
    // Обозначаем, что дальше надо принимать заголовок
    phr.mgc = '\0';
    phr.cmd = 0;
    
    return true;
}

/* ------------------------------------------------------------------------------------------- *
 *  отправка на сервер
 * ------------------------------------------------------------------------------------------- */
bool srvSend(uint8_t cmd, const uint8_t *data, uint16_t sz) {
    if (!cli.connected()) {
        last_err = PSTR("server connect lost");
        CONSOLE("srvSend on server connect lost");
        return false;
    }
    
    uint8_t d[4 + sz] = { '%', cmd };
    uint16_t sz1 = htons(sz);
    memcpy(d+2, &sz1, sizeof(sz1));
    if (sz > 0)
        memcpy(d+4, data, sz);
    
    auto sz2 = cli.write(d, 4 + sz);
    if (sz2 != (4 + sz)) {
        last_err = PSTR("server send fail");
        CONSOLE("srvSend FAIL: sended=%d; sz=%d", sz2, sz);
        return false;
    }
    return true;
}
