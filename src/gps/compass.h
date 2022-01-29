/*
    Compass - processing
*/

#ifndef _compass_H
#define _compass_H

#include "../../def.h"
#include <stdint.h>

template <typename T> struct vec_t { T x, y, z; };

typedef vec_t<int16_t> vec16_t;
typedef vec_t<int32_t> vec32_t;
typedef vec_t<float> vecf_t;

typedef struct {
    vec16_t mag;
    vec16_t acc;
	float   head;
    vec32_t e;
    uint8_t ok;
} compass_t;

const compass_t &compass();

void compInit();
void compProcess();

#endif // _compass_H
