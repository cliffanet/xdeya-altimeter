
#include "power.h"
#include "display.h"
#include "button.h"

#include <EEPROM.h>


/* ------------------------------------------------------------------------------------------- *
 *  работа с eeprom
 * ------------------------------------------------------------------------------------------- */
static RTC_DATA_ATTR bool isoff = false;

static void hwOff() {
    displayOff();
#if HWVER > 1
    digitalWrite(HWPOWER_PIN_GPS, HIGH);
    pinMode(HWPOWER_PIN_GPS, OUTPUT);
#endif
        Serial.println("hw off");
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
    Serial.println("Going to sleep now");
    esp_deep_sleep_start();
    Serial.println("This will never be printed");
}

static void hwOn() {
#if HWVER > 1
    pinMode(HWPOWER_PIN_GPS, OUTPUT);
    digitalWrite(HWPOWER_PIN_GPS, HIGH);
    delay(200);
    digitalWrite(HWPOWER_PIN_GPS, LOW);
    delay(200);
    pinMode(HWPOWER_PIN_BATIN, INPUT);
#endif
    //displayOn();
        Serial.println("hw on");
}

/* ------------------------------------------------------------------------------------------- *
 *  
 * ------------------------------------------------------------------------------------------- */


bool pwrCheck() {
    auto wakeup_reason = esp_sleep_get_wakeup_cause();
    switch(wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
        case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
        case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
        case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
        default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
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
        Serial.printf("Btn power is pushed: %d\r\n", n);
        if (n > 20) {
            // если кнопка нажата более 2 сек, 
            // сохраняем состояние как "вкл" и выходим с положительной проверкой
        Serial.println("pwrCheck on");
            isoff = false;
            hwOn();
            return true;
        }
        delay(100);
        n++;
    }

    Serial.println("pwrCheck off");
    hwOff();
    return false;
}

void pwrOff() {
    isoff = true;
    Serial.println("pwr off");
    hwOff();
}

#if HWVER > 1
uint16_t pwrBattValue() {
    return analogRead(HWPOWER_PIN_BATIN);
}
#endif
