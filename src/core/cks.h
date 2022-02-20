/*
    Контрольная сумма
*/

#ifndef _core_cks_H
#define _core_cks_H

#include <stdint.h>
#include <stddef.h>

// Длинная контрольная сумма - для валидации целиком локальных файлов, передваемых по сети
typedef struct  __attribute__((__packed__)) chs_s {
    uint16_t    csa;
    uint16_t    csb;
    uint32_t    sz;
    
    bool operator== (const struct chs_s & cks);
    operator bool() const { return (csa != 0) && (csb != 0) && (sz != 0); }
    uint32_t    smpl() const { return (csa << 16) | csb; }
    
    void clear();
    void add(uint8_t d);
    void add(const uint8_t *d, size_t _sz);
    
    template <typename T>
    void add(const T &data) {
        add(reinterpret_cast<const uint8_t *>(&data), sizeof(T));
    }
} cks_t;

inline void cks8(uint8_t d, uint8_t &cka, uint8_t &ckb) {
    cka += d;
    ckb += cka;
}
inline void cks16(uint8_t d, uint16_t &cka, uint16_t &ckb) {
    cka += d;
    ckb += cka;
}

#endif // _core_cks_H
