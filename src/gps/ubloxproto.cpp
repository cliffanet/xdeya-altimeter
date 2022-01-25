/*
    u-blox GPS proto
*/

#include "ubloxproto.h"
//#include "../log.h"


bool UbloxGpsProto::recv(uint8_t c) {
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

bool UbloxGpsProto::tick(void (*readhnd)(uint8_t c)) {
    if (_uart == NULL)
        return false;
    
    while (_uart->available()) {
        uint8_t c = _uart->read();
        cntrecv++;
        if (readhnd != NULL)
            readhnd(c);
        if (!recv(c)) {
            cntrecverr++;
            rcvclear();
            if (sndcnt > 0)
                cnfclear();
            return false;
        }
    }
    
    return true;
}

bool UbloxGpsProto::docmd() {
    if ((rcv_class == UBX_ACK) && (rcv_plen == 2))
        return rcvconfirm(rcv_ident == UBX_ACK_ACK);
    else
    if (hndcall(rcv_class, rcv_ident) > 0)
        return true;
    
    //CONSOLE("gps recv unknown cmd class=0x%02X, id=0x%02X, len=%d", rcv_class, rcv_ident, rcv_plen);
    cntcmdunknown++;
    
    return false;
}

void UbloxGpsProto::cnfclear() {
    sndcnt = 0;
}

void UbloxGpsProto::rcvcks(uint8_t c) {
    rcv_cka += c;
    rcv_ckb += rcv_cka;
}

bool UbloxGpsProto::rcvconfirm(bool isok) {
    if (sndcnt == 0) {
        //CONSOLE("gps recv confirm(%d), but cmd not sended", isok);
        return false;
    }
    
    sndcnt--;
    //if (isok)
    //    CONSOLE("gps recv cmd-confirm");
    //else
    //    CONSOLE("gps recv cmd-reject");
    
    return true;
}

bool UbloxGpsProto::bufcopy(uint8_t *data, uint16_t dsz, uint16_t offs) {
    if (rcv_bytewait != UBXWB_CKB)
        return false;
    
    uint8_t *d = data, *b = buf;
    uint16_t bsz = sizeof(buf);
    if (bsz > rcv_plen)
        bsz = rcv_plen;
    
    if (offs < bsz) {
        bsz -= offs;
        b += offs;
    }
    else {
        bsz = 0;
    }
    
    while ((dsz > 0) && (bsz > 0)) {
        *d = *b;
        d++;
        b++;
        dsz--;
        bsz--;
    }
    
    while (dsz > 0) {
        *d = 0;
        dsz--;
    }
    
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

    //CONSOLE("gps send class=0x%02X, id=0x%02X, len=%d", cl, id, dlen);
    
    if (!_uart->write(reinterpret_cast<const uint8_t *>(&ck), sizeof(ck)))
        return false;
    sndcnt++;
    
    return true;
}
        
bool UbloxGpsProto::hndadd(uint8_t cl, uint8_t id, ubloxgps_hnd_t hnd, bool istmp) {
    for (auto &h : hndall)
        if ((h.cl == 0) && (h.id == 0)) {
            h.cl = cl;
            h.id = id;
            h.istmp = istmp;
            h.hnd = hnd;
            
            return true;
        }
            
    return false;
}

bool UbloxGpsProto::hnddel(uint8_t cl, uint8_t id, ubloxgps_hnd_t hnd) {
    for (auto &h : hndall)
        if ((h.cl == cl) && (h.id == id)) {
            hndzero(h);
            return true;
        }
            
    return false;
}

void UbloxGpsProto::hndclear() {
    for (auto &h : hndall)
        hndzero(h);
}

void UbloxGpsProto::hndzero(ubloxgps_hnditem_t &h) {
    h.cl = 0;
    h.id = 0;
    h.istmp = false;
    h.hnd = NULL;
}

uint8_t UbloxGpsProto::hndcall(uint8_t cl, uint8_t id) {
    uint8_t cnt = 0;
    for (auto &h : hndall)
        if ((h.cl == cl) && (h.id == id)) {
            if (h.hnd != NULL)
                h.hnd(*this);
            if (h.istmp)
                hndzero(h);
            cnt++;
        }
    return cnt;
}
