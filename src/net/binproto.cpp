/*
    Server base function
*/

#include "binproto.h"

#include <string.h>
//#include <arpa/inet.h> // ntohs()
#include <math.h>

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
static void _ntoh(T &dst, const uint8_t *buf) {
    dst = 0;
    for (int i = 0; i < sizeof(T); i++)
        dst = (dst << 8) | buf[i];
}

static void _fton(uint8_t *buf, float val) {
    _hton(buf, static_cast<uint16_t>(round(val*100)));
}
static float _ntof(const uint8_t *buf) {
    uint16_t val;
    _ntoh(val, buf);
    return static_cast<float>(val) / 100;
}

static void _dton(uint8_t *buf, double val) {
    int32_t i = static_cast<int32_t>(val);
    
    val -= i;
    if (val < 0) val = val*(-1);
    
    uint32_t d = static_cast<uint32_t>(val * 0xffffffff);
    
    _hton(buf, i);
    _hton(buf+4, d);
}

static double _ntod(const uint8_t *buf) {
    int32_t i;
    uint32_t d;
    
    _ntoh(i, buf);
    _ntoh(d, buf+4);
    
    double val = static_cast<double>(i);
    
    if (val < 0)
        val -= static_cast<double>(d) / 0xffffffff;
    else
        val += static_cast<double>(d) / 0xffffffff;
    
    return val;
}

static int _vnpack(uint8_t *buf, size_t sz, const char *pk, va_list va) {
    int len = 0;
    
    while (*pk && (sz > 0)) {
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
                if (sz < 2)
                    return len;
                _hton(buf, static_cast<uint16_t>(va_arg(va, int)));
                buf += 2;
                sz -= 2;
                len += 2;
                break;
            
            case 'N':
            case 'I':
            case 'X':
                if (sz < 4)
                    return len;
                _hton(buf, va_arg(va, uint32_t));
                buf += 4;
                sz -= 4;
                len += 4;
                break;

            case 'H':
                if (sz < 8)
                    return len;
                _hton(buf, va_arg(va, uint64_t));
                buf += 8;
                sz -= 8;
                len += 8;
                break;
            
            case 'f':
                if (sz < 2)
                    return len;
                _fton(buf, va_arg(va, double));
                buf += 2;
                sz -= 2;
                len += 2;
                break;
            
            case 'D':
                if (sz < 8)
                    return len;
                _dton(buf, va_arg(va, double));
                buf += 8;
                sz -= 8;
                len += 8;
                break;
        }
        
        pk++;
    }
    
    return len;
}

