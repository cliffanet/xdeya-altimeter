
#include "power.h"
#include "log.h"
#include "clock.h"
#include "view/base.h"
#include "view/menu.h"
#include "gps/proc.h"
#include "jump/proc.h"
#include "file/track.h"

#include <EEPROM.h>


/* ------------------------------------------------------------------------------------------- *
 *  работа с eeprom
 * ------------------------------------------------------------------------------------------- */
static RTC_DATA_ATTR power_mode_t mode = PWR_ACTIVE;

static void hwOff() {
    displayOff();
    gpsOff();
    CONSOLE("hw off");
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


    // перед тем, как уйти в сон окончательно, дождёмся отпускания кнопки питания
    while (digitalRead(BUTTON_GPIO_PWR) == LOW)
        delay(100);

    //Go to sleep now
    CONSOLE("Going to sleep now");
    esp_deep_sleep_start();
    CONSOLE("This will never be printed");
}

static void hwOn() {
    gpsOff();
    delay(200);
    gpsOn();
    delay(200);
#if HWVER > 1
    pinMode(HWPOWER_PIN_BATIN, INPUT);
#endif
#if HWVER >= 3
    pinMode(HWPOWER_PIN_BATCHRG, INPUT_PULLUP);
#endif
    //displayOn();
    CONSOLE("hw on");
}

/* ------------------------------------------------------------------------------------------- *
 *  
 * ------------------------------------------------------------------------------------------- */

power_mode_t pwrMode() {
    return mode;
}

bool pwrInit() {
    auto wakeup_reason = esp_sleep_get_wakeup_cause();
    switch(wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0 : CONSOLE("Wakeup caused by external signal using RTC_IO"); break;
        case ESP_SLEEP_WAKEUP_EXT1 : CONSOLE("Wakeup caused by external signal using RTC_CNTL"); break;
        case ESP_SLEEP_WAKEUP_TIMER : CONSOLE("Wakeup caused by timer"); break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD : CONSOLE("Wakeup caused by touchpad"); break;
        case ESP_SLEEP_WAKEUP_ULP : CONSOLE("Wakeup caused by ULP program"); break;
        default : CONSOLE("Wakeup was not caused by deep sleep: %d", wakeup_reason); break;
    }
    
    if (wakeup_reason == 0) {
        // если была перезагрузка по питанию, то просто включаемся
        mode = PWR_ACTIVE;
        hwOn();
        return true;
    }

    if (mode > PWR_OFF) {
        // если при загрузке обнаружили, что текущее состояние - "вкл", то включаемся
        hwOn();
        return true;
    }
    
    pinMode(BUTTON_GPIO_PWR, INPUT_PULLUP);
    int n = 0;
    while (digitalRead(BUTTON_GPIO_PWR) == LOW) {
        CONSOLE("Btn power is pushed: %d", n);
        if (n > 20) {
            // если кнопка нажата более 2 сек, 
            // сохраняем состояние как "вкл" и выходим с положительной проверкой
            CONSOLE("pwrInit on");
            mode = PWR_ACTIVE;
            hwOn();
            return true;
        }
        delay(100);
        n++;
    }

    CONSOLE("pwrInit off");
    hwOff();
    return false;
}

static power_mode_t pwrModeCalc() {
    if (mode == PWR_OFF)
        return PWR_OFF;
    
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
    if (mode == PWR_OFF)
        return;
    
    auto m = pwrModeCalc();
    if (mode == m)
        return;
    
    CONSOLE("[change] %d => %d", mode, m);
    if ((mode > PWR_SLEEP) && (m <= PWR_SLEEP))
        displayOff();
    else
    if ((mode <= PWR_SLEEP) && (m > PWR_SLEEP))
        displayOn();
    
    mode = m;
}

void pwrRun(void (*run)()) {
    uint64_t u = utm();
    
    if (run != NULL)
        run();
    
    pwrModeUpd();
    
    switch (mode) {
        case PWR_OFF:
            pwrOff();
            return;
        
        case PWR_SLEEP:
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

void pwrOff() {
    mode = PWR_OFF;
    CONSOLE("pwr off");
    hwOff();
}

#if HWVER > 1
uint16_t pwrBattValue() {
    return analogRead(HWPOWER_PIN_BATIN);
}
#endif

#if HWVER >= 3
bool pwrBattCharge() {
    return digitalRead(HWPOWER_PIN_BATCHRG) == LOW;
}
#endif
