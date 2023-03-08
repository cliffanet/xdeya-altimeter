/*
    WiFi functions
*/

#include "wifi.h"
#include "../log.h"
#include "../clock.h"
#include "binproto.h"
#include "../core/filetxt.h"
#include "../view/text.h"

#include <WiFiClient.h>
#include <esp_err.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include "freertos/event_groups.h"
#include "soc/soc.h" // brownout detector
#include "soc/rtc_cntl_reg.h"

#include "esp_log.h"

#include "lwip/sockets.h"       // host -> ip
#include "lwip/sys.h"
#include "lwip/netdb.h"

#include <vector>
#include <string.h> // strcpy

static std::vector<wifi_net_t> wifiall;
static EventGroupHandle_t sta_status = NULL;
static esp_netif_t *sta_netif = NULL;
static esp_event_handler_instance_t instance_any_id = NULL;
static esp_event_handler_instance_t instance_any_ip = NULL;

/* ------------------------------------------------------------------------------------------- *
 *  Поиск пароля по имени wifi-сети
 * ------------------------------------------------------------------------------------------- */
bool wifiPassFind(const char *ssid, char *pass) {
    size_t len = strlen(ssid);
    char ssid1[len+1];
    FileTxt f(PSTR(WIFIPASS_FILE));
    
    CONSOLE("need ssid: %s (avail: %d)", ssid, f.available());
    
    if (!f)
        return false;
    
    while (f.available() > 0) {
        if (!f.find_param(PSTR("ssid")))
            break;
        f.read_line(ssid1, sizeof(ssid1));
        if (strcmp(ssid1, ssid) != 0)
            continue;
        
        char param[30];
        f.read_param(param, sizeof(param));
        if (strcmp_P(param, PSTR("pass")) == 0)
            f.read_line(pass, 33);
        else
        if (pass != NULL)
            *pass = 0;
        if (pass != NULL)
            CONSOLE("found pass: %s", pass);
        else
            CONSOLE("founded");
        f.close();
        return true;
    }
    
    CONSOLE("EOF");

    f.close();
    return false;
}

/* ------------------------------------------------------------------------------------------- *
 *  События
 * ------------------------------------------------------------------------------------------- */
static void setStatus(wifi_status_t status) {
    if (sta_status == NULL)
        return;
    xEventGroupClearBits(sta_status, 0x00FFFFFF);
    xEventGroupSetBits(sta_status, status);
}
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    CONSOLE("event_handler: [%d]%d", event_base, event_id);
    
    if (event_base == WIFI_EVENT)
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                setStatus(WIFI_STA_DISCONNECTED);
                break;
            
            case WIFI_EVENT_STA_STOP:
                setStatus(WIFI_STA_NULL);
                break;
            
            case WIFI_EVENT_STA_CONNECTED:
                setStatus(WIFI_STA_WAITIP);
                break;
            
            case WIFI_EVENT_STA_DISCONNECTED: {
                    wifi_event_sta_disconnected_t * event = (wifi_event_sta_disconnected_t*)event_data;
                    CONSOLE("disconnected, reason: %u", event->reason);
                    switch (event->reason)  {
                        case WIFI_REASON_NO_AP_FOUND:
                        case WIFI_REASON_AUTH_FAIL:
                        case WIFI_REASON_AUTH_EXPIRE:
                        case WIFI_REASON_ASSOC_FAIL:
                        case WIFI_REASON_BEACON_TIMEOUT:
                        case WIFI_REASON_HANDSHAKE_TIMEOUT:
                            setStatus(WIFI_STA_FAIL);
                            break;
                        default:
                            setStatus(WIFI_STA_DISCONNECTED);
                    }
                }
                break;
        }
    else
    if (event_base == IP_EVENT)
        switch (event_id) {
            case IP_EVENT_STA_GOT_IP:
                //CONSOLE("got %sIP:" IPSTR, IP2STR((&((ip_event_got_ip_t*) event_data)->ip_info.ip)));
                setStatus(WIFI_STA_CONNECTED);
                break;
            
            case IP_EVENT_STA_LOST_IP:
                setStatus(WIFI_STA_FAIL);
                break;
            }
}

/* ------------------------------------------------------------------------------------------- *
 *  Запуск / остановка
 * ------------------------------------------------------------------------------------------- */
