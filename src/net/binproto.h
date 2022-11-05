/*
    BinProto class
*/

#ifndef _net_binproto_H
#define _net_binproto_H

#include "netsocket.h"

/* ------------------------------------------------------------------------------------------- *
 *  Процессинг по передаче упакованных данных
 * ------------------------------------------------------------------------------------------- */

// Размер строки, ожидаемой в структуре данных, при работе pack/unpack с форматтером 's'
#define BINPROTO_STRSZ      256

class BinProto {
    public:
        typedef uint8_t cmdkey_t;
        typedef enum {
            RCV_ERROR,
            RCV_DISCONNECTED,
            RCV_WAITCMD,
            RCV_DATA,
            RCV_NULL,
            RCV_COMPLETE
        } rcvst_t;
        
        BinProto(NetSocket * nsock = NULL, char mgcsnd = '#', char mgcrcv = '#');
        
        int hdrsz() const { return 4; }
        void hdrpack(uint8_t *buf, const cmdkey_t &cmd, uint16_t sz);
        bool hdrunpack(const uint8_t *buf, cmdkey_t &cmd, uint16_t &sz);
        
        static
        int datapack(uint8_t *dst, size_t dstsz, const char *pk, const uint8_t *src, size_t srcsz);
        static
        bool dataunpack(uint8_t *dst, size_t dstsz, const char *pk, const uint8_t *src, size_t srcsz);

        NetSocket *sock() const { return m_nsock; }
        void sock_set(NetSocket * nsock);
        void sock_clear();
        
        bool send(const cmdkey_t &cmd, const char *pk_P, const uint8_t *data, size_t sz);
        // Данные будем упаковывать не из аргументов вызова send,
        // как это было изначально реализовано в BinProtoSend,
        // а из упакованной структуры данных, но где поля уже будем считать аргументами
        // нужных типов (в т.ч. dooble, float и т.д.),
        // По сути, точно так же делает va_list, но при этом va_list не умеет нормально
        // работать с аргументами как с ссылками на переменные (& - актуально для recv),
        // только если аргументы переданы как указатель на переменную (*)
        template <typename T>
        bool send(const cmdkey_t &cmd, const char *pk_P, const T &data) {
            return send(cmd, pk_P, reinterpret_cast<const uint8_t *>(&data), sizeof(T));
        }
        bool send(const cmdkey_t &cmd) {
            return send(cmd, NULL, NULL, 0);
        }
        
        rcvst_t rcvstate()  const { return m_rcvstate; }
        // то же, что и rcvstate, но ограничивается определением, в рабочей ли фазе находимся или зафиксирована проблема
        bool    rcvvalid()  const { return m_rcvstate > RCV_DISCONNECTED; }
        // текущая принятая команда
        cmdkey_t rcvcmd()   const { return m_rcvcmd; }
        // сбрасывает процесс приёма команды (например, при дисконнекте от сервера)
        void    rcvclear();
        // процессинг приёма данных
        rcvst_t rcvprocess();
        
        // Приём данных с распаковкой:
        // - формат pk_P - это PROGMEM-строка
        bool rcvdata(const char *pk_P, uint8_t *data, size_t sz);
        bool rcvdata(const char *pk_P, char *data, size_t sz) {
            return rcvdata(pk_P, reinterpret_cast<uint8_t *>(data), sz);
        }
        // Данные будем распаковывать не в аргументы вызова recv,
        // как это было изначально реализовано в BinProtoSend,
        // а в упакованную структуру данных, но где поля уже будем считать аргументами
        // нужных типов (в т.ч. dooble, float и т.д.),
        // По сути, точно так же делает va_list, но при этом va_list не умеет нормально
        // работать с аргументами как с ссылками на переменные (&),
        // только если аргументы переданы как указатель на переменную (*)
        template <typename T>
        bool rcvdata(const char *pk_P, T &data) {
            return rcvdata(pk_P, reinterpret_cast<uint8_t *>(&data), sizeof(T));
        }
        
        // Читает данные как есть, без распаковки
        // Допустимо использовать как при RCV_DATA, так и при RCV_NULL,
        // т.е. уже после применения rcvdata()
        size_t rcvraw(uint8_t *data, size_t sz);
        
        // Переход к следующей команде (пропускаем приём данных от текущей)
        bool rcvnext();
    
    private:
        char m_mgcsnd, m_mgcrcv;
        rcvst_t m_rcvstate;
        cmdkey_t m_rcvcmd;
        uint16_t m_rcvsz;
        NetSocket *m_nsock;
};

#endif // _net_binproto_H
