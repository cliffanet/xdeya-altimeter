/*
    Compass - processing
*/

#ifndef _compass_H
#define _compass_H

#include "../../def.h"
#include <stdint.h>

#define CFG_MAGCAL_ID     6
#define CFG_MAGCAL_VER    1
#define CFG_MAGCAL_NAME   "magcal"

template <typename T> struct vec_t { T x, y, z; };

typedef vec_t<int8_t> vec8_t;
typedef vec_t<int16_t> vec16_t;
typedef vec_t<int32_t> vec32_t;
typedef vec_t<float> vecf_t;

typedef struct {
    vec16_t mag;
    vec16_t acc;
	double  head;
	double  speed;
    vec32_t e;
    uint8_t ok;
} compass_t;

typedef struct __attribute__((__packed__)) {
    vec16_t min;
    vec16_t max;
} cfg_magcal_t;

const compass_t &compass();

void compInit();
void compStop();
bool compCalibrate(const vec16_t &min, const vec16_t &max);
void compProcess();

#endif // _compass_H
