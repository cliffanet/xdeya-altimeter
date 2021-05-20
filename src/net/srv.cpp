/*
    Server base function
*/

#include "srv.h"

#include <WiFi.h> // htonl


static WiFiClient cli;

// Заголовок данных при сетевой передаче
typedef struct __attribute__((__packed__)) {
    char    mgc;
    uint8_t cmd;
    int16_t len;
} phdr_t;

// заголовок принимаемой команды - на время, пока будем принимать данные
static phdr_t phr = { .mgc = '\0', .cmd = 0 };

bool srvConnect() {
    srvStop();
    return cli.connect("gpstat.dev.cliffa.net", 9971);
}

void srvStop() {
    cli.stop();
    phr = { mgc: '\0', cmd: 0 };
}

/* ------------------------------------------------------------------------------------------- *
 *  Блок из mode/netsync - для отображения ошибок при работе с сервером
 * ------------------------------------------------------------------------------------------- */
const char *last_err = NULL;
const char *srvErr() { return last_err; }

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
        return false;
    }
    return true;
}
