/*
    Server base function
*/

#ifndef _net_srv_H
#define _net_srv_H

#include <Arduino.h>

const char *srvErr();

bool srvConnect();
void srvStop();

/* ------------------------------------------------------------------------------------------- *
 *  чтение
 * ------------------------------------------------------------------------------------------- */
bool srvRecv(uint8_t &cmd, uint8_t *data = NULL, uint16_t sz = 0);

template <typename T>
bool srvRecv(uint8_t &cmd, T &data) {
    return srvRecv(cmd, reinterpret_cast<uint8_t *>(&data), sizeof(T));
}

/* ------------------------------------------------------------------------------------------- *
 *  отправка
 * ------------------------------------------------------------------------------------------- */
bool srvSend(uint8_t cmd, const uint8_t *data = NULL, uint16_t sz = 0);

template <typename T>
bool srvSend(uint8_t cmd, const T &data) {
    return srvSend(cmd, reinterpret_cast<const uint8_t *>(&data), sizeof(T));
}

#endif // _net_srv_H
