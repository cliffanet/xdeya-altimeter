
#include "clock.h"
#include "log.h"
#include "gps/proc.h"
#include "cfg/main.h"
#include "power.h"

#include "RTClib.h"

#include "soc/rtc.h"
extern "C" {
  #include <esp32/clk.h>
}

#include "esp32-hal-gpio.h" // OPEN_DRAIN

/* ------------------------------------------------------------------------------------------- *
 *  RTC-таймер, который не сбрасывается при deep sleep
 * ------------------------------------------------------------------------------------------- */
uint64_t utm() {
    return rtc_time_slowclk_to_us(rtc_time_get(), esp_clk_slowclk_cal_get());
}

/*
uint64_t utm_diff(uint64_t prev, uint64_t &curr) {
    curr = utm();
    return curr-prev;
}

uint64_t utm_diff(uint64_t prev) {
    uint64_t curr;
    return utm_diff(prev, curr);
}

uint32_t utm_diff32(uint32_t prev, uint32_t &curr) {
    curr = utm();
    return curr-prev;
}

uint32_t utm_diff32(uint32_t prev) {
    uint32_t curr;
    return utm_diff32(prev, curr);
}
*/

/*
    Есть проблема с отсчитыванием интервалов с использованием rtc_time.
    Само время в чистом виде не считается, но есть два параметра:
        1. Счётчик тиков таймера: rtc_time_get
        2. Калибровочное значение на данный момент: esp_clk_slowclk_cal_get
    Из двух этих параметров и получается в итоге время в микросекундах
    
    Счётчик rtc_time_get всегда последователен,
    однако esp_clk_slowclk_cal_get постоянно плавает, и при измерении времени
    с интервалом ~ 100мс может так случится, что предыдущее значение в мкс больше текущего
    Разница обычно не более 20000 мкс (20мс), но интервал получается отрицательным.

    В основном это происходит при использовании light sleep
    
    Поэтому, для измерения интервалов времени усложним логику.
    В качестве опорной величины будем использовать не время в мкс, а тики rtc_time_get.
    И уже к разнице тиков будем применять поправку esp_clk_slowclk_cal_get
*/

uint64_t utick() {
    return rtc_time_get();
}


uint64_t utm_diff(uint64_t utick_prev, uint64_t &utick_curr) {
    utick_curr = utick();
    return rtc_time_slowclk_to_us(utick_curr-utick_prev, esp_clk_slowclk_cal_get());
}

uint64_t utm_diff(uint64_t utick_prev) {
    uint64_t utick_curr;
    return utm_diff(utick_prev, utick_curr);
}

uint32_t utm_diff32(uint32_t utick_prev, uint32_t &utick_curr) {
    utick_curr = utick();
    return rtc_time_slowclk_to_us(utick_curr-utick_prev, esp_clk_slowclk_cal_get());
}

uint32_t utm_diff32(uint32_t utick_prev) {
    uint32_t utick_curr;
    return utm_diff32(utick_prev, utick_curr);
}

/* ------------------------------------------------------------------------------------------- *
 *  время-считающие таймеры
 * ------------------------------------------------------------------------------------------- */
static RTC_DATA_ATTR uint64_t utm1 = 0;
static RTC_DATA_ATTR struct {
        uint64_t tm;
        bool en;
    } tmcnt[] = { { 0 }, { 0 } };

void tmcntReset(tm_counter_t id, bool enable) {
    if ((id < 0) || (id >= TMCNT_FAIL))
        return;
    auto &t = tmcnt[id];
    t.en = enable;
    t.tm = 0;
}
bool tmcntEnabled(tm_counter_t id) {
    if ((id < 0) || (id >= TMCNT_FAIL))
        return false;

    return tmcnt[id].en;
}
uint32_t tmcntInterval(tm_counter_t id) {
    if ((id < 0) || (id >= TMCNT_FAIL))
        return 0;

    return tmcnt[id].tm / 1000000;
}
void tmcntUpdate() {
    uint64_t utm2 = utm();
    uint64_t tmdiff = utm2 - utm1;
    utm1 = utm2;
    
    for (auto &t : tmcnt)
        if (t.en)
            t.tm += tmdiff;
    
    if ((cfg.d().hrpwrafton > 0) && 
        (tmcnt[TMCNT_UPTIME].tm > (cfg.d().hrpwrafton * 3600 * 1000000))) {
        CONSOLE("pwrOff by %d hour after pwrOn", cfg.d().hrpwrafton);
        pwrOff();
    }
    
    if ((cfg.d().hrpwrnofly > 0) && 
        (tmcnt[TMCNT_NOFLY].tm > (cfg.d().hrpwrnofly * 3600 * 1000000))) {
        CONSOLE("pwrOff by %d hour no fly", cfg.d().hrpwrnofly);
        pwrOff();
    }
}

/* ------------------------------------------------------------------------------------------- *
 *  работа с часами
 * ------------------------------------------------------------------------------------------- */

static RTC_DATA_ATTR tm_t tm = { 0 };
static RTC_DATA_ATTR bool tmvalid = false;

