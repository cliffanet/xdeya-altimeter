
#include "power.h"
#include "display.h"
#include "button.h"

#include <EEPROM.h>


/* ------------------------------------------------------------------------------------------- *
 *  работа с eeprom
 * ------------------------------------------------------------------------------------------- */
typedef struct __attribute__((__packed__)) {
    uint8_t     isoff;
} pwr_state_t;

static bool getState() {
    EEPROMClass eepmy("pwr", 0);
    if (!eepmy.begin(sizeof(pwr_state_t)))
        return false;
    
    pwr_state_t pwr;
    eepmy.get(0, pwr);
    eepmy.end();
    
    if (pwr.isoff == 0xFF)
        pwr.isoff = 1;
    
    return pwr.isoff != 1;
}

static bool setState(bool ison) {
    EEPROMClass eepmy("pwr", 0);
    if (!eepmy.begin(sizeof(pwr_state_t)))
        return false;
    
    pwr_state_t pwr = { .isoff = ison ? 0 : 1 };
    eepmy.put(0, pwr);
    bool ok = eepmy.commit();
    eepmy.end();
    
    return ok;
}

static void hwOff() {
    displayOff();
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
    
    if (wakeup_reason == 0)
        // если была перезагрузка по питанию, то просто включаемся
        return true;

    if (getState())
        // если при загрузке обнаружили, что текущее состояние - "вкл", то включаемся
        return true;
    
    pinMode(BUTTON_GPIO_PWR, INPUT_PULLUP);
    int n = 0;
    while (digitalRead(BUTTON_GPIO_PWR) == LOW) {
        Serial.printf("Btn power is pushed: %d\r\n", n);
        if (n > 20) {
            // если кнопка нажата более 2 сек, 
            // сохраняем состояние как "вкл" и выходим с положительной проверкой
        Serial.println("pwrCheck on");
            setState(true);
            return true;
        }
        delay(100);
        n++;
    }

    Serial.println("pwrCheck off");
    hwOff();
    return false;
}

/*
void pwrOn() {
    displayOn();
        Serial.println("pwr on");
}
*/

void pwrOff() {
    if (!setState(false)) {
        Serial.println("setState fail");
        return;
    }
    Serial.println("pwr off");
    hwOff();
}