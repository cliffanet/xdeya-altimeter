/*
    Server base function
*/

#include "binproto.h"

#include <string.h>
#include <math.h>
#include <sys/select.h>

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

int BinProto::datapack(uint8_t *dst, size_t dstsz, const char *pk, const uint8_t *src, size_t srcsz) {
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
                    // этот модификатор, если указаны цифры (даже если только 1),
                    // рассчитывает, что в локальной структуре данных (в src)
                    // в этом месте находится char[N+1], где N - указанное число
                    // Предполагается, что тут строка, но копироваться будут все N байт
                    // Возможно, при отправке потребуется все байты после первого '\0' - так же обнулить
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
                    CHKSZ(1, uint8_t)
                    *dst = *src;
                    NXTSZ(1, uint8_t)
                }
                break;
            
            case 's': {
                // этот модификатор рассчитывает, что в локальной структуре данных (в src)
                // в этом месте находится char[BINPROTO_STRSZ]
                uint8_t l = 0;
                const uint8_t *s = src;
                while ((*s != '\0') && (l < srcsz) && (l < 255) && (l < (BINPROTO_STRSZ-1))) {
                    s++;
                    l++;
                }
                CHKSZ_(l+1, BINPROTO_STRSZ)
                *dst = l;
                if (l > 0)
                    memcpy(dst+1, src, l);
                NXTSZ_(l+1, BINPROTO_STRSZ)
                break;
            }
            
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

bool BinProto::dataunpack(uint8_t *dst, size_t dstsz, const char *pk, const uint8_t *src, size_t srcsz) {

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
                    // этот модификатор, если указаны цифры (даже если только 1),
                    // рассчитывает, что в локальной структуре данных (в dst)
                    // в этом месте находится char[N+1], где N - указанное число
                    // Предполагается, что тут строка, но копироваться будут все N байт
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
                    CHKSZ(uint8_t, 1)
                    *dst = *src;
                    NXTSZ(uint8_t, 1)
                }
                break;
            
            case 's': {
                // этот модификатор рассчитывает, что в локальной структуре данных (в dst)
                // в этом месте находится char[BINPROTO_STRSZ]
                CHKSZ_(BINPROTO_STRSZ, 1)
                int l = *src, l1 = *src;
                if (l1 > 0) {
                    CHKSZ_(BINPROTO_STRSZ, l+1)
                    if (l1 >= BINPROTO_STRSZ)
                        l1 = BINPROTO_STRSZ-1;
                    memcpy(dst, src+1, l1);
                }
                dst[l1] = '\0';
                NXTSZ_(BINPROTO_STRSZ, l+1)
                break;
            }
            
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
BinProto::BinProto(NetSocket * nsock, char mgcsnd, char mgcrcv) :
    m_mgcsnd(mgcsnd),
    m_mgcrcv(mgcrcv),
    m_rcvstate(RCV_WAITCMD),
    m_rcvcmd(0),
    m_rcvsz(0)
{
    sock_set(nsock);
}

// header
void BinProto::hdrpack(uint8_t *buf, const cmdkey_t &cmd, uint16_t sz) {
    buf[0] = m_mgcsnd;
    buf[1] = cmd;
    _hton(buf+2, sz);
}

bool BinProto::hdrunpack(const uint8_t *buf, cmdkey_t &cmd, uint16_t &sz) {
    if ((buf[0] != m_mgcrcv) || (buf[1] == 0))
        return false;
    cmd = buf[1];
    _ntoh(sz, buf+2);
    
    return true;
}

// socket
void BinProto::sock_set(NetSocket * nsock) {
    if (nsock == NULL) {
        sock_clear();
        return;
    }
    
    m_nsock = nsock;
}

void BinProto::sock_clear() {
    m_nsock = NULL;
}

