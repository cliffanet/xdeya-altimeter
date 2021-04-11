/*
    u-blox GPS proto
*/

#ifndef __gps_ubloxproto_H
#define __gps_ubloxproto_H

#include <stddef.h>
#include <Stream.h>


#define UBX_SYNC1           0xb5
#define UBX_SYNC2           0x62

#define UBX_CFG             0x06
#define UBX_CFG_PRT         0x00
#define UBX_CFG_MSG         0x01
#define UBX_CFG_RST         0x04
#define UBX_CFG_RATE        0x08
#define UBX_CFG_NAV5        0x24

#define UBX_NAV             0x01
#define UBX_NAV_POSLLH      0x02
#define UBX_NAV_STATUS      0x03
#define UBX_NAV_SOL         0x06
#define UBX_NAV_PVT         0x07
#define UBX_NAV_VELNED      0x12
#define UBX_NAV_TIMEUTC     0x21

#define UBX_NMEA            0xf0
#define UBX_NMEA_GPGGA      0x00
#define UBX_NMEA_GPGLL      0x01
#define UBX_NMEA_GPGSA      0x02
#define UBX_NMEA_GPGSV      0x03
#define UBX_NMEA_GPRMC      0x04
#define UBX_NMEA_GPVTG      0x05

typedef enum {
    UBXWB_SYNC1,
    UBXWB_SYNC2,
    UBXWB_CLASS,
    UBXWB_IDENT,
    UBXWB_PLEN1,
    UBXWB_PLEN2,
    UBXWB_PAYLOAD,
    UBXWB_CKA,
    UBXWB_CKB
} ubloxgps_bytewait_t;

class UbloxGpsProto
{
    public:
        UbloxGpsProto() { rcvclear(); _uart = NULL; }
        UbloxGpsProto(Stream &uart) { rcvclear(); _uart = &uart; }
        bool recv(char c);
        bool recv() { return (_uart != NULL) && _uart->available() ? recv(_uart->read()) : false; }
        void rcvclear();
        bool tick();
        
        UbloxGpsProto& operator << (const char &c) { recv(c); return *this; }
        
        bool send(uint8_t cl, uint8_t id, const uint8_t *data = NULL, uint16_t len = 0);
        template <typename T>
        bool send(uint8_t cl, uint8_t id, const T &data) {
            return send(cl, id, reinterpret_cast<const uint8_t *>(&data), sizeof(T));
        }
  
    private:
        Stream *_uart;
        ubloxgps_bytewait_t rcv_bytewait;
        uint8_t rcv_class, rcv_ident, rcv_cka, rcv_ckb;
        uint16_t rcv_plen;
        uint8_t buf[128], bufi;
        
        void rcvcks(char c);
};

#endif // __gps_ubloxproto_H
