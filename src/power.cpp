
#include "power.h"
#include "log.h"
#include "clock.h"
#include "view/base.h"
#include "view/menu.h"
#include "navi/proc.h"
#include "navi/compass.h"
#include "jump/proc.h"
#include "jump/track.h"
#include "core/worker.h"
#include "cfg/main.h"


static RTC_DATA_ATTR power_mode_t mode = PWR_ACTIVE;

// Уход в сон без таймера - pwroff
static void pwrDeepSleep() {
  /*
    First we configure the wake up source
    We set our ESP32 to wake up for an external trigger.
    There are two types for ESP32, ext0 and ext1 .
    ext0 uses RTC_IO to wakeup thus requires RTC peripherals
    to be on while ext1 uses RTC Controller so doesnt need
    peripherals to be powered on.
    Note that using internal pullups/pulldowns also requires
    RTC peripherals to be turned on.
    */
    esp_sleep_enable_ext0_wakeup(BUTTON_GPIO_PWR, LOW); //1 = High, 0 = Low

    //If you were to use ext1, you would use it like
    //esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK,ESP_EXT1_WAKEUP_ANY_HIGH);

    //Go to sleep now
    CONSOLE("Going to deep sleep now");
    esp_deep_sleep_start();
    CONSOLE("This will never be printed");
}

// Уход в сон с таймером - sleep
static void pwrDeepSleep(uint64_t timer) {
    esp_sleep_enable_ext0_wakeup(BUTTON_GPIO_PWR, LOW); //1 = High, 0 = Low
    
    if (timer > 0)
        esp_sleep_enable_timer_wakeup(timer);

    //Go to sleep now
    CONSOLE("Going to deep sleep now");
    esp_deep_sleep_start();
    CONSOLE("This will never be printed");
}

#if HWVER >= 3
static bool pwrBattSleepChg(uint32_t &tm) {
    pinMode(HWPOWER_PIN_BATIN, INPUT);
    pinMode(HWPOWER_PIN_BATCHRG, INPUT_PULLUP);
    
    auto draw = [](U8G2 &u8g2) {
        u8g2.setFont(u8g2_font_battery24_tr);
        uint8_t blev = pwrBattLevel();
        if (blev > 0) blev--;
        u8g2.drawGlyph((u8g2.getDisplayWidth()-20)/2, u8g2.getDisplayHeight()/2 + 24/2, 0x30+blev);
        
        if (pwrBattCharge()) {
            u8g2.setFont(u8g2_font_open_iconic_embedded_1x_t);
            u8g2.drawGlyph((u8g2.getDisplayWidth()-20)/2+22, u8g2.getDisplayHeight()/2 + 24/2, 'C');
        }
    };
    displayDraw(draw, true, true);
    
    while (1) {
        delay(100);
        viewProcess();
        displayDraw(draw);
        
        uint8_t bm = 0;
        tm = btnPressed(bm);
        if (bm != btnMask(BTN_SEL))
            return false;
        if (tm > 4000)
            return true;
        //CONSOLE("wait: %d (bm: %d)", tm, bm);
    }
    
    CONSOLE("pwr off");
    displayOff();
    gpsPwrDown();
    pwrDeepSleep();
    
    return false;
}
#endif

/* ------------------------------------------------------------------------------------------- *
 *  
 * ------------------------------------------------------------------------------------------- */

power_mode_t pwrMode() {
    return mode;
}

