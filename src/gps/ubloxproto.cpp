/*
    u-blox GPS proto
*/

#include "ubloxproto.h"
#include <Arduino.h>


bool UbloxGpsProto::recv(uint8_t c, bool clearonfail) {
    switch (rcv_bytewait) {
        case UBXWB_SYNC1:
    		if (c == UBX_SYNC1)
    			rcv_bytewait = UBXWB_SYNC2;
            return true;
            
        case UBXWB_SYNC2:
    		if (c == UBX_SYNC2) {
    			rcv_bytewait = UBXWB_CLASS;
                return true;
            }
            break;
            
        case UBXWB_CLASS:
            rcv_class = c;
            rcvcks(c);
            rcv_bytewait = UBXWB_IDENT;
            return true;
            
        case UBXWB_IDENT:
            rcv_ident = c;
            rcvcks(c);
            rcv_bytewait = UBXWB_PLEN1;
            return true;
            
        case UBXWB_PLEN1:
            rcv_plen = c;
            rcvcks(c);
            rcv_bytewait = UBXWB_PLEN2;
            return true;
            
        case UBXWB_PLEN2:
            rcv_plen |= c << 8;
            rcvcks(c);
            rcv_bytewait = rcv_plen > 0 ? UBXWB_PAYLOAD : UBXWB_CKA;
            return true;
            
        case UBXWB_PAYLOAD:
            if (rcv_plen > 0) {
                if (bufi < sizeof(buf))
                    buf[bufi] = c;
                bufi++;
                rcvcks(c);
                if (bufi >= rcv_plen)
                    rcv_bytewait = UBXWB_CKA;
                return true;
            }
            break;
            
        case UBXWB_CKA:
            if (c == rcv_cka) {
                rcv_bytewait = UBXWB_CKB;
                return true;
            }
            break;
            
        case UBXWB_CKB:
            if (c == rcv_ckb) {
                docmd();
                rcvclear();
                return true;
            }
            break;
    }

    if (clearonfail) rcvclear();
    return false;
}

void UbloxGpsProto::rcvclear() {
    rcv_bytewait = UBXWB_SYNC1;
    rcv_class = 0;
    rcv_ident = 0;
    rcv_cka = 0;
    rcv_ckb = 0;
    rcv_plen = 0;
    bufi = 0;
}

bool UbloxGpsProto::tick() {
    if (_uart == NULL)
        return false;
    
    while (_uart->available()) {
        uint8_t c = _uart->read();
        if (!recv(c, false)) {
            Serial.printf("gps recv proto fail on byte=0x%02x, waited=0x%02x, rcv_class=0x%02x, rcv_ident=0x%02x, rcv_plen=%d\n", 
                                c, rcv_bytewait, rcv_class, rcv_ident, rcv_plen);
            rcvclear();
            return false;
        }
    }
    
    if ((cnftimeout > 0) && (cnftimeout <= millis())) {
        Serial.printf("gps timeout on wait confirm (sndcnt=%d)\n", sndcnt);
        cnfclear();
    }
    
    return true;
}

bool UbloxGpsProto::docmd() {
    if ((rcv_class == UBX_ACK) && (rcv_plen == 2))
        return rcvconfirm(rcv_ident == UBX_ACK_ACK);
    
    Serial.printf("gps recv unknown cmd class=0x%02X, id=0x%02X, len=%d\n", rcv_class, rcv_ident, rcv_plen);
    
    return false;
}

bool UbloxGpsProto::waitcnf() {
    if (sndcnt == 0)
        return false;
    
    while (cnftimeout >= millis()) {
        if (!_uart->available()) {
            delay(50);
            continue;
        }
        
        char c;
        if (!recv(c = _uart->read())) {
            _uart->flush();
            Serial.printf("gps on waitcnf proto fail on byte=0x%02x, waited=0x%02x, rcv_class=0x%02x, rcv_ident=0x%02x, rcv_plen=%d\n",
                                c, rcv_bytewait, rcv_class, rcv_ident);
            return false;
        }
        
        if (sndcnt == 0)
            return true;
    }
    
    Serial.printf("gps wait confirm timeout, sndcnt=%d, waited=0x%02x, rcv_class=0x%02x, rcv_ident=0x%02x\n", sndcnt, rcv_bytewait, rcv_class, rcv_ident);
    cnfclear();
    
    return false;
}

void UbloxGpsProto::cnfclear() {
    sndcnt = 0;
    cnftimeout = 0;
}

void UbloxGpsProto::rcvcks(uint8_t c) {
    rcv_cka += c;
    rcv_ckb += rcv_cka;
}

bool UbloxGpsProto::rcvconfirm(bool isok) {
    if (sndcnt == 0) {
        Serial.printf("gps recv confirm(%d), but cmd not sended\n", isok);
        return false;
    }
    
    sndcnt--;
    if (sndcnt == 0)
        cnftimeout = 0;
    Serial.printf(isok ? "gps recv cmd-confirm\n" : "gps recv cmd-reject\n");
    
    return true;
}

bool UbloxGpsProto::send(uint8_t cl, uint8_t id, const uint8_t *data, uint16_t dlen) {
    if (_uart == NULL)
        return false;
    if (data == NULL)
        dlen = 0;

    struct __attribute__((__packed__)) {
        uint8_t sync1;
        uint8_t sync2;
        uint8_t cl;
        uint8_t id;
        uint8_t plen1;
        uint8_t plen2;
    } hdr = {
        UBX_SYNC1, UBX_SYNC2, cl, id, dlen & 0xff, (dlen & 0xff00) >> 8
    };
    struct __attribute__((__packed__)) {
        uint8_t a;
        uint8_t b;
    } ck = { 0, 0 };
    
#define CK(x)   { ck.a += x; ck.b += ck.a; }
    CK(hdr.cl);
    CK(hdr.id);
    CK(hdr.plen1);
    CK(hdr.plen2);
    if (dlen > 0)
        for (uint16_t i = 0; i < dlen; i++)
            CK(data[i]);
#undef CK
    
    if (!_uart->write(reinterpret_cast<const uint8_t *>(&hdr), sizeof(hdr)))
        return false;
    if ((dlen > 0) && !_uart->write(data, dlen))
        return false;

    Serial.printf("gps send class=0x%02X, id=0x%02X, len=%d\n", cl, id, dlen);
    
    if (!_uart->write(reinterpret_cast<const uint8_t *>(&ck), sizeof(ck)))
        return false;
    sndcnt++;
    cnftimeout = millis()+1000;
    
    return true;
}
