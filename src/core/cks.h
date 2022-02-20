/*
    Контрольная сумма
*/

#ifndef _core_cks_H
#define _core_cks_H

#include <stdint.h>
#include <stddef.h>

// Длинная контрольная сумма - для валидации целиком локальных файлов, передваемых по сети
template <typename Tcs, typename Tsz, typename Tsmpl, typename Tfull>
struct  __attribute__((__packed__)) cks_s {
    Tcs     csa;
    Tcs     csb;
    Tsz     sz;
    
    bool operator== (const struct cks_s<Tcs, Tsz, Tsmpl, Tfull> & cks) {
        return (this == &cks) || ((this->csa == cks.csa) && (this->csb == cks.csb) && (this->sz == cks.sz));
    };
    operator bool() const { return (csa != 0) && (csb != 0) && (sz != 0); }
    Tsmpl   smpl() const {
        return (csa << (sizeof(Tcs)*8)) | csb;
    }
    Tfull   full() const {
        return (csa << (sizeof(Tcs)*8 + sizeof(Tsz)*8)) | (csb << (sizeof(Tsz)*8)) | sz;
    }

    void clear() {
        csa = 0;
        csb = 0;
        sz = 0;
    }

    void add(uint8_t d) {
        csa += d;
        csb += csa;
        sz ++;
    }

    void add(const uint8_t *d, size_t _sz) {
        sz += _sz;
        for (; _sz > 0; _sz--, d++) {
            csa += *d;
            csb += csa;
        }
    }
    
    template <typename T>
    void add(const T &data) {
        add(reinterpret_cast<const uint8_t *>(&data), sizeof(T));
    }
};

typedef struct cks_s<uint8_t,uint16_t,uint16_t,uint32_t> cks8_t;
typedef struct cks_s<uint16_t,uint32_t,uint32_t,uint64_t> cks16_t;

inline void cks8(uint8_t d, uint8_t &cka, uint8_t &ckb) {
    cka += d;
    ckb += cka;
}
inline void cks16(uint8_t d, uint16_t &cka, uint16_t &ckb) {
    cka += d;
    ckb += cka;
}

#endif // _core_cks_H
