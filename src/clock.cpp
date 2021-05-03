
#include "clock.h"
#include "log.h"
#include "gps/proc.h"
#include "cfg/main.h"

#include "RTClib.h"

/* ------------------------------------------------------------------------------------------- *
 *  работа с часами
 * ------------------------------------------------------------------------------------------- */

static RTC_DATA_ATTR tm_t tm = { 0 };
static RTC_DATA_ATTR bool tmvalid = false;
static volatile RTC_DATA_ATTR uint32_t tmcnt = 0;

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
    //bool timeOk() { return (tmadj > 0) && ((tmadj > millis()) || ((millis()-tmadj) >= TIME_ADJUST_TIMEOUT)); }
tm_val_t tmValue() {
    auto dt = rtc.now();
    tm_val_t tmval = {
        .uts    = dt.unixtime(),
        .ms     = tmcnt * TIME_TICK_INTERVAL,
    };
    
    return tmval;
}

int32_t tmInterval(const tm_val_t &tmval) {
    auto dt = rtc.now();
    return (dt.unixtime() - tmval.uts) * 1000 + (tmcnt * TIME_TICK_INTERVAL - tmval.ms);
}

// обновление времени
// при аппаратных часах - по прерыванию
// при millis-часах - в clockProcess()
#if HWVER >= 3
static volatile bool istick = false;
static void IRAM_ATTR  clockTick() { istick = true; tmcnt=0; }
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
    CONSOLE("tm size: %d", sizeof(tm));
    
    pinMode(CLOCK_PIN_INT, INPUT_PULLUP);  // set up interrupt pin, turn on pullup resistors
    // attach interrupt
    attachInterrupt(digitalPinToInterrupt(CLOCK_PIN_INT), clockTick, RISING);
    
    CONSOLE("clock init ok, lostPower: %d, isrunning: %d", rtc.lostPower(), rtc.isrunning());
    rtc.start();
    rtc.writeSqwPinMode(PCF8563_SquareWave1Hz);
#else
    tm.valid = false;
#endif
}

static uint32_t adj = 0;
void clockForceAdjust() {
    adj = 0;
}

void clockProcess() {
    if ((millis() >= adj) || !tmvalid) {
        auto &gps = gpsInf();
        if (GPS_VALID_TIME(gps)) {
            // set the Time to the latest GPS reading
            DateTime dtgps(gps.tm.year, gps.tm.mon, gps.tm.day, gps.tm.h, gps.tm.m, gps.tm.s);
            rtc.adjust(DateTime(dtgps.unixtime() + cfg.d().timezone * 60));
            CONSOLE("time ajust - gps: %d.%02d.%d %2d:%02d:%02d, tm: %d.%02d.%d %2d:%02d:%02d / %d", 
                gps.tm.year, gps.tm.mon, gps.tm.day, gps.tm.h, gps.tm.m, gps.tm.s,
                tm.year, tm.mon, tm.day, tm.h, tm.m, tm.s, tmvalid);
            adj = millis() + TIME_ADJUST_INTERVAL;
#if HWVER >= 3
            istick = true;
#endif
        }
    }

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
        tmcnt++;
        tm.cs = tmcnt*TIME_TICK_INTERVAL / 10;
    }
}
