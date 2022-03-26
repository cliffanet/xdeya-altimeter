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
        virtual bool connected() = 0;
        virtual size_t available() = 0;
        virtual size_t recv(uint8_t *data, size_t sz) = 0;
        virtual size_t recv(size_t sz); // стравливание буфера
        virtual size_t send(const uint8_t *data, size_t sz) = 0;
        virtual const char *err_P() const { return NULL; }
};

/* ------------------------------------------------------------------------------------------- *
 *  Процессинг по передаче упакованных данных
 * ------------------------------------------------------------------------------------------- */
class BinProto {
    public:
        typedef uint8_t cmdkey_t;
        
        typedef struct {
            const char *pk_P;
        } item_t;
        
        typedef struct {
            cmdkey_t cmd;
            const char *pk_P;
        } elem_t;
        
        BinProto(char mgc = '#', const elem_t *all = NULL, size_t count = 0);
        void add(const cmdkey_t &cmd, const char *pk_P);
        void add(const elem_t *all, size_t count);
        void del(const cmdkey_t &cmd);

        int vpack(uint8_t *buf, size_t sz, const cmdkey_t &cmd, va_list va);
        int pack(uint8_t *buf, size_t sz, const cmdkey_t &cmd, ...);
        bool unpack(uint8_t *buf, size_t sz, const cmdkey_t &cmd, ...);
    
    private:
        char m_mgc;
        std::map<cmdkey_t, item_t> m_all;
};

class BinProtoSend : public BinProto {
    public:
        BinProtoSend(NetSocket * nsock = NULL, char mgc = '#', const elem_t *all = NULL, size_t count = 0);
        void sock_set(NetSocket * nsock);
        void sock_clear();
        bool send(const cmdkey_t &cmd, ...);
    
    private:
        NetSocket *m_nsock;
};

/*
class BinProtoRecv : public BinProto {
    public:
    
        typedef void (*hnd_t)();
        typedef void (*recv_hnd_t)(const uint8_t *data);
        typedef struct {
            recv_hnd_t hnd;
            uint16_t sz;
            uint16_t timeout;
            hnd_t on_timeout;
        } recv_item_t;
    
        typedef enum {
            ERR_NONE = 0,
            ERR_PROTO,
            ERR_RECV,
            ERR_RECVUNKNOWN,
            ERR_SEND
        } err_t;
    
        BinProtoRecv();
        BinProtoRecv(NetSocket * nsock);
        void sock_set(NetSocket * nsock);
        void sock_clear();
        void recv_set(uint8_t cmd, recv_hnd_t hnd, uint16_t sz = 0, uint16_t timeout = 0, hnd_t on_timeout = NULL);
        void recv_del(uint8_t cmd);
        void recv_clear();
        bool process();
    
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
        uint16_t m_datasz;
};
*/

#endif // _net_binproto_H
