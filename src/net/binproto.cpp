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

static int _pack(uint8_t *dst, size_t dstsz, const char *pk, const uint8_t *src, size_t srcsz) {
    int len = 0;

#define CHKSZ_(dsz, ssz) \
        if ((dstsz < dsz) || (srcsz < ssz)) { \
            nxt = false; \
            break; \
        }
#define CHKSZ(dsz, T) CHKSZ_(dsz, sizeof(T))

#define NXTSZ_(dsz, ssz) \
        len += dsz; \
        dst += dsz; \
        dstsz -= dsz; \
        src += ssz; \
        srcsz -= ssz;
#define NXTSZ(dsz, T) NXTSZ_(dsz, sizeof(T))

#define SRC(T)  *(reinterpret_cast<const T*>(src))

    bool nxt = true;
    while (*pk && nxt) {
        switch (*pk) {
            case ' ':
                if (dstsz < 1) {
                    nxt = false;
                    break;
                }
                *dst = 0;
                dst ++;
                dstsz --;
                len ++;
                break;
            
            case 'b':
                CHKSZ(1, bool)
                *dst = SRC(bool) ? 1 : 0;
                NXTSZ(1, bool)
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
                _hton(dst, SRC(uint16_t));
                NXTSZ(2, uint16_t)
                break;
            
            case 'N':
            case 'I':
            case 'X':
                CHKSZ(4, uint32_t)
                _hton(dst, SRC(uint32_t));
                NXTSZ(4, uint32_t)
                break;

            case 'H':
                CHKSZ(8, uint64_t)
                _hton(dst, SRC(uint64_t));
                NXTSZ(8, uint64_t)
                break;
            
            case 'f':
                CHKSZ(2, float)
                _fton(dst, SRC(float));
                NXTSZ(2, float)
                break;
            
            case 'D':
                CHKSZ(8, double)
                _dton(dst, SRC(double));
                NXTSZ(8, double)
                break;
            
            case 'T':
                CHKSZ_(8, 8)
                _hton(dst, SRC(uint16_t));
                memcpy(dst+2, src+2, 6);
                NXTSZ_(8, 8)
                break;
            
            case 'a':
                if ((pk[1] >= '0') && (pk[1] <= '9')) {
                    int l = 0;
                    while ((pk[1] >= '0') && (pk[1] <= '9')) {
                        l = l*10 + (pk[1] - '0');
                        pk++;
                    }
                    CHKSZ_(l+1, l)
                    memcpy(dst, src, l);
                    dst[l] = '\0';
                    NXTSZ_(l+1, l)
                }
                else {
                    CHKSZ(1, char)
                    *dst = *src;
                    NXTSZ(1, char)
                }
                break;
            
            default:
                return -1;
        }
        
        pk++;
    }

#undef CHKSZ_
#undef CHKSZ
#undef NXTSZ_
#undef NXTSZ
#undef SRC
    
    return len;
}

static bool _unpack(uint8_t *dst, size_t dstsz, const char *pk, const uint8_t *src, size_t srcsz) {

#define CHKSZ_(dsz, ssz) \
        if ((dstsz < dsz) || (srcsz < ssz)) { \
            nxt = false; \
            break; \
        }
#define CHKSZ(T, ssz) CHKSZ_(sizeof(T), ssz)

#define NXTSZ_(dsz, ssz) \
        dst += dsz; \
        dstsz -= dsz; \
        src += ssz; \
        srcsz -= ssz;
#define NXTSZ(T, ssz) NXTSZ_(sizeof(T), ssz)

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
            
            case 'b':
                CHKSZ(bool, 1)
                DST(bool) = *src > 0;
                NXTSZ(bool, 1)
                break;
            
            case 'c':
            case 'C':
                CHKSZ(uint8_t, 1)
                *dst = *src;
                NXTSZ(uint8_t, 1)
                break;
            
            case 'n':
            case 'i':
            case 'x':
                CHKSZ(uint16_t, 2)
                _ntoh(DST(uint16_t), src);
                NXTSZ(uint16_t, 2)
                break;
            
            case 'N':
            case 'I':
            case 'X':
                CHKSZ(uint32_t, 4)
                _ntoh(DST(uint32_t), src);
                NXTSZ(uint32_t, 4)
                break;

            case 'H':
                CHKSZ(uint64_t, 8)
                _ntoh(DST(uint64_t), src);
                NXTSZ(uint64_t, 8)
                break;
            
            case 'f':
                CHKSZ(float, 2)
                DST(float) = _ntof(src);
                NXTSZ(float, 2)
                break;
            
            case 'D':
                CHKSZ(double, 8)
                DST(double) = _ntod(src);
                NXTSZ(double, 8)
                break;
            
            case 'T':
                CHKSZ_(8, 8)
                _ntoh(DST(uint16_t), src);
                memcpy(dst+2, src+2, 6);
                NXTSZ_(8, 8)
                break;
            
            case 'a':
                if ((pk[1] >= '0') && (pk[1] <= '9')) {
                    int l = 0;
                    while ((pk[1] >= '0') && (pk[1] <= '9')) {
                        l = l*10 + (pk[1] - '0');
                        pk++;
                    }
                    CHKSZ_(l, l+1)
                    memcpy(dst, src, l);
                    NXTSZ_(l, l+1)
                }
                else {
                    CHKSZ(char, 1)
                    *dst = *src;
                    NXTSZ(char, 1)
                }
                break;
            
            default:
                return false;
        }
        
        pk++;
    }

