/*
    NetSocket class
*/

#ifndef _net_socket_H
#define _net_socket_H

#include <stdint.h>
#include <stddef.h>

/* ------------------------------------------------------------------------------------------- *
 *  Виртуальный класс, обеспечивающий необходимые функции передачи данных
 * ------------------------------------------------------------------------------------------- */
class NetSocket {
    public:
        virtual void close() = 0;
        virtual bool connected() = 0;
        virtual size_t available() = 0;
        virtual size_t recv(uint8_t *data, size_t sz) = 0;
        virtual size_t recv(size_t sz); // стравливание буфера
        virtual size_t send(const uint8_t *data, size_t sz) = 0;
        virtual const char *err_P() const { return NULL; }
};


/* ------------------------------------------------------------------------------------------- *
 *  Надстройка над классом WiFiClient (или над любым другим классом на базе Client от Arduino)
 *
 *  Мы используем свой класс для работы с сетью, чтобы определять основные требуемые функции:
 *  read/write и т.д.
 *  Пока это нужно только для BinProto
 *
 *  Почти всегда работают стандартные функции socket(), read(), write().
 *  Однако, есть сложность с реализацией нужного нам метода available().
 *  В esp32 для wifi-модуля в sdk предусмотрена работа с rx-буферами,
 *  но я не нашёл размер этого буфера по умолчанию. И кажется, по умолчанию
 *  он вообще отключен (но это неточно).
 *
 *  В классе WiFiClient используется свой rx-roundrobin-буфер размером 1436 байт.
 *  Для работы метода available() берётся весь размер данных, принятых в свой rx-буфер,
 *  а так же, данных во внутреннем буфере sdk - через lwip_ioctl(_fd, FIONREAD, &count);
 * 
 *  Чтобы не изобретать велосипед, в Arduino уже всё нужное реализовано в WiFiClient.
 *  Для использования этого шаблона под WiFiClient пишем в любом модуле:
 *  template class NetSocketClient<WiFiClient>;
 *  Это реализует методы класса шаблона NetSocketClient под WiFiClient
 *
 *  А со временем можно реализовать свой класс, который не будет подвязан на специфичные
 *  для lwIP функции для esp32, а будет только читать в свой буфер стандартными
 *  select/read без учёта sdk-буфера.
 * ------------------------------------------------------------------------------------------- */
template <class T>
class NetSocketClient : public NetSocket {
    public:
        NetSocketClient() {}
        NetSocketClient(const T &_cli) { m_cli = _cli; }
        void setcli(const T &_cli) { m_cli = _cli; }
        
        /*
            T &cli() const { return m_cli; } // есть какой-то макрос cli, с ним конфликтует
            
            ERR: /Users/cliff/Library/Arduino15/packages/esp32/hardware/esp32/2.0.5/cores/esp32/Arduino.h:84:15: note: in expansion of macro 'portDISABLE_INTERRUPTS'
            ERR:  #define cli() portDISABLE_INTERRUPTS()
        */
        
        void close() {
            m_cli.stop();
        }
        
        bool connected() {
            return m_cli.connected();
        }
        
        size_t available() {
            return m_cli.available();
        }
        
        size_t recv(uint8_t *data, size_t sz) {
            return m_cli.read(data, sz);
        }
        
        size_t send(const uint8_t *data, size_t sz) {
            return m_cli.write(data, sz);
        }
    
    private:
        T m_cli;
};

#endif // _net_socket_H
