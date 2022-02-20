
/* ------------------------------------------------------------------------------------------- *
 *  Контрольная сумма
 * ------------------------------------------------------------------------------------------- */

#include "cks.h"

bool cks_t::operator== (const struct chs_s & cks) {
    return (this == &cks) || ((this->csa == cks.csa) && (this->csb == cks.csb) && (this->sz == cks.sz));
};

void cks_t::clear() {
    csa = 0;
    csb = 0;
    sz = 0;
}

void cks_t::add(uint8_t d) {
    csa += d;
    csb += csa;
    sz ++;
}

void cks_t::add(const uint8_t *d, size_t _sz) {
    sz += _sz;
    for (; sz > 0; _sz--, d++) {
        csa += *d;
        csb += csa;
    }
}