bool pwrInit() {
    pwrBattChk(true);
    
    auto wakeup_reason = esp_sleep_get_wakeup_cause();
    switch(wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0 : CONSOLE("Wakeup caused by external signal using RTC_IO"); break;
        case ESP_SLEEP_WAKEUP_EXT1 : CONSOLE("Wakeup caused by external signal using RTC_CNTL"); break;
        case ESP_SLEEP_WAKEUP_TIMER : CONSOLE("Wakeup caused by timer"); break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD : CONSOLE("Wakeup caused by touchpad"); break;
        case ESP_SLEEP_WAKEUP_ULP : CONSOLE("Wakeup caused by ULP program"); break;
        default : CONSOLE("Wakeup was not caused by deep sleep: %d", wakeup_reason); break;
    }
    
    if (wakeup_reason == 0)
        // если была перезагрузка по питанию, то просто включаемся
        return true;
    
    switch (mode) {
        case PWR_OFF:
            {   // при состоянии "выкл" проверяем состояние кнопок, надо удержать 4 сек кнопку 
                btnInit();
                uint8_t bm = 0;
                uint32_t tm = btnPressed(bm);
                CONSOLE("btn [%d] pressed %d ms", bm, tm);
                if (bm == btnMask(BTN_SEL)) {
                    // вывод "батарея заряжается" при необходимости
                    if (!pwrBattSleepChg(tm))
                        pwrDeepSleep(1000000);
                    
                    // Если держим достаточно долго, запускаемся
                    if (tm > 1200) {
                        CONSOLE("pwrInit on");
                        return true;
                    }
                    else
                        pwrDeepSleep(1000000);
                }
            }
            break;
            
        case PWR_SLEEP:
            if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
                btnInit();
                CONSOLE("pwrInit resume from sleep by btn");
                return true;
            }
            else
            if (jmpTakeoffCheck()) {
                CONSOLE("pwrInit resume from sleep by Takeoff");
                return true;
            }
            else {
                tmcntUpdate();
                pwrDeepSleep(1000000);
            }
            
            return false;
        
        default:
            // если при загрузке обнаружили, что текущее состояние - "вкл", то включаемся
            return true;
    }

    CONSOLE("pwrInit off");
    
    pwrDeepSleep();
    return false;
}

static power_mode_t pwrModeCalc() {
    if (altCalc().state() != ACST_GROUND)
        return PWR_ACTIVE;
    if (gpsPwr())
        return PWR_ACTIVE;
    if (menuIsWifi())
        return PWR_ACTIVE;
#if HWVER >= 5
    if (menuIsFwSd())
        return PWR_ACTIVE;
#endif // if HWVER >= 5
    if (!wrkEmpty())
        // Если есть какие-либо выполняемые процессы, надо чтобы они завершились.
        // Иначе, они просто не будут выполняться.
        // PWR_ACTIVE - паузы заполняются работой worker
        // PWR_PASSIVE - паузу делает esp_light_sleep, а worker просто не будет выполняться
        // Это надо: для записи трека (чтобы он продолжал писаться и мог завершиться)
        // Но почему-то мы отсюда вырезали этот код совсем недавно,
        // видимо в каком-то случае оно запрещает уходить в сон.
        return PWR_ACTIVE;
    
    if (btnIdle() < PWR_SLEEP_TIMEOUT)
        return PWR_PASSIVE;
    
    return PWR_SLEEP;
}

void pwrModeUpd() {
    auto m = pwrModeCalc();
    if (mode == m)
        return;
    
    CONSOLE("[change] %d => %d", mode, m);
    
    mode = m;
    
    if (m < PWR_ACTIVE) {
        if (getCpuFrequencyMhz() != 80) {
            setCpuFrequencyMhz(80);
            CONSOLE("change CPU Freq to: %lu MHz", getCpuFrequencyMhz());
        }
    }
    else {
        if (getCpuFrequencyMhz() != 240) {
            setCpuFrequencyMhz(240);
            CONSOLE("change CPU Freq to: %lu MHz", getCpuFrequencyMhz());
        }
    }
    CONSOLE("CPU Freq = %lu MHz; XTAL Freq = %lu MHz; APB Freq = %lu Hz",
            getCpuFrequencyMhz(),
            getXtalFrequencyMhz(),
            getApbFrequency());
}

void pwrRun(void (*run)()) {
    uint64_t u = utick();
    
    if (run != NULL)
        run();
    
    pwrModeUpd();
    
    switch (mode) {
        case PWR_OFF:
            pwrOff();
            return;
        
        case PWR_SLEEP:
            pwrSleep();
            break;
            
        case PWR_PASSIVE:
            u = utm_diff(u);
            if (u < 99000) {
                esp_sleep_enable_timer_wakeup(100000-u);
                esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
                esp_light_sleep_start();
            }
            break;
            
        case PWR_ACTIVE:
            {
                uint64_t u1 = utm_diff(u);
                if (u1 < 90000)
                    wrkProcess((100000-u1) / 1000);
            }
            u = utm_diff(u);
            if (u < 99000)
                delay((100000-u) / 1000);
            break;
    }
}