#if HWVER >= 3
static RTC_PCF8563 rtc;
#else
static RTC_Millis rtc;
#endif

/* ------------------------------------------------------------------------------------------- *
 *  
 * ------------------------------------------------------------------------------------------- */

tm_t &tmNow() { return tm; }
bool tmValid() { return tmvalid; }

tm_t tmNow(uint32_t earlerms) {
    if (earlerms == 0)
        return tm;
    
    DateTime dt1(tm.year, tm.mon, tm.day, tm.h, tm.m, tm.s);
    uint32_t ut = dt1.unixtime() - (earlerms / 1000);
    
    int8_t tick = (earlerms % 1000) / TIME_TICK_INTERVAL;
    tick = tm.tick - tick;
    
    ut += tick * TIME_TICK_INTERVAL / 1000;
    tick = tick % (1000 / TIME_TICK_INTERVAL);
    if (tick < 0) {
        ut --;
        tick += 100;
    }
    
    DateTime dt(ut);
    return { 
        year: dt.year(),
        mon : dt.month(),
        day : dt.day(),
        h   : dt.hour(),
        m   : dt.minute(),
        s   : dt.second(),
        tick: tick
    };
}

int32_t tmInterval(const tm_t &tmbeg, const tm_t &tmend) {
    DateTime
        dtbeg(tmbeg.year, tmbeg.mon, tmbeg.day, tmbeg.h, tmbeg.m, tmbeg.s),
        dtend(tmend.year, tmend.mon, tmend.day, tmend.h, tmend.m, tmend.s);
    
    int32_t tint = (dtend.unixtime() - dtbeg.unixtime()) * 1000;
    int8_t tick1 = tmend.tick - tmbeg.tick;
    int16_t tick = tick1;
    return tint + (tick * TIME_TICK_INTERVAL);
}

int32_t tmIntervalToNow(const tm_t &tmbeg) {
    return tmInterval(tmbeg, tm);
}

// обновление времени
// при аппаратных часах - по прерыванию
// при millis-часах - в clockProcess()
#if HWVER >= 3
static volatile bool istick = false;
static void IRAM_ATTR  clockTick() { istick = true; tm.tick = 0; }
#endif
static void clockUpd() {
    auto dt = rtc.now();
    tm.year = dt.year();
    tm.mon  = dt.month();
    tm.day  = dt.day();
    tm.h    = dt.hour();
    tm.m    = dt.minute();
    tm.s    = dt.second();
    tm.tick = 0;
    
    tmvalid = dt.isValid()
#if HWVER >= 3
                && !rtc.lostPower()
#endif
    ;
}

void clockInit() {
#if HWVER >= 3
    if (! rtc.begin()) {
      CONSOLE("Couldn't find RTC");
      return;
    }
    
    CONSOLE("clock init ok, lostPower: %d, isrunning: %d", rtc.lostPower(), rtc.isrunning());
    rtc.start();
    clockIntEnable();
#else
    tmvalid = false;
#endif
}

#if HWVER >= 3
void clockIntEnable() {
    rtc.writeSqwPinMode(PCF8563_SquareWave1Hz);
    
    pinMode(CLOCK_PIN_INT, INPUT_PULLUP);  // set up interrupt pin, turn on pullup resistors
    // attach interrupt
    attachInterrupt(digitalPinToInterrupt(CLOCK_PIN_INT), clockTick, RISING);
}
void clockIntDisable() {
    detachInterrupt(digitalPinToInterrupt(CLOCK_PIN_INT));
    pinMode(CLOCK_PIN_INT, OPEN_DRAIN);
    rtc.writeSqwPinMode(PCF8563_SquareWaveOFF);
}
#endif

static uint16_t adj = 0;
void clockForceAdjust() {
    adj = TIME_ADJUST_INTERVAL;
}

void clockProcess() {
    if ((adj >= TIME_ADJUST_INTERVAL) || !tmvalid) {
        auto &gps = gpsInf();
        if (GPS_VALID_TIME(gps)) {
            // set the Time to the latest GPS reading
            DateTime dtgps(gps.tm.year, gps.tm.mon, gps.tm.day, gps.tm.h, gps.tm.m, gps.tm.s);
            rtc.adjust(DateTime(dtgps.unixtime() + cfg.d().timezone * 60));
            CONSOLE("time ajust - gps: %d.%02d.%d %2d:%02d:%02d, tm: %d.%02d.%d %2d:%02d:%02d / %d", 
                gps.tm.year, gps.tm.mon, gps.tm.day, gps.tm.h, gps.tm.m, gps.tm.s,
                tm.year, tm.mon, tm.day, tm.h, tm.m, tm.s, tmvalid);
            adj = 0;
#if HWVER >= 3
            istick = true;
#endif
        }
    }
    else
        adj++;

#if HWVER >= 3
    if (istick) {
#endif
        clockUpd();
#if HWVER >= 3
        istick = false;
    }
    else
#endif
    {
        tm.tick ++;
    }
}
