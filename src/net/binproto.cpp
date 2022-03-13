/*
    Server base function
*/

#include "binproto.h"

#include <strings.h>
#include <arpa/inet.h> // ntohs() 

size_t NetSocket::recv(size_t sz) {
    size_t avail = available();
    if (avail > sz)
        avail = sz;
    if (avail == 0)
        return 0;
    
    uint8_t data[avail];
    size_t sz1 = recv(data, avail);
    if (sz1 > sz)
        sz1 = sz;
    return sz1;
}

BinProto::BinProto() : 
    BinProto(NULL)
{
    
}

BinProto::BinProto(NetSocket * nsock) :
    m_connected(false),
    m_waitcmd(0),
    m_waitsz(0)
{
    sock_set(nsock);
}

void BinProto::sock_set(NetSocket * nsock) {
    if (nsock == NULL) {
        sock_clear();
        return;
    }
    
    m_nsock = nsock;
}

void BinProto::sock_clear() {
    m_nsock = NULL;
    m_waitcmd = 0;
    m_waitsz = 0;
}

void BinProto::recv_set(uint8_t cmd, recv_hnd_t hnd, uint16_t sz, uint16_t timeout, hnd_t on_timeout) {
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

void BinProto::recv_del(uint8_t cmd) {
    m_recv.erase(cmd);
}

void BinProto::recv_clear() {
    m_recv.clear();
}

void BinProto::process() {
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
        }
    }

    if ((m_nsock != NULL) && (m_waitcmd > 0)) {
        // Принимаем данные по ожидаемой команде
        auto it = m_recv.find(m_waitcmd);
        if (it == m_recv.end()) {
            m_waitcmd = 0;
            onerror(ERR_RECVUNKNOWN);
        }
        else
        if (it->second.hnd == NULL) {
            m_waitcmd = 0;
        }
        else
        if (it->second.sz > 0) {
            uint8_t data[it->second.sz];
            size_t sz1 = m_waitsz < it->second.sz ? m_waitsz : it->second.sz;
            size_t sz2 = m_nsock->recv(data, sz1);
            if (sz2 == sz1) {
                if (sz2 < it->second.sz)
                    bzero(data + sz2, it->second.sz - sz2);
                it->second.hnd(data);
                m_waitsz -= sz2;
            }
            else {
                onerror(ERR_RECV);
                if (sz2 > m_waitsz)
                    m_waitsz = 0;
                else
                if (sz2 > 0)
                    m_waitsz -= sz2;
            }
            
            m_waitcmd = 0;
        }
    }

    if ((m_nsock != NULL) &&
        (m_waitcmd == 0) &&
        (m_waitsz > 0) &&
        (m_nsock->available() >= sizeof(hdr_t))) {
        // Стравливание остатков данных команды, 
        // которых мы не ожидали.
        size_t sz1 = m_nsock->recv(m_waitsz);
        if (sz1 >= 0)
            m_waitsz -= sz1;
        else
            onerror(ERR_RECV);
    }
    
    if (m_connected && ((m_nsock == NULL) || !m_nsock->connected())) {
        ondisconnect();
        m_connected = false;
        m_waitcmd = 0;
        m_waitsz = 0;
    }
    else
    if (!m_connected && (m_nsock != NULL) && m_nsock->connected()) {
        onconnect();
        m_connected = true;
    }
}