// send
bool BinProto::send(const cmdkey_t &cmd, const char *pk_P, const uint8_t *data, size_t sz) {
    if ((m_nsock == NULL) || !m_nsock->connected())
        return false;
    
    uint8_t buf[1024], *b = buf;
    
    int len;
        
    if (sz > 0) {
        if (pk_P == NULL)
            return false;
        
        char pk[ strlen_P(pk_P) + 1 ];
        strcpy_P(pk, pk_P);
        
        len = datapack(buf+hdrsz(), sizeof(buf)-hdrsz(), pk, data, sz);
        if (len < 0)
            return false;
    }
    else
        len = 0;
    
    CONSOLE("cmd: 0x%02x; len: %d", cmd, len);
    
    hdrpack(buf, cmd, static_cast<uint16_t>(len));
    len += hdrsz();
    
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

// recv
void BinProto::rcvclear() {
    m_rcvstate = RCV_WAITCMD;
    m_rcvcmd = 0;
    m_rcvsz = 0;
}

BinProto::rcvst_t BinProto::rcvprocess() {
    if (m_rcvstate <= RCV_DISCONNECTED) {
        if ((m_nsock == NULL) || !m_nsock->connected()) {
            if (m_rcvstate < RCV_DISCONNECTED)
                m_rcvstate = RCV_DISCONNECTED;
            return m_rcvstate;
        }
        
        rcvclear();
    }

    if (m_rcvstate == RCV_NULL) {
        if (m_nsock == NULL) {
            m_rcvstate = RCV_DISCONNECTED;
            return m_rcvstate;
        }
        if (m_nsock->available() <= 0) {
            if (!m_nsock->connected())
                m_rcvstate = RCV_DISCONNECTED;
            return m_rcvstate;
        }
        
        // Стравливание остатков данных команды, 
        // которых мы не ожидали.
        size_t sz = m_nsock->recv(m_rcvsz);
        if ((sz < 0) || (sz > m_rcvsz)) {
            m_rcvstate = RCV_ERROR;
            return m_rcvstate;
        }
        
        m_rcvsz -= sz;
        if (m_rcvsz > 0)
            return m_rcvstate;
        
        // Стравили всё, что нужно, теперь ждём следующую команду
        rcvclear();
    }
    
    if (m_rcvstate == RCV_WAITCMD) {
        if (m_nsock == NULL) {
            m_rcvstate = RCV_DISCONNECTED;
            return m_rcvstate;
        }
        if (m_nsock->available() < hdrsz()) {
            if (!m_nsock->connected())
                m_rcvstate = RCV_DISCONNECTED;
            return m_rcvstate;
        }
        
        // Получение заголовка
        uint8_t hdr[hdrsz()];
        size_t sz = m_nsock->recv(hdr, hdrsz());
        if (sz != hdrsz()) {
            m_rcvstate = RCV_ERROR;
            return m_rcvstate;
        }
        if (!hdrunpack(hdr, m_rcvcmd, m_rcvsz)) {
            m_rcvstate = RCV_ERROR;
            return m_rcvstate;
        }
        
        m_rcvstate = RCV_DATA;
    }
    
    if (m_rcvstate == RCV_DATA) {
        if (m_nsock == NULL) {
            m_rcvstate = RCV_DISCONNECTED;
            return m_rcvstate;
        }
        if (m_nsock->available() < m_rcvsz) {
            CONSOLE("wait data");
            if (!m_nsock->connected())
                m_rcvstate = RCV_DISCONNECTED;
            return m_rcvstate;
        }
        
        // Мы получили нужный объём данных
        m_rcvstate = RCV_COMPLETE;
    }
    
    return m_rcvstate;
}

bool BinProto::rcvdata(const char *pk_P, uint8_t *data, size_t sz) {
    if ((m_rcvstate != RCV_DATA) && (m_rcvstate != RCV_COMPLETE))
        return false;
    // Все модификаторы предполагают, что наши локальные данные (data) -
    // либо такие же по размеру, либо больше, поэтому передаваемого sz
    // должно хватить, чтобы принять все raw-исходные данные
    // Неверно - у нас могут быть модификаторы ' ' - и это значит, что
    // в локальных данных (data) будет меньше полей и меньше общий размер,
    // чем в пришедших данных. Непонятно пока, что с этим делать.
    // Надо предварительно читать pk и по ней узнавать размер требуемого буфера
    // для принимаемых данных
    uint8_t src[sz];
    size_t sz1 = rcvraw(src, sz);
    if (sz1 < 0)
        return false;

    char pk[ strlen_P(pk_P) + 1 ];
    strcpy_P(pk, pk_P);

    return dataunpack(data, sz, pk, src, sz1);
}

size_t BinProto::rcvraw(uint8_t *data, size_t sz) {
    // Принимаем данные по ожидаемой команде
    if (m_nsock == NULL)
        return -1;
    if ((m_rcvstate != RCV_DATA) && (m_rcvstate != RCV_COMPLETE) && (m_rcvstate != RCV_NULL))
        return -1;
    
    size_t rsz = sz < m_rcvsz ? sz : m_rcvsz;
    size_t sz1 = -1;
    if (rsz > 0) {
        sz1 = m_nsock->recv(data, rsz);
        if (sz1 > m_rcvsz)
            m_rcvsz = 0;
        else
        if (sz1 > 0)
            m_rcvsz -= sz1;
    }
    
    if (m_rcvsz > 0)
        m_rcvstate = RCV_NULL;
    else
        rcvnext();
    
    return sz1;
}

bool BinProto::rcvnext() {
    // пропускаем данные по ожидаемой команде
    // и пытаемся принять следующую команду
    if (m_nsock == NULL)
        return false;
    if ((m_rcvstate != RCV_DATA) && (m_rcvstate != RCV_COMPLETE))
        return false;
    
    if (m_rcvsz > 0)
        m_rcvstate = RCV_NULL;
    else
        rcvclear();
    
    if (rcvprocess() < RCV_WAITCMD)
        return false;
    
    return true;
}
