
#include "proc.h"
#include "../log.h"
#include "../cfg/main.h"
#include "../gps/proc.h"
#include "../file/track.h"
#include "../view/base.h"   // btnPushed()
#include "../power.h"       // pwrBattValue()

#include <Adafruit_BMP280.h>

#if HWVER <= 1
static Adafruit_BMP280 bmp; // hardware Wire
#else
static Adafruit_BMP280 bmp(5); // hardware SPI on IO5
#endif

static AltCalc ac;

static log_item_t logall[JMP_PRELOG_SIZE] = { { 0 } };
static uint16_t logcur = 0xffff;

/* ------------------------------------------------------------------------------------------- *
 *  Обработка изменения режима высотомера
 * ------------------------------------------------------------------------------------------- */
static void altState(ac_state_t prev, ac_state_t curr) {
    if ((prev == ACST_FREEFALL) && (curr != ACST_FREEFALL) && cfg.d().dsplautoff) {
        // Восстанавливаем обработчики после принудительного FF-режима
        setViewMain();
    }
    
    switch (curr) {
        case ACST_GROUND:
            setViewMain(cfg.d().dsplland);
            break;
        
        case ACST_FREEFALL:
            if (cfg.d().dsplautoff)
                setViewMain(MODE_MAIN_ALT, false);
            break;
        
        case ACST_CANOPY:
            setViewMain(cfg.d().dsplcnp, false);
            break;
    }
}

/* ------------------------------------------------------------------------------------------- *
 * Базовые функции
 * ------------------------------------------------------------------------------------------- */

AltCalc & altCalc() {
    return ac;
}

/* ------------------------------------------------------------------------------------------- *
 *  Сбор текущих данных
 * ------------------------------------------------------------------------------------------- */
static void jmpPreLogAdd(uint16_t interval) {
    auto &gps = gpsInf();
    
    logcur ++;
    if (logcur >= JMP_PRELOG_SIZE)
        logcur = 0;
    auto &li = logall[logcur];
    
    li = {
        tmoffset    : interval,
        flags       : 0,
        state       : 'U',
        direct      : 'U',
        alt         : ac.alt(),
        altspeed    : ac.speedapp()*100,
        lon         : gps.lon,
        lat         : gps.lat,
        hspeed      : gps.gSpeed,
        heading     : GPS_DEG(gps.heading),
        gpsalt      : GPS_MM(gps.hMSL),
        vspeed      : gps.speed,
        gpsdage     : gpsDataAge(),
        sat         : gps.numSV,
        _           : 0,
#if HWVER > 1
        batval      : pwrBattValue(),
#else
        batval      : 0,
#endif
        hAcc        : gps.hAcc,
        vAcc        : gps.vAcc,
        sAcc        : gps.sAcc,
        cAcc        : gps.cAcc,
        //tm          : gps.tm,
        millis      : millis(),
    };
    
    if (GPS_VALID(gps))
        li.flags |= LI_FLAG_GPS_VALID;
    if (GPS_VALID_LOCATION(gps))
        li.flags |= LI_FLAG_GPS_VLOC;
    if (GPS_VALID_VERTICAL(gps))
        li.flags |= LI_FLAG_GPS_VVERT;
    if (GPS_VALID_SPEED(gps))
        li.flags |= LI_FLAG_GPS_VSPEED;
    if (GPS_VALID_HEAD(gps))
        li.flags |= LI_FLAG_GPS_VHEAD;
    if (GPS_VALID_TIME(gps))
        li.flags |= LI_FLAG_GPS_VTIME;
    
    if (btnPushed(BTN_UP))
        li.flags |= LI_FLAG_BTN_UP;
    if (btnPushed(BTN_SEL))
        li.flags |= LI_FLAG_BTN_SEL;
    if (btnPushed(BTN_DOWN))
        li.flags |= LI_FLAG_BTN_DOWN;
    
    switch (ac.state()) {
        case ACST_INIT:         li.state = 'i'; break;
        case ACST_GROUND:       li.state = 'g'; break;
        case ACST_TAKEOFF40:    li.state = 's'; break;
        case ACST_TAKEOFF:      li.state = 't'; break;
        case ACST_FREEFALL:     li.state = 'f'; break;
        case ACST_CANOPY:       li.state = 'c'; break;
        case ACST_LANDING:      li.state = 'l'; break;
    }
    switch (ac.direct()) {
        case ACDIR_INIT:        li.direct = 'i'; break;
        case ACDIR_ERR:         li.direct = 'e'; break;
        case ACDIR_UP:          li.direct = 'u'; break;
        case ACDIR_FLAT:        li.direct = 'f'; break;
        case ACDIR_DOWN:        li.direct = 'd'; break;
    }
}

