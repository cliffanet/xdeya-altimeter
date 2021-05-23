/*
    WiFi functions
*/

#include "wifi.h"
#include "../log.h"

#include <esp_err.h>
#include <esp_wifi.h>
#include <esp_event_loop.h>

#include "esp_log.h"

#include <vector>
#include <string.h> // strcpy

static std::vector<wifi_net_t> wifiall;

/* ------------------------------------------------------------------------------------------- *
 *  Запуск / остановка
 * ------------------------------------------------------------------------------------------- */
// Empty event handler
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    CONSOLE("event_handler: %d", *event);
    return ESP_OK;
}

bool wifiStart() {
    esp_err_t err;
    wifiall.clear();
    
    static bool tcpip_init = false;
    if (!tcpip_init) {
        tcpip_adapter_init();

        err = esp_event_loop_init(event_handler, NULL);
        if (err != ESP_OK) {
            CONSOLE("esp_event_loop_init: %d", err);
            return false;
        }
        
        tcpip_init = true;
    }
    
    // esp32-sdk не умеет устанавливать максимальную мощность wifi
    // до выполнения esp_wifi_start();
    // В документации к esp_wifi_set_max_tx_power() сказано:
    // Maximum power before wifi startup is limited by PHY init data bin
    
    // Исходники к esp_wifi_start() или esp_wifi_set_max_tx_power() не нашёл,
    // почему именно так - непонятно,
    // однако, есть исходники к PHY init data:
    // https://github.com/espressif/esp-idf/blob/master/components/esp_wifi/src/phy_init.c
    // И там встречается вот такой код, который понижает мощность в случае просадки:
    // if (esp_reset_reason() == ESP_RST_BROWNOUT) esp_phy_reduce_tx_power(init_data);
    
    // а нам нужно понижать заранее и не дожидаться срабатывания ESP_RST_BROWNOUT
    
    //esp_phy_init_data_t* init_data
    //memcpy(init_data, phy_init_data, sizeof(esp_phy_init_data_t));
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        CONSOLE("esp_wifi_init: %d", err);
        return false;
    }
    
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    
    err = esp_wifi_set_mode( WIFI_MODE_STA);
    if (err != ESP_OK) {
        CONSOLE("esp_wifi_set_mode: %d", err);
        return false;
    }
    
    err = esp_wifi_start();
    if (err != ESP_OK) {
        CONSOLE("esp_wifi_start: %d", err);
        return false;
    }
    
    err = esp_wifi_set_max_tx_power(60);
    if (err != ESP_OK) {
        CONSOLE("esp_wifi_set_max_tx_power: %d", err);
        return false;
    }
    
    return true;
}

bool wifiStop() {
    wifiall.clear();
    esp_err_t err;
    
    err = esp_wifi_stop();
    if (err != ESP_OK) {
        CONSOLE("esp_wifi_stop: %d", err);
        return false;
    }
    
    err = esp_wifi_deinit();
    if (err != ESP_OK) {
        CONSOLE("esp_wifi_deinit: %d", err);
        return false;
    }
    
    return true;
}
/* ------------------------------------------------------------------------------------------- *
 *  Поиск доступных сетей
 * ------------------------------------------------------------------------------------------- */
uint16_t wifiScan(bool show_hidden, bool passive, uint32_t max_ms_per_chan, uint8_t channel) {
    wifiall.clear();
    
    wifi_scan_config_t config;
    config.ssid = 0;
    config.bssid = 0;
    config.channel = channel;
    config.show_hidden = show_hidden;
    
    if (passive) {
        config.scan_type = WIFI_SCAN_TYPE_PASSIVE;
        config.scan_time.passive = max_ms_per_chan;
    } else {
        config.scan_type = WIFI_SCAN_TYPE_ACTIVE;
        config.scan_time.active.min = 100;
        config.scan_time.active.max = max_ms_per_chan;
    }
    
    esp_err_t err;
    err = esp_wifi_scan_start(&config, true);
    if (err != ESP_OK) {
        CONSOLE("WiFi scan start fail: %d", err);
        return 0;
    }
    
    uint16_t count;
    err = esp_wifi_scan_get_ap_num(&count);
    if (err != ESP_OK)
        count = 0;
    
    if (count == 0) {
        count = 1;
        wifi_ap_record_t rall[count];
        esp_wifi_scan_get_ap_records(&count, rall);
        return 0;
    }
    else {
        wifi_ap_record_t rall[count];
        err = esp_wifi_scan_get_ap_records(&count, rall);
        if (err != ESP_OK)
            return 0;
        
        wifiall.reserve(count);
        for (uint16_t i = 0; i < count; i++) {
            wifi_ap_record_t &r = rall[i];
            wifi_net_t n;
            strcpy(n.ssid, reinterpret_cast<const char*>(r.ssid));
            n.rssi = r.rssi;
            n.isopen = r.authmode == WIFI_AUTH_OPEN;
            wifiall.push_back(n);
            CONSOLE("found: %s (rssi: %d, open: %d)", n.ssid, r.rssi, r.authmode);
        }
    }
    
    return count;
}

const wifi_net_t * wifiScanInfo(uint16_t i) {
    if (i >= wifiall.size())
        return NULL;
    return &(wifiall[i]);
}

/* ------------------------------------------------------------------------------------------- *
 *  отладка
 * ------------------------------------------------------------------------------------------- */

int8_t wifiPower() {
    int8_t power;
    if (esp_wifi_get_max_tx_power(&power))
        return -1;
    return power;
}

