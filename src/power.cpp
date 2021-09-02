
#include "power.h"
#include "log.h"
#include "view/base.h"
#include "gps/proc.h"

#include <EEPROM.h>


/* ------------------------------------------------------------------------------------------- *
 *  работа с eeprom
 * ------------------------------------------------------------------------------------------- */
static RTC_DATA_ATTR bool isoff = false;

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


bool pwrCheck() {
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
        isoff = false;
        hwOn();
        return true;
    }

    if (!isoff) {
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
            CONSOLE("pwrCheck on");
            isoff = false;
            hwOn();
            return true;
        }
        delay(100);
        n++;
    }

    CONSOLE("pwrCheck off");
    hwOff();
    return false;
}

void pwrOff() {
    isoff = true;
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
