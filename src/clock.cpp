
#include "clock.h"
#include "log.h"
#include "gps/proc.h"
#include "cfg/main.h"

#include "RTClib.h"

#include "soc/rtc.h"
extern "C" {
  #include <esp_clk.h>
}

/* ------------------------------------------------------------------------------------------- *
 *  RTC-таймер, который не сбрасывается при deep sleep
 * ------------------------------------------------------------------------------------------- */
uint64_t utm() {
    return rtc_time_slowclk_to_us(rtc_time_get(), esp_clk_slowclk_cal_get());
}

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
    
    int8_t cs = (earlerms % 1000) / 10;
    cs = tm.cs - cs;
    
    ut += cs / 100;
    cs = cs % 100;
    if (cs < 0) {
        ut --;
        cs += 100;
    }
    
    DateTime dt(ut);
    return { 
        year: dt.year(),
        mon : dt.month(),
        day : dt.day(),
        h   : dt.hour(),
        m   : dt.minute(),
        s   : dt.second(),
        cs  : cs
    };
}

int32_t tmInterval(const tm_t &tmbeg, const tm_t &tmend) {
    DateTime
        dtbeg(tmbeg.year, tmbeg.mon, tmbeg.day, tmbeg.h, tmbeg.m, tmbeg.s),
        dtend(tmend.year, tmend.mon, tmend.day, tmend.h, tmend.m, tmend.s);
    
    int32_t tint = (dtend.unixtime() - dtbeg.unixtime()) * 1000;
    int8_t cs1 = tmend.cs - tmbeg.cs;
    int16_t cs = cs1;
    return tint + (cs*10);
}

int32_t tmIntervalToNow(const tm_t &tmbeg) {
    return tmInterval(tmbeg, tm);
}

// обновление времени
// при аппаратных часах - по прерыванию
// при millis-часах - в clockProcess()
#if HWVER >= 3
static volatile bool istick = false;
static void IRAM_ATTR  clockTick() { istick = true; tm.cs = 0; }
#endif
static void clockUpd() {
    auto dt = rtc.now();
    tm.year = dt.year();
    tm.mon  = dt.month();
    tm.day  = dt.day();
    tm.h    = dt.hour();
    tm.m    = dt.minute();
    tm.s    = dt.second();
    tm.cs   = 0;
    
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
    
    pinMode(CLOCK_PIN_INT, INPUT_PULLUP);  // set up interrupt pin, turn on pullup resistors
    // attach interrupt
    attachInterrupt(digitalPinToInterrupt(CLOCK_PIN_INT), clockTick, RISING);
    
    CONSOLE("clock init ok, lostPower: %d, isrunning: %d", rtc.lostPower(), rtc.isrunning());
    rtc.start();
    rtc.writeSqwPinMode(PCF8563_SquareWave1Hz);
#else
    tmvalid = false;
#endif
}

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
        tm.cs += TIME_TICK_INTERVAL / 10;
    }
}
