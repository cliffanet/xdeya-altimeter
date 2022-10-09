/*
    u-blox proto
*/

#ifndef __gps_ubloxproto_H
#define __gps_ubloxproto_H

#include <stddef.h>
#include <Stream.h>


#define UBX_SYNC1           0xb5
#define UBX_SYNC2           0x62

#define UBX_ACK             0x05
#define UBX_ACK_NAK         0x00
#define UBX_ACK_ACK         0x01

#define UBX_CFG             0x06
#define UBX_CFG_PRT         0x00
#define UBX_CFG_MSG         0x01
#define UBX_CFG_RST         0x04
#define UBX_CFG_RATE        0x08
#define UBX_CFG_NAV5        0x24
#define UBX_CFG_GNSS        0x3E

#define UBX_NAV             0x01
#define UBX_NAV_POSLLH      0x02
#define UBX_NAV_STATUS      0x03
#define UBX_NAV_SOL         0x06
#define UBX_NAV_PVT         0x07
#define UBX_NAV_VELNED      0x12
#define UBX_NAV_TIMEUTC     0x21
#define UBX_NAV_SAT         0x35

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

class UbloxGpsProto;
typedef void (*ubloxgps_hnd_t)(UbloxGpsProto &gps);
typedef struct {
    uint8_t         cl;
    uint8_t         id;
    bool            istmp;
    ubloxgps_hnd_t  hnd;
} ubloxgps_hnditem_t;

#define UBX_HND_SIZE    10
#define UBX_CONFIRM_TIMEOUT 1000

typedef struct {
    	uint8_t msgClass;  // Message class
    	uint8_t msgID;     // Message identifier
    	uint8_t rate;      // Send rate
    } ubx_cfg_rate_t;

class UbloxGpsProto
{
    public:
        UbloxGpsProto(uint16_t _bufsize = 256);
        UbloxGpsProto(Stream &__uart, uint16_t _bufsize = 256);
        ~UbloxGpsProto();
        bool bufinit(uint16_t _bufsize = 256);
        void uart(Stream *__uart) { _uart = __uart; }
        Stream *uart() const { return _uart; }
        
        bool recv(uint8_t c);
        void rcvclear();
        bool tick(void (*readhnd)(uint8_t c) = NULL);
        
        bool docmd();
        void cnfclear();
        uint16_t cnfneed() { return sndcnt; }
        
        UbloxGpsProto& operator << (const char &c) { if (!recv(c)) rcvclear(); return *this; }
        
        bool bufcopy(uint8_t *data, uint16_t dsz, uint16_t offs = 0);
        template <typename T>
        bool bufcopy(T &data) {
            return bufcopy(reinterpret_cast<uint8_t *>(&data), sizeof(T));
        }
        template <typename T, typename TH>
        bool bufitem(TH &hdr, T &item, uint16_t n) {
            return bufcopy(reinterpret_cast<uint8_t *>(&item), sizeof(item), sizeof(hdr)+(n*sizeof(item)));
        }
        uint16_t plen() { return rcv_plen; }
        
        bool send(uint8_t cl, uint8_t id, const uint8_t *data = NULL, uint16_t len = 0);
        template <typename T>
        bool send(uint8_t cl, uint8_t id, const T &data) {
            return send(cl, id, reinterpret_cast<const uint8_t *>(&data), sizeof(T));
        }
        
        bool get(uint8_t cl, uint8_t id, ubloxgps_hnd_t hnd);
        
        bool hndadd(uint8_t cl, uint8_t id, ubloxgps_hnd_t hnd, bool istmp = false);
        bool hnddel(uint8_t cl, uint8_t id, ubloxgps_hnd_t hnd);
        void hndclear();
        void hndzero(ubloxgps_hnditem_t &h);
        uint8_t hndcall(uint8_t cl, uint8_t id);
        
        uint32_t cntRecv() const { return cntrecv; }
        uint32_t cntRecvErr() const { return cntrecverr; }
        uint32_t cntCmdUnknown() const { return cntcmdunknown; }
  
    private:
        Stream *_uart;
        ubloxgps_bytewait_t rcv_bytewait;
        uint8_t rcv_class, rcv_ident, rcv_cka, rcv_ckb;
        uint16_t rcv_plen, bufi, bufsz;
        uint8_t *buf = NULL;
        uint16_t sndcnt;
        ubloxgps_hnditem_t hndall[UBX_HND_SIZE];
        
        uint32_t cntrecv=0, cntrecverr=0, cntcmdunknown=0;
        
        void rcvcks(uint8_t c);
        bool rcvconfirm(bool isok);
};

#endif // __gps_ubloxproto_H
