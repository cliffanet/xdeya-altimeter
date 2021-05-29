/*
    Alt calculate
*/

#include "altcalc.h"
#include <math.h>

float press2alt(float pressgnd, float pressure) {
  return 44330 * (1.0 - pow(pressure / pressgnd, 0.1903));
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
    
    auto &d = _data[_c];
    _press0 = d.press;
    _alt0   = d.alt;
    d.press = press;
    d.alt   = press2alt(_pressgnd, press);
    
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
    // sy - единственный ненулевой элемент в нулевой точке - равен _alt0
    double sy=_alt0, sxy = 0, sx = 0, sx2 = 0;
    // количество элементов на один больше, чем AC_DATA_COUNT,
    // т.к. нулевой элемент находится не в _data, а в _alt0/_press0
    uint32_t x = 0, n = AC_DATA_COUNT + 1;
    
    int8_t i = ifrst();
    do {
        auto &d = _data[i];
        if (d.interval == 0)
            continue;
        
        x   += d.interval;
        sx  += x;
        sx2 += x*x;
        sy  += d.alt;
        sxy += x*d.alt;
    } while (inext(i));
    
    // коэфициенты
    _interval = x;
    _ka       = (sxy*n - (sx*sy)) / (sx2*n-(sx*sx));
    _kb       = (sy - (_ka*sx)) / n;
    
    // средняя высота
    _altavg   = sy / n;
    
    // средняя скорость - разница между крайними значениями
    if (_interval > 0) { // защита от деления на ноль и переполнения при вычитании
        // вычитаем из x самый первый интервал (перед вычисленной самой первой высотой), 
        // т.к. для вычисления скорости он не нужен, а нужны только интервалы между первой высотой и текущей
        _speedavg = (_data[_c].alt - _alt0) * 1000 / _interval;
    }
    else {
        _speedavg = 0;
    }
    
    // среднеквадратичное отклонение высот от прямой _speedavg
    double sq = 0;
    x = 0;
    i = ifrst();
    do {
        auto &d = _data[i];
        x += d.interval;
        double altd = _speedavg * x / 1000 + _alt0 - d.alt;
        sq += altd * altd;
    } while (inext(i));
    
    _sqdiff = sqrt(sq / (n-1));
}

ac_state_t AltCalc::stateupdate() {
    ac_direct_t dir = // направление движения
        speedapp() > AC_SPEED_FLAT ?
            ACDIR_UP :
        speedapp() < -AC_SPEED_FLAT ?
            ACDIR_DOWN :
            ACDIR_FLAT;
    if (_dir != dir) {
        _dir = dir;
        _dircnt = 0;
        _dirtm = 0;
    }
    
    ac_state_t st = _state;
    uint32_t stcnt = 0, sttm = 0;
    if (altapp() < 10) {
        st = ACST_GROUND;
    }
    else
    if (_dir == ACDIR_UP) {
        st = altapp() < 40 ? ACST_TAKEOFF40 : ACST_TAKEOFF;
    }
    else
    if (speedapp() < -AC_SPEED_FREEFALL) {
        st = ACST_FREEFALL;
        stcnt = AC_DATA_COUNT;
        sttm = _interval;
    }
    else
    if ((_dir == ACDIR_DOWN) && (altapp() < 100)) {
        st = ACST_LANDING;
    }
    else
    if ((speedapp() > -AC_SPEED_FREEFALL) || (speedapp() < -AC_SPEED_FLAT)) {
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
    _alt0 = press2alt(_pressgnd, _press0);
    
    if (_state == ACST_INIT)
        return;
    
    if (_state != ACST_GROUND) {
        _state = ACST_GROUND;
        statereset();
    }
    
    // т.к. изменились высоты, пересчитаем все коэфициенты
    dcalc();
}

int8_t AltCalc::i2i(int8_t i) {
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
    
    return i;
}

const int8_t AltCalc::ifrst() const {
    int8_t i = _c+1;
    if (i >= AC_DATA_COUNT)
        i = 0;
    return i;
}

bool AltCalc::inext(int8_t &i) {
    bool ok = i != _c;
    i++;
    if (i >= AC_DATA_COUNT)
        i = 0;
    return ok;
}
