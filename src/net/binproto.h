/*
    BinProto class
*/

#ifndef _net_binproto_H
#define _net_binproto_H

#include <stdint.h>
#include <stddef.h>
#include <map>

/* ------------------------------------------------------------------------------------------- *
 *  Виртуальный класс, обеспечивающий необходимые функции передачи данных
 * ------------------------------------------------------------------------------------------- */
class NetSocket {
    public:
        virtual bool connect() = 0;
        virtual void disconnect() = 0;
        virtual bool connected() const = 0;
        virtual size_t available() const = 0;
        virtual size_t recv(uint8_t *data, size_t sz) = 0;
        virtual size_t recv(size_t sz); // стравливание буфера
        virtual size_t send(const uint8_t *data, size_t sz) = 0;
};

/* ------------------------------------------------------------------------------------------- *
 *  Процессинг по передаче упакованных данных
 * ------------------------------------------------------------------------------------------- */

class BinProto {
    public:
    
        typedef void (*hnd_t)();
        typedef void (*recv_hnd_t)(const uint8_t *data);
        typedef struct {
            recv_hnd_t hnd;
            uint16_t sz;
            uint16_t timeout;
            hnd_t on_timeout;
        } recv_item_t;
    
        // Заголовок данных при сетевой передаче
        typedef struct __attribute__((__packed__)) {
            char    mgc;
            uint8_t cmd;
            uint16_t sz;
        } hdr_t;
    
        typedef enum {
            ERR_NONE = 0,
            ERR_PROTO,
            ERR_RECV,
            ERR_RECVUNKNOWN,
            ERR_SEND
        } err_t;
    
        BinProto();
        BinProto(NetSocket * nsock);
        void sock_set(NetSocket * nsock);
        void sock_clear();
        void recv_set(uint8_t cmd, recv_hnd_t hnd, uint16_t sz = 0, uint16_t timeout = 0, hnd_t on_timeout = NULL);
        void recv_del(uint8_t cmd);
        void recv_clear();
        void process();
    
    protected:
        virtual void onconnect() {}
        virtual void ondisconnect() {}
        virtual void onerror(err_t err) {}
    
    private:
        NetSocket *m_nsock;
        std::map<uint8_t, recv_item_t> m_recv;
        
        bool m_connected;
        uint8_t m_waitcmd;
        uint16_t m_waitsz;
};

#endif // _net_binproto_H
