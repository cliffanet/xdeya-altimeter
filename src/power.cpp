
#include "power.h"
#include "log.h"
#include "clock.h"
#include "view/base.h"
#include "view/menu.h"
#include "gps/proc.h"
#include "jump/proc.h"
#include "file/track.h"


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
                    if (tm > 4000) {
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
    
    if (trkRunning())
        return PWR_PASSIVE;
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
    setCpuFrequencyMhz(10);
    
    CONSOLE("pwr sleep");
    pwrDeepSleep(1000000);
}

void pwrOff() {
    mode = PWR_OFF;
    
    displayOff();
    gpsPwrDown();
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
#endif

#if HWVER >= 3
bool pwrBattCharge() {
    return digitalRead(HWPOWER_PIN_BATCHRG) == LOW;
}
#endif