static bool _unpack(const uint8_t *src, size_t srcsz, const char *pk, uint8_t *dst, size_t dstsz) {

#define CHKSZ(ssz, T) \
        if ((srcsz < ssz) || (dstsz < sizeof(T))) { \
            nxt = false; \
            break; \
        }

#define NXTSZ(ssz, T) \
        src += ssz; \
        srcsz -= ssz; \
        dst += sizeof(T); \
        dstsz -= sizeof(T);

#define DST(T)  *(reinterpret_cast<T*>(dst))

    bool nxt = true;
    while (*pk && nxt) {
        switch (*pk) {
            case ' ':
                if (srcsz < 1) {
                    nxt = false;
                    break;
                }
                src ++;
                srcsz --;
                break;
            
            case 'c':
            case 'C':
                CHKSZ(1, uint8_t)
                *dst = *src;
                NXTSZ(1, uint8_t)
                break;
            
            case 'n':
            case 'i':
            case 'x':
                CHKSZ(2, uint16_t)
                _ntoh(DST(uint16_t), src);
                NXTSZ(2, uint16_t)
                break;
            
            case 'N':
            case 'I':
            case 'X':
                CHKSZ(4, uint32_t)
                _ntoh(DST(uint32_t), src);
                NXTSZ(4, uint32_t)
                break;

            case 'H':
                CHKSZ(8, uint64_t)
                _ntoh(DST(uint64_t), src);
                NXTSZ(8, uint64_t)
                break;
            
            case 'f':
                CHKSZ(2, float)
                DST(float) = _ntof(src);
                NXTSZ(2, float)
                break;
            
            case 'D':
                CHKSZ(8, double)
                DST(double) = _ntod(src);
                NXTSZ(8, double)
                break;
            
            default:
                return false;
        }
        
        pk++;
    }

#undef CHKSZ
#undef DST
#undef NXTSZ
    
    return true;
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
    el = pk_P;
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

BinProto::item_t BinProto::pk_P(const cmdkey_t &cmd) const {
    auto it = m_all.find(cmd);
    if (it == m_all.end())
        return NULL;
    
    return it->second;
}

void BinProto::hdrpack(uint8_t *buf, const cmdkey_t &cmd, uint16_t sz) {
    buf[0] = m_mgc;
    buf[1] = cmd;
    _hton(buf+2, sz);
}

bool BinProto::hdrunpack(const uint8_t *buf, cmdkey_t &cmd, uint16_t &sz) {
    if ((buf[0] != m_mgc) || (buf[1] == 0))
        return false;
    cmd = buf[1];
    _ntoh(sz, buf+2);
    
    return true;
}

int BinProto::vpack(uint8_t *buf, size_t sz, const cmdkey_t &cmd, va_list va) {
    if (sz < hdrsz())
        return -1;
    
    auto pkP = pk_P(cmd);
    if (pkP == NULL)
        return -1;
    
    char pk[ strlen_P(pkP) + 1 ];
    strcpy_P(pk, pkP);
    
    int len = _vnpack(buf+hdrsz(), sz-hdrsz(), pk, va);
    if (len < 0)
        return -1;
    
    hdrpack(buf, cmd, static_cast<uint16_t>(len));
    
    return len + hdrsz();
}

int BinProto::pack(uint8_t *buf, size_t sz, const cmdkey_t &cmd, ...) {
    va_list va;
    va_start(va, cmd);
    int len = vpack(buf, sz, cmd, va);
    va_end(va);
    
    return len;
}

bool BinProto::unpack(const uint8_t *buf, size_t bufsz, cmdkey_t &cmd, uint8_t *dst, size_t dstsz) {
    if (bufsz < hdrsz())
        return false;
    
    uint16_t len;
    if (!hdrunpack(buf, cmd, len))
        return false;
    
    auto pkP = pk_P(cmd);
    if (pkP == NULL)
        return false;
    
    char pk[ strlen_P(pkP) + 1 ];
    strcpy_P(pk, pkP);
    
    return _unpack(buf+hdrsz(), len < bufsz-hdrsz() ? len : bufsz-hdrsz(), pk, dst, dstsz);
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
    
    if (sz < hdrsz())
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

/* ------------------------------------------------------------------------------------------- *
 *  BinProtoRecv
 * ------------------------------------------------------------------------------------------- */
BinProtoRecv::BinProtoRecv(NetSocket * nsock, char mgc, const elem_t *all, size_t count) :
    BinProto(mgc, all, count),
    m_err(ERR_NONE),
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

bool BinProtoRecv::recv(cmdkey_t &cmd, uint8_t *data, size_t sz) {
    // Принимаем данные по ожидаемой команде
    if ((m_nsock == NULL) ||
        (m_waitcmd == 0) &&
        (m_nsock->available() < m_datasz))
        return false;
    
    size_t rsz = sz < m_datasz ? sz : m_datasz;
    bool ok = false;
    if (rsz > 0) {
        uint8_t src[rsz + hdrsz()];
        size_t sz1 = m_nsock->recv(src + hdrsz(), rsz);
        if (sz1 == rsz) {
            hdrpack(src, m_waitcmd, m_waitsz);
            
            ok = unpack(src, rsz + hdrsz(), cmd, data, sz);
            
            m_waitsz -= sz1;
        }
        else {
            if (sz1 > m_waitsz)
                m_waitsz = 0;
            else
            if (sz1 > 0)
                m_waitsz -= sz1;
        }
    }
    
    m_waitcmd = 0;
    m_datasz = 0;
    return ok;
}

BinProtoRecv::state_t BinProtoRecv::process() {
    bool isrun = false;
    m_err = ERR_NONE;
    
    if ((m_nsock != NULL) &&
        (m_waitcmd == 0) &&
        (m_waitsz == 0) &&
        (m_nsock->available() >= hdrsz())) {
        // Получение заголовка
        uint8_t hdr[hdrsz()];
        size_t sz = m_nsock->recv(hdr, hdrsz());
        if (sz != hdrsz()) {
            m_err = ERR_RECV;
            return STATE_ERROR;
        }
        else
        if (hdrunpack(hdr, m_waitcmd, m_waitsz)) {
            m_datasz = m_waitsz < 1024 ? m_waitsz : 1024;
        }
        else {
            m_err = ERR_PROTO;
            return STATE_ERROR;
        }
        isrun = true;
    }
        
    if ((m_nsock != NULL) &&
        (m_waitcmd > 0) &&
        (m_nsock->available() >= m_datasz))
        // Мы получили нужный объём данных
        return STATE_CMD;

    if ((m_nsock != NULL) &&
        (m_waitcmd == 0) &&
        (m_waitsz > 0) &&
        (m_nsock->available() >= 0)) {
        // Стравливание остатков данных команды, 
        // которых мы не ожидали.
        size_t sz = m_nsock->recv(m_waitsz);
        if (sz >= 0)
            m_waitsz -= sz;
        else {
            m_err = ERR_RECV;
            return STATE_ERROR;
        }
        isrun = true;
    }
    
    if (isrun)
        return STATE_OK;
    
    if ((m_nsock == NULL) || !m_nsock->connected()) {
        m_err = ERR_DISCONECT;
        m_waitcmd = 0;
        m_waitsz = 0;
        m_datasz = 0;
        return STATE_ERROR;
    }
    
    return STATE_NORECV;
}
