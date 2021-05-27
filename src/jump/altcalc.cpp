/*
    Alt calculate
*/

#include "altcalc.h"
#include <math.h>

float press2alt(float pressgnd, float pressure) {
  return 44330 * (1.0 - pow(pressure / pressgnd, 0.1903));
}

const ac_data_t & AltCalc::data(int32_t i) const {
    if (i < 0) {
        i += AC_DATA_COUNT;
        if (i < 0)
            i = 0;
    }
    i += _c;
    if (i >= AC_DATA_COUNT)
        i -= AC_DATA_COUNT;
    
    if (i >= AC_DATA_COUNT)
        i = AC_DATA_COUNT-1;
    else
    if (i < 0)
        i = 0;
    
    return _data[i];
}

void AltCalc::tick(float press, uint16_t tinterval)
{
    if (tinterval > 0) {
        // курсор первого уровня
        _c ++;
        if (_c >= AC_DATA_COUNT)
            _c = 0;
    
        _data[_c].interval = tinterval;
    }
    else
    if (_c >= AC_DATA_COUNT)
        return;
    
    _data[_c].press = press;
    _data[_c].alt   = press2alt(_pressgnd, press);
    
    if (_state == ACST_INIT) {
        if (_c+1 >= AC_DATA_COUNT) {
            gndreset();
            _state = ACST_GROUND;
        }
        else
            return;
    }
    
    dcalc();
    // количество тиков между изменениями в направлении движения (вверх/вниз)
    _dirtm += tinterval;
    _dircnt ++;
    _statetm += tinterval;
    _statecnt ++;
    
    stateupdate();
}

void AltCalc::dcalc() {
    // считаем коэффициенты линейной аппроксимации
    double sy=0, sxy = 0, sx = 0, sx2 = 0, alt = 0;
    uint32_t x = 0, n = 0;
    
    for (uint8_t i = 0; i < AC_DATA_COUNT; i++) {
        auto &d = data(i);
        if (d.interval == 0)
            continue;
        
        x   += d.interval;
        sx  += x;
        sx2 += x*x;
        sy  += d.alt;
        sxy += x*d.alt;
        
        alt += d.alt;
        n   ++;
    }
    
    // коэфициенты
    _interval = x;
    _ka       = (n*sxy - (sx*sy)) / (n*sx2-(sx*sx));
    _kb       = (sy - (_ka*sx)) / n;
    
    // средняя высота
    _altavg   = alt / n;
    
    // средняя скорость - разница между крайними значениями
    if (_interval > 0) { // защита от деления на ноль и переполнения при вычитании
        // вычитаем из x самый первый интервал (перед вычисленной самой первой высотой), 
        // т.к. для вычисления скорости он не нужен, а нужны только интервалы между первой высотой и текущей
        uint32_t x1 = x - data(-1).interval;
        _speedavg = (data(0).alt - data(-1).alt) * 1000 / x1;
    }
    else {
        _speedavg = 0;
    }
    
    float alt0 = data(0).alt;
    x = 0;
    double sq = 0;
    for (uint8_t i = 1; i < AC_DATA_COUNT; i++) {
        auto &d = data(i);
        x += d.interval;
        double altd = _speedavg * x / 1000 + alt0 - d.alt;
        sq += altd * altd;
    }
    _sqdiff = sqrt(sq / (AC_DATA_COUNT-1));
}

ac_state_t AltCalc::stateupdate() {
    ac_direct_t dir = // направление движения
        _ka > AC_SPEED_FLAT ?
            ACDIR_UP :
        _ka < -AC_SPEED_FLAT ?
            ACDIR_DOWN :
            ACDIR_FLAT;
    if (_dir != dir) {
        _dir = dir;
        _dircnt = 0;
    }
    
    ac_state_t st = _state;
    uint32_t stcnt = 0, sttm = 0;
    if (altapp() < 10) {
        st = ACST_GROUND;
    }
    else
    if (_dir == ACDIR_UP) {
        st = altapp() < 40 ? ACST_TAKEOFF40 : ACST_TAKEOFF;
        stcnt = _dircnt;
        sttm = _dirtm;
    }
    else
    if (_ka > -AC_SPEED_FREEFALL) {
        st = ACST_FREEFALL;
        stcnt = AC_DATA_COUNT;
        sttm = _interval;
    }
    else
    if ((_dir == ACDIR_DOWN) && (altapp() < 100)) {
        st = ACST_LANDING;
    }
    else
    if ((_ka < -AC_SPEED_FREEFALL) || (_ka > -AC_SPEED_FLAT)) {
        st = ACST_CANOPY;
        stcnt = AC_DATA_COUNT;
        sttm = _interval;
    }
    
    if (_state != st) {
        _state = st;
        _statecnt = stcnt;
        _statetm = sttm;
    }
}

void AltCalc::gndreset() {
    // пересчёт _pressgnd
    double pr = 0;
    for (auto &d: _data) // т.к. мы пересчитали _pressgnd, то пересчитаем и alt
        pr += d.press;
    _pressgnd = pr / AC_DATA_COUNT;
    
    for (auto &d: _data) // т.к. мы пересчитали _pressgnd, то пересчитаем и alt
        d.alt = press2alt(_pressgnd, d.press);
    
    if (_state == ACST_INIT)
        return;
    
    if (_state != ACST_GROUND) {
        _state = ACST_GROUND;
        statereset();
    }
    
    // т.к. изменились высоты, пересчитаем все коэфициенты
    dcalc();
}
