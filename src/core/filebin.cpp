
#include "filebin.h"
#include "cks.h"

#include "FS.h"

/* ------------------------------------------------------------------------------------------- *
 *  Прямая работа с записью в файле
 * ------------------------------------------------------------------------------------------- */

bool recBinRead(fs::File &fh, uint8_t *data, uint16_t dsz, bool tailnull) {
    uint16_t sz = 0;
    uint8_t bs[2];
    uint8_t cka = 0, ckb = 0;
    
    if (fh.read(bs, 2) != 2)
        return false;
    
    cks8(bs[0], cka, ckb);
    cks8(bs[1], cka, ckb);
    
    sz |= bs[0];
    sz |= bs[1] << 8;
    sz+=2;
    
    uint8_t buf[sz], *b = buf;
    size_t sz1 = fh.read(buf, sz);
    
    if (sz1 != sz)
        return false;
    
    while ((sz > 2) && (dsz > 0)) {
        cks8(*b, cka, ckb);
        *data = *b;
        b++;
        sz--;
        data++;
        dsz--;
    }
    
    while (sz > 2) {
        cks8(*b, cka, ckb);
        b++;
        sz--;
    }
    
    while (tailnull && (dsz > 0)) {
        *data = 0;
        data++;
        dsz--;
    }
    
    if ((b[0] != cka) || (b[1] != ckb))
        return false;
    
    return true;
}

bool recBinWrite(fs::File &fh, const uint8_t *data, uint16_t dsz) {
    uint16_t sz = dsz + 4;
    uint8_t buf[sz], *b = buf;
    uint8_t cka = 0, ckb = 0;
    
    *b = dsz & 0xff;
    cks8(*b, cka, ckb);
    b++;
    *b = (dsz & 0xff00) >> 8;
    cks8(*b, cka, ckb);
    b++;
    
    while (dsz > 0) {
        cks8(*data, cka, ckb);
        *b = *data;
        b++;
        data++;
        dsz--;
    }
    
    *b = cka;
    b++;
    *b = ckb;
    
    size_t sz1 = fh.write(buf, sz);
    if (sz1 != sz)
        return false;
    
    return true;
}