#undef CHKSZ_
#undef CHKSZ
#undef NXTSZ_
#undef NXTSZ
#undef DST
    
    return true;
}

/* ------------------------------------------------------------------------------------------- *
 *  BinProto
 * ------------------------------------------------------------------------------------------- */
BinProto::BinProto(char mgc) :
    m_mgc(mgc)
{
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

int BinProto::pack(uint8_t *buf, size_t bufsz, const cmdkey_t &cmd, const char *pk_P, const uint8_t *src, size_t srcsz) {
    if (bufsz < hdrsz())
        return -1;
    
    int len;
        
    if (srcsz > 0) {
        if (pk_P == NULL)
            return -1;
        
        char pk[ strlen_P(pk_P) + 1 ];
        strcpy_P(pk, pk_P);
        
        len = _pack(buf+hdrsz(), bufsz-hdrsz(), pk, src, srcsz);
        if (len < 0)
            return -1;
    }
    else
        len = 0;
    
    CONSOLE("len: %d", len);
    
    hdrpack(buf, cmd, static_cast<uint16_t>(len));
    
    return len + hdrsz();
}

bool BinProto::unpack(cmdkey_t &cmd, uint8_t *dst, size_t dstsz, const char *pk_P, const uint8_t *buf, size_t bufsz) {
    if ((bufsz < hdrsz()) || (pk_P == NULL))
        return false;
    
    uint16_t len;
    if (!hdrunpack(buf, cmd, len))
        return false;
    
    char pk[ strlen_P(pk_P) + 1 ];
    strcpy_P(pk, pk_P);
    
    return _unpack(dst, dstsz, pk, buf+hdrsz(), len < bufsz-hdrsz() ? len : bufsz-hdrsz());
}

/* ------------------------------------------------------------------------------------------- *
 *  BinProtoSend
 * ------------------------------------------------------------------------------------------- */
BinProtoSend::BinProtoSend(NetSocket * nsock, char mgc) :
    BinProto(mgc)
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

bool BinProtoSend::send(const cmdkey_t &cmd, const char *pk_P, const uint8_t *data, size_t sz) {
    if ((m_nsock == NULL) || !m_nsock->connected())
        return false;
    
    uint8_t buf[1024], *b = buf;
    
    int len = pack(buf, sizeof(buf), cmd, pk_P, data, sz);
    
    if (len < hdrsz())
        return false;
    
    uint8_t t = 5;
    
    while (len > 0) {
        t --;
        auto sz1 = m_nsock->send(b, len);
        
        if (sz1 < len) {
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
        
        if (sz1 > len)
            sz1 = len;
        
        len -= sz1;
        b += sz1;
    }
    
    return true;
}

/* ------------------------------------------------------------------------------------------- *
 *  BinProtoRecv
 * ------------------------------------------------------------------------------------------- */
BinProtoRecv::BinProtoRecv(NetSocket * nsock, char mgc, const elem_t *all, size_t count) :
    BinProtoSend(nsock, mgc),
    m_err(ERR_NONE),
    m_waitcmd(0),
    m_waitsz(0),
    m_datasz(0)
{
    add(all, count);
}

void BinProtoRecv::add(const cmdkey_t &cmd, const char *pk_P) {
    m_all[cmd] = pk_P;
}

void BinProtoRecv::add(const elem_t *all, size_t count) {
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

void BinProtoRecv::del(const cmdkey_t &cmd) {
    m_all.erase(cmd);
}

BinProtoRecv::item_t BinProtoRecv::pk_P(const cmdkey_t &cmd) const {
    auto it = m_all.find(cmd);
    if (it == m_all.end())
        return NULL;
    
    return it->second;
}

bool BinProtoRecv::recv(cmdkey_t &cmd, uint8_t *data, size_t sz) {
    // Принимаем данные по ожидаемой команде
    if ((m_nsock == NULL) ||
        (m_waitcmd == 0) &&
        (m_nsock->available() < m_datasz))
        return false;
    
    auto pkP = pk_P(m_waitcmd);
    if (pkP == NULL)
        return false;
    
    size_t rsz = sz < m_datasz ? sz : m_datasz;
    bool ok = false;
    if (rsz > 0) {
        uint8_t src[rsz + hdrsz()];
        size_t sz1 = m_nsock->recv(src + hdrsz(), rsz);
        if (sz1 == rsz) {
            hdrpack(src, m_waitcmd, m_waitsz);
            
            ok = unpack(cmd, data, sz, pkP, src, rsz + hdrsz());
            
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