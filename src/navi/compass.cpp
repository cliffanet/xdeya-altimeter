
#include "compass.h"
#include "../log.h"
#include "../clock.h"
#include "../filtlib/filter_avgdeg.h"
#include "../cfg/main.h"

#include <Wire.h>

static compass_t _cmp = { 0 };

static FilterAvgDeg <double>filt(7);

static Config<cfg_magcal_t> cal(PSTR(CFG_MAGCAL_NAME), CFG_MAGCAL_ID, CFG_MAGCAL_VER);


/* --------------------------------------------------
 *                      vector func
 */
template <typename Ta, typename Tb>
vec32_t vec_cross(const vec_t<Ta> &a1, const vec_t<Tb> &b) {
    vec32_t a = {
        static_cast<int32_t>(a1.x),
        static_cast<int32_t>(a1.y),
        static_cast<int32_t>(a1.z)
    };
    return {
        (a.y * b.z) - (a.z * b.y),
        (a.z * b.x) - (a.x * b.z),
        (a.x * b.y) - (a.y * b.x)
    };
}

template <typename Ta, typename Tb> float vec_sp(const vec_t<Ta> &a, const vec_t<Tb> &b) {
    return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

template <typename T> float vec_len(const vec_t<T> &a) {
    return sqrt(vec_sp(a, a));
}

template <typename T> void vec_norm(vec_t<T> &a) {
    float len = vec_len(a);
    a.x /= len;
    a.y /= len;
    a.z /= len;
}

static vecf_t vec16f(const vec16_t &a) {
    return {
        static_cast<float>(a.x),
        static_cast<float>(a.y),
        static_cast<float>(a.z)
    };
}

/* --------------------------------------------------
 *                      wire
 */
static bool wwrite8(uint8_t addr, uint8_t reg, uint8_t val) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(val);
    return Wire.endTransmission() == 0;
}

static void wend() {
    Wire.endTransmission();
}
static bool wwait(uint8_t count, uint64_t us = 100000) {
    uint64_t u = utick();
    while ((Wire.available() < count) && (utm_diff(u) < us)) ;
    if (Wire.available() >= count)
        return true;
    CONSOLE("wire read timeout");
    return false;
}
static bool wreq(uint8_t addr, uint8_t reg, uint8_t count, uint64_t us = 100000) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.endTransmission();
    Wire.requestFrom(addr, count);

    // Wait around until enough data is available
    return wwait(count, us);
}
static bool wreq(uint8_t addr, uint8_t reg, uint8_t *data, uint8_t count, uint64_t us = 100000) {
    if (!wreq(addr, reg, count, us))
        return false;
    while (count > 0) {
        *data = Wire.read();
        data++;
        count--;
    }
    wend();
    
    return true;
}

static uint8_t wread8(uint8_t addr, uint8_t reg, uint64_t us = 100000) {
    uint8_t val;
    
    if (!wreq(addr, reg, &val, 1, us))
        return -1;

    return val;
}


#include "compas/lsm303dlh.h"




/* --------------------------------------------------
 *                      Compass Processing
 */
const compass_t &compass() { return _cmp; }

void compInit() {
    _cmp.ok = 0;
/*
    hmc5883 + mpu6050
    or
    mpu9250 + ak8963
    Для ak8963 важно, чтобы сначала инициализировался mpu9250,
    т.к. ak8963 подключён по i2c через mpu9250, и надо на mpu9250
    включить режим bypass
*/
    if (accInit())
        _cmp.ok |= 2;
    else
        CONSOLE("accel/gyro fail");
    
    if (magInit())
        _cmp.ok |= 1;
    else
        CONSOLE("compas fail");
    
    if (_cmp.ok == 3)
        CONSOLE("compas & accel/gyro init ok");
    
    cal.reset();
    cal.load();
    if (cal.isempty())
        CONSOLE("calibrate: empty");
    else
        CONSOLE("calibrate: x: %d..%d; y: %d..%d; z: %d..%d",
            cal.d().min.x, cal.d().max.x, cal.d().min.y, 
            cal.d().max.y, cal.d().min.z, cal.d().max.z);
}

void compStop() {
/*
    hmc5883 + mpu6050
    or
    mpu9250 + ak8963
*/
    if (_cmp.ok & 1) {
        magStop();
        CONSOLE("compas stop");
    }
    if (_cmp.ok & 2) {
        accStop();
        CONSOLE("accel stop");
    }
    
    _cmp = { 0 };
}

bool compCalibrate(const vec16_t &min, const vec16_t &max) {
    cal.set().min = min;
    cal.set().max = max;
    CONSOLE("x: %d..%d; y: %d..%d; z: %d..%d", min.x, max.x, min.y, max.y, min.z, max.z);
    return cal.save();
}

void compProcess() {
    vec16_t mag;
    
    if (_cmp.ok & 1) {
        _cmp.mag = magRead();
        
        if (cal.isempty())
            mag = _cmp.mag;
        else {
            vec16_t cen = {
                static_cast<int16_t>( (cal.d().max.x - cal.d().min.x) / 2 + cal.d().min.x ),
                static_cast<int16_t>( (cal.d().max.y - cal.d().min.y) / 2 + cal.d().min.y ),
                static_cast<int16_t>( (cal.d().max.z - cal.d().min.z) / 2 + cal.d().min.z ),
            };
            
            vec32_t c = {
                _cmp.mag.x - cen.x,
                _cmp.mag.y - cen.y,
                _cmp.mag.z - cen.z,
            };
            
            mag = {
                static_cast<int16_t>( c.x * 2000 / (cal.d().max.x - cal.d().min.x) ),
                static_cast<int16_t>( c.y * 2000 / (cal.d().max.y - cal.d().min.y) ),
                static_cast<int16_t>( c.z * 2000 / (cal.d().max.z - cal.d().min.z) )
            };
        }
    }
    else
        mag = { 0, 0, 0 };
    
    if (_cmp.ok & 2)
        _cmp.acc = accRead();
    
    // heading
    if ((_cmp.ok & 3) == 3) {
        vec32_t E = vec_cross(mag, _cmp.acc);
        _cmp.e = E;
        //vec32_t N = vec_cross(_cmp.acc, E);
        
        // В математике 0 градусов - это направление оси X,
        // А нам нужны навигационные градусы (по оси Y),
        // Поэтому нам фактически нужен даже не вектор N,
        // а как раз подходит вектор E - его математические
        // градусы равны навигационным градусам вектора N
        _cmp.head = atan2(static_cast<double>(E.y), E.x);
        
        // фильтр - среднее арифметическое для круглых величин
        static uint32_t tck;
        uint32_t interval = utm_diff32(tck, tck);
        filt.tick(_cmp.head, interval / 1000);
        
        _cmp.head = filt.value();
        if (_cmp.head < 0)
            _cmp.head += 2*PI;
        
        _cmp.speed = filt.speed();
    }
}