static bool _wifiStart() {
    esp_err_t err;
    wifiall.clear();

#define ERR(txt, ...)   { CONSOLE(txt, ##__VA_ARGS__); return false; }
#define ESPRUN(func)    { CONSOLE(TOSTRING(func)); if ((err = func) != ESP_OK) ERR(TOSTRING(func) ": %d", err); }
    
    // event
    if (sta_status == NULL) {
        CONSOLE("xEventGroupCreate");
        sta_status = xEventGroupCreate();
        if (sta_status == NULL)
            ERR("xEventGroupCreate fail");
    }
    
    if ((instance_any_id == NULL) && (instance_any_ip == NULL))
        ESPRUN(esp_event_loop_create_default());
    
    if (instance_any_id == NULL)
        ESPRUN(esp_event_handler_instance_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &event_handler,
            NULL,
            &instance_any_id
        ));
    if (instance_any_ip == NULL)
        ESPRUN(esp_event_handler_instance_register(
            IP_EVENT,
            ESP_EVENT_ANY_ID,
            &event_handler,
            NULL,
            &instance_any_ip
        ));
    
    // netif
    static bool netif_init = false;
    if (!netif_init) {
#if CONFIG_IDF_TARGET_ESP32
        uint8_t mac[8];
        if (esp_efuse_mac_get_default(mac) == ESP_OK)
            esp_base_mac_addr_set(mac);
#endif
        
        ESPRUN(esp_netif_init());
        netif_init = true;
    }
    
    if (sta_netif == NULL) {
        CONSOLE("esp_netif_create_default_wifi_sta");
        sta_netif = esp_netif_create_default_wifi_sta();
        if (sta_netif == NULL)
            ERR("esp_netif_create_default_wifi_sta fail");
    }
    
    // config
    setStatus(WIFI_STA_NULL);
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESPRUN(esp_wifi_init(&cfg));
    
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

    CONSOLE("RTC_CNTL_BROWN_OUT_REG 0");
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
    
    ESPRUN(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESPRUN(esp_wifi_set_mode(WIFI_MODE_STA));
    // staart
    ESPRUN(esp_wifi_start());
    ESPRUN(esp_wifi_set_max_tx_power(60));

    CONSOLE("RTC_CNTL_BROWN_OUT_REG 1");
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 1); //enable brownout detector
    
    CONSOLE("wifi started");

#undef ERR
#undef ESPRUN
    
    return true;
}

bool wifiStart() {
    if (!_wifiStart()) {
        wifiStop();
        return false;
    }
    
    return true;
}