/* ------------------------------------------------------------------------------------------- *
 *  Получение данных по old-индексу (old=0 - текущие данные, old=1 - на 1 тик ранее, ...)
 * ------------------------------------------------------------------------------------------- */
static uint16_t old2index(uint16_t old) {
    if (old >= JMP_PRELOG_SIZE) {
        uint16_t ind = logcur+1;
        return ind >= JMP_PRELOG_SIZE ? 0 : ind;
    }
    
    uint16_t ind = logcur;
    if (ind < old) {
        old -= ind+1;
        ind = JMP_PRELOG_SIZE-1;
    }
    return ind - old;
}

const log_item_t &jmpPreLog(uint16_t old) {
    return logall[old2index(old)];
}

uint32_t jmpPreLogInterval(uint16_t old) {
    if (old >= JMP_PRELOG_SIZE)
        old = JMP_PRELOG_SIZE-1;
    uint16_t cur = logcur;
    uint32_t interval = 0;
    
    while (old > 0) {
        // интервал до текущего (old = 0) равен 0
        // до предыдущего (old = 1) = tmoffset текущего и т.д.
        interval += logall[cur].tmoffset;
        old --;
        cur = cur > 0 ? cur-1 : JMP_PRELOG_SIZE-1;
    }
    
    return interval;
}

/* ------------------------------------------------------------------------------------------- *
 *  Получение данных, начиная от текущего положения
 * ------------------------------------------------------------------------------------------- */
uint16_t jmpPreLogFirst(log_item_t *li) {
    if (li != NULL)
        *li = logall[logcur];
    return logcur;
}
// При обращении к jmpPreLogNext по индексу полученному ранее из jmpPreLogFirst,
// пока next не вернёт false, мы гаранируем, что проёдёмся по всем данным, не пропустив ничего 
bool jmpPreLogNext(uint16_t &cursor, log_item_t *li) {
    if (cursor == logcur)
        return false;
    
    cursor ++;
    if (cursor >= JMP_PRELOG_SIZE)
        cursor = 0;
    if (li != NULL)
        *li = logall[cursor];
    return true;
}

/* ------------------------------------------------------------------------------------------- *
 *  Инициализация
 * ------------------------------------------------------------------------------------------- */
void jmpInit() {
    if (!bmp.begin(BMP280_ADDRESS_ALT)) {   
        CONSOLE("Could not find a valid BMP280 sensor, check wiring!");
    }
}

/* ------------------------------------------------------------------------------------------- *
 *  Определяем текущее состояние прыга и переключаем по необходимости
 * ------------------------------------------------------------------------------------------- */
