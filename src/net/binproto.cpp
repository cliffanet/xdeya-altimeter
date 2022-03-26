/*
    Server base function
*/

#include "binproto.h"

#include <string.h>
#include <arpa/inet.h> // ntohs()

#include "../log.h" // временно для отладки

#if defined(ARDUINO_ARCH_ESP32)

#include <pgmspace.h> // strlen_P / strcpy_P

#else

#define strlen_P      strlen
#define strcpy_P      strcpy

#endif

/* ------------------------------------------------------------------------------------------- *
 *  NetSocket
 * ------------------------------------------------------------------------------------------- */
size_t NetSocket::recv(size_t sz) {
    size_t avail = available();
    if (avail == 0)
        return 0;
    if (avail > sz)
        avail = sz;
    
    uint8_t data[avail];
    size_t sz1 = recv(data, avail);
    if (sz1 > sz)
        sz1 = sz;
    return sz1;
}

/* ------------------------------------------------------------------------------------------- *
 *  упаковка/распаковка данных для BinProto
 * ------------------------------------------------------------------------------------------- */
template <typename T>
static void _hton(uint8_t *buf, T src) {
    for (int i = sizeof(T)-1; i >= 0; i--) {
        buf[i] = src & 0xff;
        src = src >> 8;
    }
}
template <typename T>
static void _ntoh(T &dst, uint8_t *buf) {
    dst = 0;
    for (int i = sizeof(T)-1; i >= 0; i--)
        dst = (dst << 8) | buf[i];
}

static int _vnpack(uint8_t *buf, size_t sz, const char *pk, va_list va) {
    int len = 0;
    
    while (*pk && (sz > 0)) {
        bool stp = false;
        
        switch (*pk) {
            case ' ':
                *buf = 0;
                buf ++;
                sz --;
                len += 8;
                break;
            
            case 'c':
            case 'C':
                *buf = static_cast<uint8_t>(va_arg(va, int));
                buf ++;
                sz --;
                len ++;
                break;
            
            case 'n':
            case 'i':
            case 'x':
                if (sz < 2) {
                    stp = true;
                    break;
                }
                _hton(buf, static_cast<uint16_t>(va_arg(va, int)));
                buf += 2;
                sz -= 2;
                len += 2;
                break;
            
            case 'N':
            case 'I':
            case 'X':
                if (sz < 4) {
                    stp = true;
                    break;
                }
                _hton(buf, va_arg(va, uint32_t));
                buf += 4;
                sz -= 4;
                len += 4;
                break;

            case 'H':
                if (sz < 8) {
                    stp = true;
                    break;
                }
                _hton(buf, va_arg(va, uint64_t));
                buf += 8;
                sz -= 8;
                len += 8;
                break;
            
            case 'f':
                if (sz < 2) {
                    stp = true;
                    break;
                }
                _hton(buf, static_cast<int16_t>(va_arg(va, double)*100));
                buf += 2;
                sz -= 2;
                len += 2;
                break;
            
            case 'D':
                if (sz < 8) {
                    stp = true;
                    break;
                }
                _hton(buf, static_cast<int64_t>(va_arg(va, double)*0xffffffff));
                buf += 8;
                sz -= 8;
                len += 8;
                break;
        }
        
        if (stp)
            break;
        pk++;
    }
    
    CONSOLE("len: %d", len);
    
    return len;
}

/* ------------------------------------------------------------------------------------------- *
 *  BinProto
 * ------------------------------------------------------------------------------------------- */
BinProto::BinProto(char mgc, const elem_t *all, size_t count) :
    m_mgc(mgc)
{
    add(all, count);
}

void BinProto::add(const cmdkey_t &cmd, const char *pk_P) {
    auto &el = m_all[cmd];
    el.pk_P = pk_P;
}

void BinProto::add(const elem_t *all, size_t count) {
    if (all == NULL)
        return;
    
    if (count > 0)
        while (count > 0) {
            add(all->cmd, all->pk_P);
            all++;
            count--;
        }
    else
        while (all->cmd > 0) {
            add(all->cmd, all->pk_P);
            all++;
        }
}

void BinProto::del(const cmdkey_t &cmd) {
    m_all.erase(cmd);
}

int BinProto::vpack(uint8_t *buf, size_t sz, const cmdkey_t &cmd, va_list va) {
    if (sz < 4)
        return -1;
    
    auto it = m_all.find(cmd);
    if (it == m_all.end())
        return -1;
    
    buf[0] = m_mgc;
    buf[1] = cmd;
    
    char pk[ strlen_P(it->second.pk_P) + 1 ];
    strcpy_P(pk, it->second.pk_P);
    
    CONSOLE("cmd: %d", cmd);
    
    int len = _vnpack(buf+4, sz-4, pk, va);
    _hton(buf+2, static_cast<uint16_t>(len));
    
    return len + 4;
}

int BinProto::pack(uint8_t *buf, size_t sz, const cmdkey_t &cmd, ...) {
    va_list va;
    va_start(va, cmd);
    int len = vpack(buf+4, sz-4, cmd, va);
    va_end(va);
    
    return len;
}

bool BinProto::unpack(uint8_t *buf, size_t sz, const cmdkey_t &cmd, ...) {
    auto it = m_all.find(cmd);
    if (it == m_all.end())
        return false;
    
    return true;
}

/* ------------------------------------------------------------------------------------------- *
 *  BinProtoSend
 * ------------------------------------------------------------------------------------------- */
