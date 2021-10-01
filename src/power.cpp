
#include "power.h"
#include "log.h"
#include "clock.h"
#include "view/base.h"
#include "view/menu.h"
#include "gps/proc.h"
#include "jump/proc.h"
#include "file/track.h"


#include "driver/adc.h";


static RTC_DATA_ATTR power_mode_t mode = PWR_ACTIVE;

// Уход в сон без таймера - pwroff
static void pwrDeepSleep() {
    adc_power_off();
    
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
    adc_power_off();
    
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
            else
                pwrDeepSleep(1000000);
            
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
    gpsOff(false);
    
    CONSOLE("pwr sleep");
    pwrDeepSleep(1000000);
}

void pwrOff() {
    mode = PWR_OFF;
    
    displayOff();
    gpsOff(false);
    
    // перед тем, как уйти в сон окончательно, дождёмся отпускания кнопки питания
    while (digitalRead(BUTTON_GPIO_PWR) == LOW)
        delay(100);
    
    CONSOLE("pwr off");
    pwrDeepSleep();
}

#if HWVER > 1
bool pwrBattChk(bool init, uint16_t val) {
    if (init)
        pinMode(HWPOWER_PIN_BATIN, INPUT);
    
    if (pwrBattValue() > val)
        return true;
    
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
    gpsOff(false);
    pwrDeepSleep();
    
    return false;
}
uint16_t pwrBattValue() {
    return analogRead(HWPOWER_PIN_BATIN);
}
#endif

#if HWVER >= 3
bool pwrBattCharge() {
    return digitalRead(HWPOWER_PIN_BATCHRG) == LOW;
}
#endif