void pwrSleep() {
    mode = PWR_SLEEP;
    
    displayOff();
    gpsPwrDown();
    compStop();
    setCpuFrequencyMhz(10);
    
    CONSOLE("pwr sleep");
    pwrDeepSleep(1000000);
}

void pwrOff() {
    mode = PWR_OFF;
    
    jmpStop();
    displayOff();
    gpsPwrDown();
    compStop();
    setCpuFrequencyMhz(10);
    
    // перед тем, как уйти в сон окончательно, дождёмся отпускания кнопки питания
    while (digitalRead(BUTTON_GPIO_PWR) == LOW)
        delay(100);
    
    CONSOLE("pwr off");
    pwrDeepSleep();
}

#if HWVER > 1
bool pwrBattChk(bool init, double val) {
    if (init)
        pinMode(HWPOWER_PIN_BATIN, INPUT);
    
    auto cur = pwrBattValue();
    if (cur >= val)
        return true;
    
    if (cur < 1) {
        // Эта величина означает, что скорее всего мы питаемся
        // от другого источника питания, либо есть проблема контроля
        // напряжения питания
        // ибо от такого низкого напряжения esp работать не может
        CONSOLE("battery value wrong: %0.2f", cur);
        return true;
    }
    
    CONSOLE("battery low");
    
    auto draw = [](U8G2 &u8g2) {
        u8g2.setFont(u8g2_font_open_iconic_embedded_8x_t);
        u8g2.drawGlyph((u8g2.getDisplayWidth()-56)/2, u8g2.getDisplayHeight()/2 + 56/2, 0x40);
    };
    displayDraw(draw, true, init);
    delay(500);
    displayDraw(NULL, true);
    delay(500);
    displayDraw(draw, true);
    delay(2000);
    
    CONSOLE("pwr off");
    displayOff();
    gpsPwrDown();
    pwrDeepSleep();
    
    return false;
}

static double fmap(uint32_t in, uint32_t in_min, uint32_t in_max, double out_min, double out_max) {
    return  (out_max - out_min) * (in - in_min) / (in_max - in_min) + out_min;
}

uint16_t pwrBattRaw() {
    return analogRead(HWPOWER_PIN_BATIN);
}
double pwrBattValue() {
    //return 3.35 * pwrBattRaw() / 4095 * 3 / 2;
    return fmap(pwrBattRaw(), 0x0000, 0x0fff, 0.12, 3.35) * 3 / 2;
    /*
        raw 2400 = 3.02v
        raw 2500 = 3.14v
        raw 2600 = 3.26v
        raw 2700 = 3.37v
        raw 2800 = 3.49v
        raw 2900 = 3.61v
        raw 3000 = 3.73v
        raw 3100 = 3.85v
        raw 3200 = 3.97v
        raw 3300 = 4.08v
        raw 3400 = 4.20v
    */
}

uint8_t pwrBattLevel() {
    uint16_t bv = pwrBattRaw();
    
    return
        bv > 3150 ? 5 :
        bv > 3050 ? 4 :
        bv > 2950 ? 3 :
        bv > 2850 ? 2 :
        bv > 2750 ? 1 :
        0;
}
#endif

#if HWVER >= 3
bool pwrBattCharge() {
    return digitalRead(HWPOWER_PIN_BATCHRG) == LOW;
}
#endif

void pwrAutoOff() {
    if ((cfg.d().hrpwrafton > 0) && 
        (tmcntIntervEn(TMCNT_UPTIME) > (cfg.d().hrpwrafton * 3600))) {
        CONSOLE("pwrOff by %d hour after pwrOn", cfg.d().hrpwrafton);
        tmcntReset(TMCNT_UPTIME, false);
        pwrOff();
    }
    
    if ((cfg.d().hrpwrnofly > 0) && 
        (tmcntIntervEn(TMCNT_NOFLY) > (cfg.d().hrpwrnofly * 3600))) {
        CONSOLE("pwrOff by %d hour no fly", cfg.d().hrpwrnofly);
        tmcntReset(TMCNT_NOFLY, false);
        pwrOff();
    }
}
