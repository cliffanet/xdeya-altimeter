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
            // в этом месте _press0 ещё не заполнена,
            // и из-за этого неверно считаются avg-значения
            // сразу после перехода из init в рабочий режим,
            // поэтому заполним её самым нулевым значением,
            // _alt0 будет пересчитана в gndreset();
            _press0 = _data[0].press;
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
    _jmptm += tinterval;
    _jmpcnt ++;
    _sqbigtm += tinterval;
    _sqbigcnt ++;
    
    stateupdate(tinterval);
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

ac_state_t AltCalc::stateupdate(uint16_t tinterval) {
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

    // большая турбуленция (высокое среднеквадратическое отклонение)
    if (!_sqbig && (_sqdiff >= AC_JMP_SQBIG_THRESH)) {
        _sqbig = true;
        _dircnt = 0;
        _dirtm = 0;
    }
    else
    if (_sqbig && (_sqdiff < AC_JMP_SQBIG_MIN)) {
        _sqbig = false;
        _dircnt = 0;
        _dirtm = 0;
    }
    
    // state определяется исходя из направления движения и скорости
    // т.е. это состояние на текущее мнгновение
    ac_state_t st = _state;
    if (_dir == ACDIR_UP) {
        st = altapp() < 40 ? ACST_TAKEOFF40 : ACST_TAKEOFF;
    }
    else 
    if ((altapp() < 50) && 
        (speedavg() < 0.5) && (speedavg() > -0.5) &&
        (sqdiff() < 0.5)) {
        st = ACST_GROUND;
    }
    else
    if ((_dir == ACDIR_DOWN) && (altapp() < 100)) {
        st = ACST_LANDING;
    }
    else
    if ((speedapp() < -AC_SPEED_FREEFALL_I) || 
        ((_state == ACST_FREEFALL) && (speedapp() < -AC_SPEED_FREEFALL_O))) {
        st = ACST_FREEFALL;
    }
    else
    if ((speedapp() < -AC_SPEED_CANOPY_I) ||
        ((_state == ACST_CANOPY) && (speedapp() < -AC_SPEED_FLAT))) {
        st = ACST_CANOPY;
    }
    
    if (_state != st) {
        _state = st;
        _statecnt = 0;
        _statetm = 0;
    }
    
    // jmpmode - определение режима прыжка, исходя из продолжительности
    // одного и того же состояния
    // т.е. тут переключение всегда происходит с задержкой, но 
    // jmptm и jmpcnt после переключения будут всегда с учётом этой задержки
    ac_jmpmode_t jmp = _jmpmode;
    auto tint = _data[_c].interval;
    switch (_jmpmode) {
        case ACJMP_INIT:
            if (state() > ACST_INIT) {
                jmp = ACJMP_NONE;
                _jmpccnt = 0;
                _jmpctm = 0;
            }
            break;
        
        case ACJMP_NONE:
            if (state() > ACST_GROUND) {
                _jmpccnt++;
                _jmpctm += tint;
                if ((_jmpccnt >= AC_JMP_TOFF_COUNT) && (_jmpctm >= AC_JMP_TOFF_TIME))
                    jmp = ACJMP_TAKEOFF;
            }
            else
            if (_jmpccnt > 0) {
                _jmpccnt = 0;
                _jmpctm = 0;
            }
            break;
            
        case ACJMP_TAKEOFF:
            toffupdate(tinterval);
            return st;
            /*
            if ((_jmpccnt == 0) && (speedapp() < -AC_JMP_SPEED_MIN)) {
                // При скорости снижения выше пороговой
                // включаем счётчик времени прыжка
                _jmpccnt++;
                _jmpctm += tint;
            }
            if ((_jmpccnt > 0) && (
                    // После взвода счётчика на скорости AC_JMP_SPEED_MIN
                    // Нам уже достаточно удерживать минимальную скорость AC_JMP_SPEED_CANCEL
                    // в течение AC_JMP_SPEED_COUNT / AC_JMP_SPEED_TIME,
                    // а после этого времени мы уже не будем проверять скорость снижения для отмены счёта
                    // С этого момента это уже считается прыжком - осталось только выяснить, есть ли тут свободное падение
                    ((_jmpccnt >= AC_JMP_SPEED_COUNT) && (_jmpctm >= AC_JMP_SPEED_TIME)) ||
                    (speedapp() < -AC_JMP_SPEED_CANCEL)
                )) {
                _jmpccnt++;
                _jmpctm += tint;
                // выясняем, есть ли тут свободное падение
                if ((state() == ACST_FREEFALL) && 
                    (statecnt() >= AC_JMP_FF_COUNT) && (statetm() >= AC_JMP_FF_TIME))
                     // скорость достигла фрифольной и остаётся такой более 5 сек,
                    jmp = ACJMP_FREEFALL;
                else
                if ((_jmpccnt >= AC_JMP_NOFF_COUNT) && (_jmpctm >= AC_JMP_NOFF_TIME))
                    // если за 80 тиков скорость так и не достигла фрифольной,
                    // значит было открытие под бортом
                    jmp = ACJMP_CANOPY;
            }
            else
            if (_jmpccnt > 0) {
                _jmpccnt = 0;
                _jmpctm = 0;
            }
            break;
            */
            
        case ACJMP_FREEFALL:
            // Переход в режим CNP после начала прыга,
            // Дальше только окончание прыга может быть, даже если начнётся снова FF,
            // Для jmp только такой порядок переходов,
            // это гарантирует прибавление только одного прыга на счётчике при одном фактическом
            if (speedapp() >= -AC_JMP_SPEED_CANOPY) {
                _jmpccnt++;
                _jmpctm += tint;
                if ((_jmpccnt >= AC_JMP_CNP_COUNT) && (_jmpctm >= AC_JMP_CNP_TIME))
                    jmp = ACJMP_CANOPY;
            }
            else
            if (_jmpccnt > 0) {
                _jmpccnt = 0;
                _jmpctm = 0;
            }
            break;
            
        case ACJMP_CANOPY:
            if (state() == ACST_GROUND) {
                _jmpccnt++;
                _jmpctm += tint;
                if ((_jmpccnt >= AC_JMP_GND_COUNT) && (_jmpctm >= AC_JMP_GND_TIME))
                    jmp = ACJMP_NONE;
            }
            else
            if (_jmpccnt > 0) {
                _jmpccnt = 0;
                _jmpctm = 0;
            }
            break;
    }
    
    if (jmp != _jmpmode) {
        _jmpmode = jmp;
        _jmpcnt = _jmpccnt;
        _jmptm = _jmpctm;
        _jmpccnt = 0;
        _jmpctm = 0;
    }
    
    return st;
}

