/*
    WiFi functions
*/

#include "wifi.h"
#include "../log.h"
#include "../clock.h"

#include <esp_err.h>
#include <esp_wifi.h>
#include <esp_event_loop.h>
#include "freertos/event_groups.h"
#include "driver/adc.h";
#include "soc/soc.h" // brownout detector
#include "soc/rtc_cntl_reg.h"

#include "esp_log.h"

#include <vector>
#include <string.h> // strcpy

static std::vector<wifi_net_t> wifiall;
static EventGroupHandle_t sta_status = NULL;

/* ------------------------------------------------------------------------------------------- *
 *  События
 * ------------------------------------------------------------------------------------------- */
static void setStatus(wifi_status_t status) {
    if (sta_status == NULL)
        return;
    xEventGroupClearBits(sta_status, 0x00FFFFFF);
    xEventGroupSetBits(sta_status, status);
}
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    CONSOLE("event_handler: %d", event->event_id);
    
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            setStatus(WIFI_STA_DISCONNECTED);
            break;
        
        case SYSTEM_EVENT_STA_STOP:
            setStatus(WIFI_STA_NULL);
            break;
        
        case SYSTEM_EVENT_STA_CONNECTED:
            setStatus(WIFI_STA_WAITIP);
            break;
        
        case SYSTEM_EVENT_STA_DISCONNECTED:
            switch (event->event_info.disconnected.reason) {
                case WIFI_REASON_NO_AP_FOUND:
                case WIFI_REASON_AUTH_FAIL:
                case WIFI_REASON_ASSOC_FAIL:
                case WIFI_REASON_BEACON_TIMEOUT:
                case WIFI_REASON_HANDSHAKE_TIMEOUT:
                case WIFI_REASON_AUTH_EXPIRE:
                    setStatus(WIFI_STA_FAIL);
                    break;
                default:
                    setStatus(WIFI_STA_DISCONNECTED);
            }
            break;
        
        case SYSTEM_EVENT_STA_GOT_IP:
            setStatus(WIFI_STA_CONNECTED);
            CONSOLE("got ip:%s", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
            break;
            
        case SYSTEM_EVENT_STA_LOST_IP:
            setStatus(WIFI_STA_FAIL);
            break;
    }
    
    return ESP_OK;
}

/* ------------------------------------------------------------------------------------------- *
 *  Запуск / остановка
 * ------------------------------------------------------------------------------------------- */
bool wifiStart() {
    esp_err_t err;
    wifiall.clear();
    
    adc_power_on();
    
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
    
    if (sta_status == NULL)
        sta_status = xEventGroupCreate();
    setStatus(WIFI_STA_NULL);
    
    clockIntDisable();
    
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
    
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&cfg);
    if (err != ESP_OK) {
        CONSOLE("esp_wifi_init: %d", err);
        clockIntEnable();
        return false;
    }
    
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    
    err = esp_wifi_set_mode( WIFI_MODE_STA);
    if (err != ESP_OK) {
        CONSOLE("esp_wifi_set_mode: %d", err);
        clockIntEnable();
        return false;
    }
    
    err = esp_wifi_start();
    if (err != ESP_OK) {
        CONSOLE("esp_wifi_start: %d", err);
        clockIntEnable();
        return false;
    }
    
    err = esp_wifi_set_max_tx_power(60);
    if (err != ESP_OK) {
        CONSOLE("esp_wifi_set_max_tx_power: %d", err);
        clockIntEnable();
        return false;
    }
    
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 1); //enable brownout detector
    
    CONSOLE("wifi started");
    
    return true;
}

bool wifiStop() {
    esp_err_t err;
    
    if (wifiStatus() > WIFI_STA_NULL) {
        err = esp_wifi_disconnect();
        if (err != ESP_OK) {
            CONSOLE("esp_wifi_disconnect: %d", err);
            clockIntEnable();
            return false;
        }
    }
    
    err = esp_wifi_stop();
    if (err != ESP_OK) {
        CONSOLE("esp_wifi_stop: %d", err);
        clockIntEnable();
        return false;
    }
    
    err = esp_wifi_deinit();
    if (err != ESP_OK) {
        CONSOLE("esp_wifi_deinit: %d", err);
        clockIntEnable();
        return false;
    }
    
    if (sta_status != NULL) {
        vEventGroupDelete(sta_status);
        sta_status = NULL;
    }
    
    CONSOLE("wifi stopped");
    
    clockIntEnable();
    adc_power_off();
    
    return true;
}
bool wifiStarted() {
    wifi_mode_t mode;
    return
        (esp_wifi_get_mode(&mode) == ESP_OK) &&
        (mode & WIFI_MODE_STA);
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
    
    CONSOLE("wifi scan found %d nets", count);
    
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

void wifiScanClean() {
    wifiall.clear();
}

/* ------------------------------------------------------------------------------------------- *
 *  соединение с wifi-сетью
 * ------------------------------------------------------------------------------------------- */

bool wifiConnect(const char* ssid, const char *pass) {
    if (!ssid || (*ssid == NULL) || (strlen(ssid) > 32)) {
        CONSOLE("SSID too long or missing!");
        return false;
    }

    if (pass && (strlen(pass) > 64)) {
        CONSOLE("passphrase too long!");
        return false;
    }
    
    wifi_config_t conf = { };
    strcpy(reinterpret_cast<char*>(conf.sta.ssid), ssid);
    conf.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;       //force full scan to be able to choose the nearest / strongest AP

    if (pass) {
        if (strlen(pass) == 64) { // it's not a passphrase, is the PSK
            memcpy(reinterpret_cast<char*>(conf.sta.password), pass, 64);
        }
        else {
            strcpy(reinterpret_cast<char*>(conf.sta.password), pass);
        }
    }
    
    esp_err_t err;
    err = esp_wifi_disconnect();
    if (err != ESP_OK) {
        CONSOLE("disconnect fail: %d", err);
        return false;
    }
    
    err = esp_wifi_set_config(WIFI_IF_STA, &conf);
    if (err != ESP_OK) {
        CONSOLE("esp_wifi_set_config: %d", err);
        return false;
    }
    
    if (tcpip_adapter_dhcpc_start(TCPIP_ADAPTER_IF_STA) == ESP_ERR_TCPIP_ADAPTER_DHCPC_START_FAILED) {
        CONSOLE("dhcp client start failed!");
        return false;
    }
    
    err = esp_wifi_connect();
    if (err != ESP_OK) {
        CONSOLE("esp_wifi_connect: %d", err);
        return false;
    }
    
    CONSOLE("wifi connect begin");
    
    return true;
}

wifi_status_t wifiStatus() {
    if (sta_status == NULL)
        return WIFI_STA_NULL;
    
    return
        static_cast<wifi_status_t>(xEventGroupClearBits(sta_status, 0));
}

ipaddr_t wifiIP() {
    ipaddr_t ip;
    
    if (wifiStatus() == WIFI_STA_NULL) {
        ip.dword = 0;
        return ip;
    }
    
    tcpip_adapter_ip_info_t ipinf;
    tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ipinf);
    ip.dword = ipinf.ip.addr;
    
    return ip;
}

bool wifiInfo(char *ssid, int8_t &rssi) {
    wifi_ap_record_t info;
    if ((wifiStatus() == WIFI_STA_NULL) ||
        (esp_wifi_sta_get_ap_info(&info) != ESP_OK)) {
        *ssid = '\0';
        rssi = 0;
        return false;
    }
    
    strcpy(ssid, reinterpret_cast<char*>(info.ssid));
    rssi = rssi;
    
    return true;
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

