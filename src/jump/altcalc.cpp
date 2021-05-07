/*
    Alt calculate
*/

#include "altcalc.h"

float calc_alt(float pressgnd, float pressure) {
  return 44330 * (1.0 - pow(pressure / pressgnd, 0.1903));
}

void altcalc::tick(float press)
{
#ifdef PRESS_TEST
    if (_state > ACST_INIT)
        press = _presstest -= PRESS_TEST;
#endif
    uint32_t m = millis();
    auto &r = (_rr[cur] = { mill: m, press: press, alt: calc_alt(_pressgnd, press) });
    
    // курсор в rr-массиве
    cur ++;
    if (cur >= AC_RR_ALT_SIZE) {
        if (_state == ACST_INIT) // Завершение инициализации (rr-массив полностью заполнен)
            initend();
        cur = 0;
    }
    
    // количество тиков между изменениями в направлении движения (вверх/вниз)
    _dircnt++;
    
    if (_state == ACST_INIT) // Пока не завершилась инициализация, дальше ничего не считаем
        return;
    
    _presslast = r.press;
    _altlast = r.alt;
    _altprev = _rr[cur].alt;
    uint32_t millprev = _rr[cur].mill;

    // считаем коэффициенты линейной аппроксимации
    float sy=0, sxy = 0, sx = 0, sx2 = 0;
    for (auto &r: _rr) {
        float x = r.mill - millprev;
        sx  += x;
        sx2 += x*x;
        sy  += r.alt;
        sxy += x*r.alt;
    }

    float n = AC_RR_ALT_SIZE;
    float a = (n*sxy - (sx*sy)) / (n*sx2-(sx*sx));
    float b = (sy - (a*sx)) / n;

    _speed = a >= 0 ? a*1000 : a * -1000;
    _altappr = ((float)(m - millprev)) * a + b;
    
    float a1000 = a*1000; // верт скорость в м/с со знаком: + вверх, - вниз
    
    _stateprev = _state;
    switch (_state) {
        case ACST_GROUND:
            if ((_altlast > 40) && (_altlast < 100) && (a1000 > 1))
                _state = ACST_TAKEOFF40;
            break;
        case ACST_TAKEOFF40:
            if ((_altlast < 10) && (_speed < 1))
                _state = ACST_GROUND;
            else
            if ((_altlast > 100) && (a1000 > 1))
                _state = ACST_TAKEOFF;
            break;
        case ACST_TAKEOFF:
            if ((_altlast < 40) && (_speed < 1))
                _state = ACST_GROUND;
            else
            if ((_altlast > 300) && (a1000 < -30))
                _state = ACST_FREEFALL;
            else
            if ((_altlast > 1000) && (a1000 < -5))
                _state = ACST_CANOPY;
            break;
        case ACST_FREEFALL:
            if ((_altlast > 300) && (a1000 > 3))
                _state = ACST_TAKEOFF;
            else
            if (a1000 > -25)
                _state = ACST_CANOPY;
            else
            if ((_altlast < 40) && (_speed < 1))
                _state = ACST_GROUND;
            else
            break;
        case ACST_CANOPY:
            if ((_altlast > 300) && (a1000 > 3))
                _state = ACST_TAKEOFF;
            else
            if ((_altlast < 40) && (_speed < 1))
                _state = ACST_GROUND;
            else
            if ((_altlast > 300) && (a1000 < -30))
                _state = ACST_FREEFALL;
            else
            if ((_altlast < 100) && ((a1000 > -25) && (a1000 < -1)))
                _state = ACST_LANDING;
            break;
        case ACST_LANDING:
            if ((_altlast < 100) && ((a1000 > -1) && (a1000 < 1)))
                _state = ACST_GROUND;
            break;
    }
    
    // текущее направление движения
    if ((_dir != ACDIR_NULL) && (a1000 > -0.3) && (a1000 < 0.3)) {
        _dir = ACDIR_NULL;
        _dircnt = 0;
    }
    else
    if ((_dir != ACDIR_UP) && (a1000 > 0.5)) {
        _dir = ACDIR_UP;
        _dircnt = 0;
    }
    else
    if ((_dir != ACDIR_DOWN) && (a1000 < -0.5)) {
        _dir = ACDIR_DOWN;
        _dircnt = 0;
    }
}

void altcalc::gndreset() {
    // пересчёт _pressgnd
    
    if (_state == ACST_INIT) // Пока не завершилась инициализация, дальше ничего не считаем
        return;
    
    double pr = 0;
    for (auto &r: _rr) // т.к. мы пересчитали _pressgnd, то пересчитаем и alt
        pr += _rr[cur].press;
    _pressgnd = pr / AC_RR_ALT_SIZE;
    _state = ACST_GROUND;
    
    for (auto &r: _rr) // т.к. мы пересчитали _pressgnd, то пересчитаем и alt
        r.alt = calc_alt(_pressgnd, r.press);
    
    // Сбрасываем направление движения при сбросе нуля по той же причине - мы пересчитали высоты
    _dir = ACDIR_INIT;
    _dircnt = 0;
}

void altcalc::initend() {
    // сразу после включения первые значения датчик даёт сбойные, поэтому считаем
    // по той же формуле среднего взвешенного - самое крайнее значение - наиболее значимое
    float sm = 0, w = 1, ws = 0;
    while (cur > 0) {
        cur--;
        sm += w * _rr[cur].press; // среднее взвешенное
        ws += w; // суммарный вес
        w /= 2;
    }
    _pressgnd = sm / ws;
    _state = ACST_GROUND;
#ifdef PRESS_TEST
    _presstest = _pressgnd;
#endif
    
    for (auto &r: _rr) // т.к. мы пересчитали _pressgnd, то пересчитаем и alt
        r.alt = calc_alt(_pressgnd, r.press);
}
