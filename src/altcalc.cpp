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
    if (_state == ACST_INIT) // Пока не завершилась инициализация, дальше ничего не считаем
        return;
    
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

    switch (_state) {
        case ACST_GROUND:
            if ((_altlast > 40) && (_altlast < 100) && ((a*1000) > 1))
                _state = ACST_TAKEOFF40;
            break;
        case ACST_TAKEOFF40:
            if ((_altlast < 10) && (_speed < 1))
                _state = ACST_GROUND;
            else
            if ((_altlast > 100) && ((a*1000) > 1))
                _state = ACST_TAKEOFF;
            break;
        case ACST_TAKEOFF:
            if ((_altlast < 40) && (_speed < 1))
                _state = ACST_GROUND;
            else
            if ((_altlast > 300) && ((a*1000) < -30))
                _state = ACST_FREEFALL;
            else
            if ((_altlast > 1000) && ((a*1000) < -5))
                _state = ACST_CANOPY;
            break;
        case ACST_FREEFALL:
            if ((_altlast > 300) && ((a*1000) > 3))
                _state = ACST_TAKEOFF;
            else
            if ((a*1000) > -25)
                _state = ACST_CANOPY;
            else
            if ((_altlast < 40) && (_speed < 1))
                _state = ACST_GROUND;
            else
            break;
        case ACST_CANOPY:
            if ((_altlast > 300) && ((a*1000) > 3))
                _state = ACST_TAKEOFF;
            else
            if ((_altlast < 40) && (_speed < 1))
                _state = ACST_GROUND;
            else
            if ((_altlast > 300) && ((a*1000) < -30))
                _state = ACST_FREEFALL;
            else
            if ((_altlast < 100) && (((a*1000) > -25) && ((a*1000) < -1)))
                _state = ACST_LANDING;
            break;
        case ACST_LANDING:
            if ((_altlast < 100) && (((a*1000) > -1) && ((a*1000) < 1)))
                _state = ACST_GROUND;
            break;
    }
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