// профиль начала прыжка
const int8_t ffprofile[] = { -10, -23, -18, -8, -4, -2 };
#define chktresh(val,tresh) (((tresh) < 0) && ((val) <= (tresh))) || (((tresh) >= 0) && ((val) >= (tresh)))

void AltCalc::toffupdate(uint16_t tinterval) {
    if (_ffprof == 0) {
        // стартуем определять вхождение в профиль по средней скорости
        if (chktresh(speedapp(), ffprofile[0])) {
            _ffprof ++;
            _altprof = altapp();
            _ffproftm = 0;
            _ffprofcnt = 0;
        }
        return;
    }

    // теперь считаем интервал от самого первого пункта в профиле
    _ffproftm += tinterval;
    _ffprofcnt ++;

    // далее - каждые 10 тиков от старта будем проверять каждый следующий пункт профиля
    if ((_ffprofcnt/AC_JMP_PROFILE_COUNT) < _ffprof)
        return;

    if (chktresh(altapp()-_altprof, ffprofile[_ffprof])) {
        // мы всё ещё соответствуем профилю начала прыга
        _ffprof ++;

        if (_ffprof >= sizeof(ffprofile)) {
            // профиль закончился, принимаем окончательное решение
            _jmpmode = speedapp() >= -AC_JMP_SPEED_CANOPY ? ACJMP_CANOPY : ACJMP_FREEFALL;
            _jmptm = _ffproftm;
            _jmpcnt = _ffprofcnt + AC_DATA_COUNT; // скорость средняя задерживается примерно на половину-весь размер буфера
            _ffprof = 0;
            _altprof = 0;
            _ffproftm = 0;
            _ffprofcnt = 0;
        }

        _altprof = altapp();
        return;
    }

    // мы вышли за пределы профиля
    if (chktresh(speedapp(), ffprofile[0])) {
        // но мы ещё в рамках старта профиля
        _ffprof = 1;
        _altprof = altapp();
    }
    else {
        // выход из профиля полный - полный сброс процесса
        _ffprof = 0;
        _altprof = 0;
    }
    _ffproftm = 0;
    _ffprofcnt = 0;
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

void AltCalc::gndset(float press, uint16_t tinterval) {
    _pressgnd = press;
    
    if (_state == ACST_INIT) {
        for (auto &d: _data) { // забиваем весь массив текущим значением
            d.press = press;
            d.interval = tinterval;
            d.alt = 0;
        }
        _state = ACST_GROUND;
    }
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
