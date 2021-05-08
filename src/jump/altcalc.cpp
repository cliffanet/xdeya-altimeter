/*
    Alt calculate
*/

#include "altcalc.h"
#include <math.h>

float press2alt(float pressgnd, float pressure) {
  return 44330 * (1.0 - pow(pressure / pressgnd, 0.1903));
}

void AltCalc::tick(float press, uint16_t interval)
{
    if (interval > 0) {
        // курсор первого уровня
        cur1 ++;
        if (cur1 >= AC_LEVEL1_COUNT)
            cur1 = 0;
    
        _d1[cur1].interval = interval;
    }
    else
    if (cur1 >= AC_LEVEL1_COUNT)
        return;
    
    _d1[cur1].press = press;
    _d1[cur1].alt   = press2alt(_pressgnd, press);
    
    if (_state == ACST_INIT) {
        // в режиме инициализации мы только заполняем _d1 и _d2
        if (cur2 > 0)
            gndreset(); // при этом каждый раз пересчитываем уровень земли
        l1calc();
        if (cur1+1 >= AC_LEVEL1_COUNT) {
            if (cur2+1 < AC_LEVEL2_COUNT) {
                // пока ещё идёт процесс заполнения _d2
                auto &d2 = _d2[cur2];
                l2next();
                // при переходе на следующий d2 дублируем его из предыдущего, 
                // чтобы сохранить актуальные значения, если к ним будут обращения через методы
                _d2[cur2] = d2;
            }
            else {
                // а если мы заполнили уже все _d2,
                // вычисляем текущее состояние и дальше будем работать 
                // по стандартному алгоритму переходов к следующим ячейкам
                _state = ACST_GROUND;
                stateupdate();
            }
        }
    }
    else {
        // стандартный алгоритм заполнения _d1 и _d2
        if (cur1 == 0)
            l2next();
    
        l1calc();
        // количество тиков между изменениями в направлении движения (вверх/вниз)
        _dirtm += interval;
        
        if (cur1+1 >= AC_LEVEL1_COUNT)
            stateupdate();
    }
}

void AltCalc::l1calc() {
    // считаем коэффициенты линейной аппроксимации
    double sy=0, sxy = 0, sx = 0, sx2 = 0, alt = 0;
    uint32_t x = 0, n = 0;
    // начинаем с самого первого (начиная с cur1+1 и до конца списка)
    for (uint8_t c = cur1+1; c < AC_LEVEL1_COUNT; c++) {
        auto &d = _d1[c];
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
    // и с самого начала списка до cur1 включительно
    for (uint8_t c = 0; c <= cur1; c++) {
        auto &d = _d1[c];
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

    auto &d2 = _d2[cur2];
    
    // коэфициенты
    d2.interval = x;
    d2.ka       = (n*sxy - (sx*sy)) / (n*sx2-(sx*sx));
    d2.kb       = (sy - (d2.ka*sx)) / n;
    
    // средняя высота
    d2.altavg   = alt / n;
    
    // средняя скорость
    uint8_t c = cur1+1;
    if (c >= AC_LEVEL1_COUNT)
        c = 0;
    if (x > _d1[c].interval) { // защита от деления на ноль и переполнения при вычитании
        // вычитаем из x самый первый интервал (перед вычисленной самой первой высотой), 
        // т.к. для вычисления скорости он не нужен, а нужны только интервалы между первой высотой и текущей
        uint32_t x1 = x-_d1[c].interval;
        d2.speedavg = (_d1[cur1].alt - _d1[c].alt) * 1000 / x1;
    }
    else {
        d2.speedavg = 0;
    }
}

void AltCalc::l2next() {
    // курсор второго уровня
    cur2 ++;
    if (cur2 >= AC_LEVEL2_COUNT)
        cur2 = 0;
}

ac_state_t AltCalc::stateupdate() {
    double ka = 0; // среднее знчение ka
    for (auto &d: _d2)
        ka += d.ka;
    ka = ka / AC_LEVEL2_COUNT;
    _kaavg = ka;

    double adm = 0; // максимальное отклонение от среднего ka
    for (auto &d: _d2) {
        double adiff = d.ka - ka;
        if (adiff < 0)
            adiff = adiff * -1;
        if (adm < adiff)
            adm = adiff;
    }
    _adiffmax = adm/ka;
    
    ac_direct_t dir = ACDIR_ERR;
    
    if ((ka > -AC_DIR_SPEED) && (ka < AC_DIR_SPEED))
        // если среднее ka в пределах +-AC_DIR_SPEED, считаем, что высота не меняется при любой погрешности
        dir = ACDIR_FLAT;
    else
    if (
            ((ka > AC_DIR_SPEED*5) && (adm/ka < 0.25)) ||
            // при 5-кратном превышении AC_DIR_SPEED, допустимое отклонение: 15%
            ((ka > AC_DIR_SPEED) && (adm/ka < 0.50))        
            // при меньшем превышении AC_DIR_SPEED, допустимое отклонение может достигать 25%
        )
        dir = ACDIR_UP;
    else
    if (
            ((ka < -AC_DIR_SPEED*5) && (adm/ka > -0.25)) ||
            ((ka < -AC_DIR_SPEED) && (adm/ka > -0.50))
        )
        dir = ACDIR_DOWN;
    
    if (_dir != dir) {
        _dirtm = 0;
        _dir = dir;
    }
    if (_dir == ACDIR_ERR)
        return _state;
    
    if (altapp() < 10) {
        _state = ACST_GROUND;
    }
    else
    switch (_dir) {
        case ACDIR_UP:
            _state = altapp() < 40 ? ACST_TAKEOFF40 : ACST_TAKEOFF;
            break;
            
        case ACDIR_DOWN:
            _state = 
                ka < -0.030 ?
                    ACST_FREEFALL :
                altapp() > 100 ?
                    ACST_CANOPY : 
                    ACST_LANDING;
            break;
    }
}

void AltCalc::gndreset() {
    // пересчёт _pressgnd
    double pr = 0;
    for (auto &d: _d1) // т.к. мы пересчитали _pressgnd, то пересчитаем и alt
        pr += d.press;
    _pressgnd = pr / AC_LEVEL1_COUNT;
    
    for (auto &d: _d1) // т.к. мы пересчитали _pressgnd, то пересчитаем и alt
        d.alt = press2alt(_pressgnd, d.press);
    
    if (_state == ACST_INIT)
        return;
    
    if (_state != ACST_GROUND) {
        _state = ACST_GROUND;
        _statetm = 0;
    }
    
    // т.к. изменились высоты, пересчитаем все коэфициенты
    l1calc();
}
