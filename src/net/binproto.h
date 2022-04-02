/*
    BinProto class
*/

#ifndef _net_binproto_H
#define _net_binproto_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
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
        
        typedef const char *item_t;
        
        typedef struct {
            cmdkey_t cmd;
            const char *pk_P;
        } elem_t;
        
        BinProto(char mgc = '#', const elem_t *all = NULL, size_t count = 0);
        void add(const cmdkey_t &cmd, const char *pk_P);
        void add(const elem_t *all, size_t count);
        void del(const cmdkey_t &cmd);
        item_t pk_P(const cmdkey_t &cmd) const;
        
        int hdrsz() const { return 4; }
        void hdrpack(uint8_t *buf, const cmdkey_t &cmd, uint16_t sz);
        bool hdrunpack(const uint8_t *buf, cmdkey_t &cmd, uint16_t &sz);

        int pack(uint8_t *buf, size_t bufsz, const cmdkey_t &cmd, const uint8_t *src, size_t srcsz);
        bool unpack(cmdkey_t &cmd, uint8_t *dst, size_t dstsz, const uint8_t *buf, size_t bufsz);
    
    private:
        char m_mgc;
        std::map<cmdkey_t, item_t> m_all;
};

class BinProtoSend : public BinProto {
    public:
        BinProtoSend(NetSocket * nsock = NULL, char mgc = '#', const elem_t *all = NULL, size_t count = 0);
        void sock_set(NetSocket * nsock);
        void sock_clear();
        bool send(const cmdkey_t &cmd, const uint8_t *data, size_t sz);
        
        // Данные будем упаковывать не из аргументов вызова send,
        // как это было изначально реализовано в BinProtoSend,
        // а из упакованной структуры данных, но где поля уже будем считать аргументами
        // нужных типов (в т.ч. dooble, float и т.д.),
        // По сути, точно так же делает va_list, но при этом va_list не умеет нормально
        // работать с аргументами как с ссылками на переменные (& - актуально для recv),
        // только если аргументы переданы как указатель на переменную (*)
        template <typename T>
        bool send(const cmdkey_t &cmd, const T &data) {
            return send(cmd, reinterpret_cast<const uint8_t *>(&data), sizeof(T));
        }
    
    private:
        NetSocket *m_nsock;
};


class BinProtoRecv : public BinProto {
    public:
        typedef enum {
            STATE_ERROR = 0,
            STATE_NORECV,
            STATE_CMD,
            STATE_OK
        } state_t;
    
        typedef enum {
            ERR_NONE = 0,
            ERR_PROTO,
            ERR_RECV,
            ERR_DISCONECT
        } err_t;
    
        BinProtoRecv(NetSocket * nsock = NULL, char mgc = '#', const elem_t *all = NULL, size_t count = 0);
        void sock_set(NetSocket * nsock);
        void sock_clear();
        
        err_t error() const { return m_err; }
        const cmdkey_t &cmd() const { return m_waitcmd; }
        bool recv(cmdkey_t &cmd, uint8_t *data, size_t sz);
        state_t process();
        
        // Данные будем распаковывать не в аргументы вызова recv,
        // как это было изначально реализовано в BinProtoSend,
        // а в упакованную структуру данных, но где поля уже будем считать аргументами
        // нужных типов (в т.ч. dooble, float и т.д.),
        // По сути, точно так же делает va_list, но при этом va_list не умеет нормально
        // работать с аргументами как с ссылками на переменные (&),
        // только если аргументы переданы как указатель на переменную (*)
        template <typename T>
        bool recv(cmdkey_t &cmd, T &data) {
            return recv(cmd, reinterpret_cast<uint8_t *>(&data), sizeof(T));
        }
        
        // И от списка обраобтчиков мы отказываемся, т.к. в c++ нет нормальной реализации
        // ссылок на методы класса, только геморное использование std::bind, но это слишком сложно.
        // После того, как process(); вернёт STATE_CMD, надо прочитать данные через recv(...).
        // Если перед вызовом recv() надо знать принимаемую команду, чтобы передать в recv() нужную
        // структуру данных, это можно проверить через cmd()
    
    private:
        NetSocket *m_nsock;
        
        err_t m_err;
        uint8_t m_waitcmd;
        uint16_t m_waitsz;
        uint16_t m_datasz;
};

#endif // _net_binproto_H