BinProtoSend::BinProtoSend(NetSocket * nsock, char mgc, const elem_t *all, size_t count) :
    BinProto(mgc, all, count)
{
    sock_set(nsock);
}

void BinProtoSend::sock_set(NetSocket * nsock) {
    if (nsock == NULL) {
        sock_clear();
        return;
    }
    
    m_nsock = nsock;
}

void BinProtoSend::sock_clear() {
    m_nsock = NULL;
}

bool BinProtoSend::send(const cmdkey_t &cmd, ...) {
    if ((m_nsock == NULL) || !m_nsock->connected())
        return false;
    
    uint8_t buf[1024], *data = buf;
    
    va_list va;
    va_start(va, cmd);
    int sz = vpack(buf, sizeof(buf), cmd, va);
    va_end(va);
    
    if (sz < 4)
        return false;
    
    uint8_t t = 5;
    
    while (sz > 0) {
        t --;
        auto sz1 = m_nsock->send(data, sz);
        
        if (sz1 < sz) {
            if (sz1 < 0)
                return false;
            if (!m_nsock->connected())
                return false;
            if (t < 1)
                return false;
            
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 50;
            
            if (select(1, NULL, NULL, NULL, &tv) < 0)
                return false;
        }
        
        sz -= sz1;
        data += sz1;
    }
    
    return true;
}


/*
BinProtoRecv::BinProto() : 
    BinProto(NULL)
{
    
}

BinProtoRecv::BinProto(NetSocket * nsock) :
    m_connected(false),
    m_waitcmd(0),
    m_waitsz(0),
    m_datasz(0)
{
    sock_set(nsock);
}

void BinProtoRecv::sock_set(NetSocket * nsock) {
    if (nsock == NULL) {
        sock_clear();
        return;
    }
    
    m_nsock = nsock;
}

void BinProtoRecv::sock_clear() {
    m_nsock = NULL;
    m_waitcmd = 0;
    m_waitsz = 0;
    m_datasz = 0;
}

void BinProtoRecv::recv_set(uint8_t cmd, recv_hnd_t hnd, uint16_t sz, uint16_t timeout, hnd_t on_timeout) {
    if (hnd == NULL) {
        recv_del(cmd);
        return;
    }
    
    auto &el = m_recv[cmd];
    el.hnd          = hnd;
    el.sz           = sz;
    el.timeout      = timeout;
    el.on_timeout   = on_timeout;
}

void BinProtoRecv::recv_del(uint8_t cmd) {
    m_recv.erase(cmd);
}

void BinProtoRecv::recv_clear() {
    m_recv.clear();
}

bool BinProtoRecv::process() {
    bool isrun = false;
    
    if ((m_nsock != NULL) &&
        (m_waitcmd == 0) &&
        (m_waitsz == 0) &&
        (m_nsock->available() >= sizeof(hdr_t))) {
        // Получение заголовка
        hdr_t p;
        size_t sz = m_nsock->recv(reinterpret_cast<uint8_t *>(&p), sizeof(hdr_t));
        if (sz != sizeof(hdr_t)) {
            onerror(ERR_RECV);
        }
        else
        if ((p.mgc != '#') || (p.cmd == 0)) {
            onerror(ERR_PROTO);
        }
        else {
            m_waitcmd = p.cmd;
            m_waitsz  = ntohs(p.sz);
            
            auto it = m_recv.find(m_waitcmd);
            m_datasz = 
                it == m_recv.end() ? 0 :
                m_waitsz < it->second.sz ? m_waitsz :
                it->second.sz;
        }
        isrun = true;
    }

    if ((m_nsock != NULL) &&
        (m_waitcmd > 0) &&
        (m_nsock->available() >= m_datasz)) {
        // Принимаем данные по ожидаемой команде
        auto it = m_recv.find(m_waitcmd);
        if (it == m_recv.end()) {
            onerror(ERR_RECVUNKNOWN);
        }
        else
        if (it->second.hnd != NULL) {
            if (m_datasz > 0) {
                uint8_t data[it->second.sz];
                size_t sz = m_nsock->recv(data, m_datasz);
                if (sz == m_datasz) {
                    if (sz < it->second.sz)
                        bzero(data + sz, it->second.sz - sz);
                    it->second.hnd(data);
                    m_waitsz -= sz;
                }
                else {
                    onerror(ERR_RECV);
                    if (sz > m_waitsz)
                        m_waitsz = 0;
                    else
                    if (sz > 0)
                        m_waitsz -= sz;
                }
            }
            else {
                it->second.hnd(NULL);
            }
        }
        
        m_waitcmd = 0;
        m_datasz = 0;
        isrun = true;
    }

    if ((m_nsock != NULL) &&
        (m_waitcmd == 0) &&
        (m_waitsz > 0) &&
        (m_nsock->available() >= 0)) {
        // Стравливание остатков данных команды, 
        // которых мы не ожидали.
        size_t sz = m_nsock->recv(m_waitsz);
        if (sz >= 0)
            m_waitsz -= sz;
        else
            onerror(ERR_RECV);
        isrun = true;
    }
    
    if (m_connected && ((m_nsock == NULL) || !m_nsock->connected())) {
        ondisconnect();
        m_connected = false;
        m_waitcmd = 0;
        m_waitsz = 0;
        m_datasz = 0;
    }
    else
    if (!m_connected && (m_nsock != NULL) && m_nsock->connected()) {
        onconnect();
        m_connected = true;
    }
    
    return isrun;
}
*/