void jmpProcess() {
    static uint32_t _mill = millis();
    uint32_t m = millis();
    ac.tick(bmp.readPressure(), m-_mill);
    jmpPreLogAdd(m-_mill);
    _mill = m;
    
    // Автокорректировка нуля
    if (cfg.d().gndauto &&
        (ac.state() == ACST_GROUND) &&
        (ac.direct() == ACDIR_FLAT) &&
        (ac.dirtm() >= ALT_AUTOGND_INTERVAL)) {
        ac.gndreset();
        ac.dirreset();
        CONSOLE("auto GND reseted");
    }
    
    // Обработка изменения режима высотомера
    static auto altstate = ac.state();
    if (altstate != ac.state())
        altState(altstate, ac.state());
    altstate = ac.state();
    
    // Определение старта прыжка
    static enum {
        JMP_NONE,
        JMP_FREEFALL,
        JMP_CANOPY
    } jmpst = JMP_NONE;
    
    if (jmpst == JMP_NONE) {
        static uint32_t dncnt = 0;
        if (dncnt == 0) {
            if (ac.speedapp() < -JMP_SPEED_MIN)     // При скорости снижения выше пороговой
                dncnt ++;                           // включаем счётчик тиков, пока это скорость сохраняется
        }
        else
        if (dncnt < JMP_SPEED_COUNT) {
            // В зоне времени до JMP_SPEED_COUNT
            // проверяем на выход из диапазона скорости снижения JMP_SPEED_CANCEL
            if (ac.speedapp() < -JMP_SPEED_CANCEL)  // При сохранении скорости снижения, увеличиваем счётчик тиков
                dncnt ++;
            else
                dncnt = 0;                          // либо сбрасываем его
            // По окончании времени JMP_SPEED_COUNT мы уже не будем проверять скорость снижения
            // по порогам JMP_SPEED_MIN и JMP_SPEED_CANCEL
            // С этого момента это уже считается прыжком - осталось только выяснить, есть ли тут свободное падение
        }
        else
            // Как только приняли решение, что это всё-таки прыг, просто отсчитываем
            // dncnt дальше, пока не примем решение, есть ли в этом прыге FF, чтобы уже стартовать сам прыг,
            // При этом нам понадобится dncnt, стобы получить момент старта
            dncnt ++;
        
        // счётчик тиков увеличивается в промежуточном состоянии, когда мы ещё не определили,
        // начался ли прыжок, но вертикальная скорость уже достаточно высокая - возможно,
        // это кратковременное снижение ЛА
        
        if (
            (dncnt > 0) && (                        // если идёт отсчёт промежуточного состояния и при этом
                ((ac.state() == ACST_FREEFALL) && (ac.statecnt() >= 50)) ||    // скорость достигла фрифольной и остаётся такой более 5 сек,
                (dncnt >= 80)                       // либо пороговая скорость сохраняется 80 тиков (8 сек) - 
            )) {                                    // считаем, что прыг начался
            // Момент начала отсчёта dncnt начинается с превышения скорости снижения JMP_SPEED_MIN,
            // Однако, это не сам момент отделения, а чуть позже - на JMP_SPEED_PREFIX тиков,
            // прибавляем их к dncnt
            dncnt += JMP_SPEED_PREFIX;
            
            // для удобства отладки помечаем текущий jmpPreLog
            // флагом LI_FLAG_JMPDECISS - момент принятия решения о начале прыжка
            logall[logcur].flags            |= LI_FLAG_JMPDECISS;
            // А момент, который мы считаем отделением - флагом LI_FLAG_JMPBEG
            logall[old2index(dncnt)].flags  |= LI_FLAG_JMPBEG;
            
            if (ac.state() == ACST_FREEFALL) {
                // если скорость всё же успела достигнуть фрифольной,
                // значит, фрифол считаем с самого начала снижения
                // и до текущего момента, пока скорость не снизится до ACST_CANOPY
                jmpst = JMP_FREEFALL;
                jmp.beg(dncnt);
            }
            else {
                // если за 80 тиков скорость так и не достигла фрифольной,
                // значит было открытие под бортом
                jmpst = JMP_CANOPY;
                logall[old2index(dncnt)].flags  |= LI_FLAG_JMPCNP;
                jmp.cnp(dncnt);
            }
            // после установки jmpst далее просто контролируем режим ac.state()

            // Включаем запись лога прыга
            if (!trkRunning())
                // Временно сделаем, чтобы при автозапуске трека, он начинал писать чуть заранее до отделения
                trkStart(false, dncnt+50);
            
            dncnt = 0;                              // при старте лога прыга обнуляем счётчик тиков пограничного состояния,
                                                    // он нам больше не нужен, чтобы в след раз не было ложных срабатываний
        }
    }
    
    if ((jmpst == JMP_FREEFALL) && (ac.state() == ACST_CANOPY)) {
        // Переход в режим CNP после начала прыга,
        // Дальше только окончание прыга может быть, даже если начнётся снова FF,
        // Для jmp только такой порядок переходов,
        // это гарантирует прибавление только одного прыга на счётчике при одном фактическом
        jmpst = JMP_CANOPY;
        logall[logcur].flags  |= LI_FLAG_JMPCNP;
        
        // Сохраняем промежуточный результат
        jmp.cnp();
    }
    
    if ((jmpst > JMP_NONE) && (
            (ac.state() == ACST_GROUND) ||
            (jmp.state() == LOGJMP_NONE)
        )) {
        // Прыг закончился совсем, сохраняем результат
        jmpst = JMP_NONE;
        logall[logcur].flags  |= LI_FLAG_JMPEND;
        
        if (trkState() == TRKRUN_AUTO)
            trkStop();
        
        // Сохраняем
        jmp.end();
    }
}

/* ------------------------------------------------------------------------------------------- *
 *  Принудительный сброс текущего прыга
 * ------------------------------------------------------------------------------------------- */
void jmpReset() {
    if (jmp.state() != LOGJMP_NONE)
        jmp.end();
}