bool wifiStop() {
    esp_err_t err;
    bool ret = true;
    
#define ERR(txt, ...)   { CONSOLE(txt, ##__VA_ARGS__); ret = false; }
#define ESPRUN(func)    { CONSOLE(TOSTRING(func)); if ((err = func) != ESP_OK) ERR(TOSTRING(func) ": %d", err); }
    
    if (wifiStatus() > WIFI_STA_NULL)
        ESPRUN(esp_wifi_disconnect());
    
    ESPRUN(esp_wifi_stop());
    ESPRUN(esp_wifi_deinit());
    
    if (sta_netif != NULL) {
        ESPRUN(esp_netif_dhcpc_stop(sta_netif));
        esp_netif_destroy_default_wifi(sta_netif);
        sta_netif = NULL;
    }
    
    // Из описания: Note: Deinitialization is not supported yet
    // ESPRUN(esp_netif_deinit());
    
    if (instance_any_id != NULL) {
        ESPRUN(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
        instance_any_id = NULL;
    }
    if (instance_any_ip != NULL) {
        ESPRUN(esp_event_handler_instance_unregister(IP_EVENT, ESP_EVENT_ANY_ID, instance_any_ip));
        instance_any_ip = NULL;
    }
    
    ESPRUN(esp_event_loop_delete_default());
    
    if (sta_status != NULL) {
        auto sta = sta_status;
        sta_status = NULL;
        vEventGroupDelete(sta);
    }
    
    CONSOLE("wifi stopped");
    
#undef ERR
#undef ESPRUN
    
    return ret;
}

/* ------------------------------------------------------------------------------------------- *
 *  Поиск доступных сетей
 * ------------------------------------------------------------------------------------------- */
uint16_t wifiScan(bool show_hidden, bool passive, uint32_t max_ms_per_chan, uint8_t channel) {
    wifiall.clear();

#define ERR(txt, ...)   { CONSOLE(txt, ##__VA_ARGS__); return 0; }
#define ESPRUN(func)    { CONSOLE(TOSTRING(func)); if ((err = func) != ESP_OK) ERR(TOSTRING(func) ": %d", err); }
    
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
        ESPRUN(esp_wifi_scan_get_ap_records(&count, rall));
        return 0;
    }
    else {
        wifi_ap_record_t rall[count];
        ESPRUN(esp_wifi_scan_get_ap_records(&count, rall));
        
        wifiall.reserve(count);
        for (uint16_t i = 0; i < count; i++) {
            wifi_ap_record_t &r = rall[i];
            wifi_net_t n;
            strcpy(n.ssid, reinterpret_cast<const char*>(r.ssid));
            n.rssi = r.rssi;
            n.sec =
                r.authmode == WIFI_AUTH_OPEN ?
                    WIFI_OPEN :
                wifiPassFind(n.ssid) ?
                    WIFI_PASSFOUND :
                    WIFI_PASSUNKNOWN;
            
            wifiall.push_back(n);
            CONSOLE("found: %s (rssi: %d, open: %d)", n.ssid, r.rssi, r.authmode);
        }
    }
    
#undef ERR
#undef ESPRUN
    
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
    if ((ssid == NULL) || (strlen(ssid) > 32)) {
        CONSOLE("SSID too long or missing!");
        return false;
    }

    if (pass && (strlen(pass) > 64)) {
        CONSOLE("passphrase too long!");
        return false;
    }

    if (sta_netif == NULL) {
        CONSOLE("wifi not inited");
        return false;
    }

#define ERR(txt, ...)   { CONSOLE(txt, ##__VA_ARGS__); return false; }
#define ESPRUN(func)    { CONSOLE(TOSTRING(func)); if ((err = func) != ESP_OK) ERR(TOSTRING(func) ": %d", err); }
    
    wifi_config_t conf = { };
    strcpy(reinterpret_cast<char*>(conf.sta.ssid), ssid);
    conf.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;       //force full scan to be able to choose the nearest / strongest AP
    conf.sta.threshold.rssi = -127;
    conf.sta.pmf_cfg.capable = true;
    conf.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;

    if (pass) {
        if (strlen(pass) == 64) { // it's not a passphrase, is the PSK
            memcpy(reinterpret_cast<char*>(conf.sta.password), pass, 64);
            conf.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
        }
        else {
            strcpy(reinterpret_cast<char*>(conf.sta.password), pass);
            conf.sta.threshold.authmode = WIFI_AUTH_WEP;
        }
    }
    else {
        conf.sta.threshold.authmode = WIFI_AUTH_OPEN;
    }
    
    esp_err_t err;
    //ESPRUN(esp_wifi_disconnect());
    ESPRUN(esp_wifi_set_config(WIFI_IF_STA, &conf));
    /*
    err = esp_netif_dhcpc_stop(sta_netif);
    if ((err != ESP_OK) && (err != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED))
        ERR(TOSTRING(esp_netif_dhcpc_stop(sta_netif)) ": %d", err);
    // из документации для esp_netif_set_ip_info():
    // DHCP client/server must be stopped (if enabled for this interface) before setting new IP information.
    esp_netif_ip_info_t info = { 0 };
    ESPRUN(esp_netif_set_ip_info(sta_netif, &info));
    */
    // из документации для esp_netif_dhcpc_start(): 
    // The default event handlers for the SYSTEM_EVENT_STA_CONNECTED and SYSTEM_EVENT_ETH_CONNECTED events call this function.
    ESPRUN(esp_netif_dhcpc_start(sta_netif));
    
    ESPRUN(esp_wifi_connect());
    
    CONSOLE("wifi connect begin");
    
#undef ERR
#undef ESPRUN
    
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
    
    if ((wifiStatus() == WIFI_STA_NULL) || (sta_netif == NULL)) {
        ip.dword = 0;
        return ip;
    }
    
    esp_netif_ip_info_t ipinf;
    esp_err_t err = esp_netif_get_ip_info(sta_netif, &ipinf);
    if (err != ESP_OK){
        CONSOLE("esp_netif_get_ip_info: %d", err);
        ip.dword = 0;
        return ip;
    }
    
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
    rssi = info.rssi;
    
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

/* ------------------------------------------------------------------------------------------- *
 *  WiFi-клиент
 * ------------------------------------------------------------------------------------------- */
template class NetSocketClient<WiFiClient>;

NetSocket *wifiCliConnect() {
    char host[64];
    strcpy_P(host, PSTR(WIFI_SRV_HOST));
    
    struct addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *res;
    int err = getaddrinfo(host, NULL, &hints, &res);
    if(err != 0 || res == NULL) {
        CONSOLE("Can't resolve host: %s", host);
        return NULL;
    }

    IPAddress ip(reinterpret_cast<struct sockaddr_in *>(res->ai_addr)->sin_addr.s_addr);
    CONSOLE("host %s -> ip %d.%d.%d.%d", host, ip[0], ip[1], ip[2], ip[3]);

    WiFiClient cli;
    if (!cli.connect(ip, WIFI_SRV_PORT)) {
        CONSOLE("Can't connect to: %s:%d", host, WIFI_SRV_PORT);
        return NULL;
    }
    CONSOLE("Connected to: %s:%d", host, WIFI_SRV_PORT);
    
    return new NetSocketClient<WiFiClient>(cli);
}
