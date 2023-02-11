/*
    Bluetooth functions
*/

#include "bt.h"

#ifdef USE_BLUETOOTH
#include "../log.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gap_bt_api.h"
#include "esp_spp_api.h"
#include "esp_bt_device.h"

//#include "freertos/task.h"

#define ERR(txt, ...)   do { CONSOLE(txt, ##__VA_ARGS__); return false; } while (0)
#define ESPRUN(func)    do { CONSOLE(TOSTRING(func)); if ((err = func) != ESP_OK) ERR(TOSTRING(func) ": 0x%04x", err); } while (0)
#define ESPVOID(func)   do { esp_err_t err; CONSOLE(TOSTRING(func)); if ((err = func) != ESP_OK) CONSOLE(TOSTRING(func) ": 0x%04x", err); return; } while (0)

/*
static void _on_gap(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param){
    CONSOLE("gap_ble event: %d", event);
    if (event == ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT){
        esp_ble_adv_params_t adv_params = {
            .adv_int_min         = 512,
            .adv_int_max         = 1024,
            .adv_type            = ADV_TYPE_NONCONN_IND,
            .own_addr_type       = BLE_ADDR_TYPE_PUBLIC,
            .peer_addr           = {0x00, },
            .peer_addr_type      = BLE_ADDR_TYPE_PUBLIC,
            .channel_map         = ADV_CHNL_ALL,
            .adv_filter_policy   = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
        };
        esp_ble_gap_start_advertising(&adv_params);
    }
}
*/

static void _on_gap(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
    switch (event) {
        case ESP_BT_GAP_MODE_CHG_EVT:
            CONSOLE("ESP_BT_GAP_MODE_CHG_EVT: %d", param->mode_chg.mode);
            break;
        default:
            CONSOLE("gap event: %d", event);
    }
}

static void _on_spp(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
    switch (event) {
        case ESP_SPP_INIT_EVT:
            CONSOLE("ESP_SPP_INIT_EVT: %d", param->init.status);
            if (param->init.status != ESP_SPP_SUCCESS)
                return;
            
#ifdef ESP_IDF_VERSION_MAJOR
            ESPVOID(esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE));
#else
            ESPVOID(esp_bt_gap_set_scan_mode(ESP_BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE));
#endif
            
            //CONSOLE("ESP_SPP_INIT_EVT: slave: start");
            ESPVOID(esp_spp_start_srv(ESP_SPP_SEC_NONE, ESP_SPP_ROLE_SLAVE, 0, "XdeYa-SPP"));
            break;
        
        case ESP_SPP_START_EVT: //server started
            CONSOLE("ESP_SPP_START_EVT: %d, sec_id:%d scn:%d", param->start.status, param->start.sec_id, param->start.scn);
            if (param->start.status != ESP_SPP_SUCCESS)
                return;
            break;
        
        default:
            CONSOLE("spp event: %d", event);
    }
}

#include "esp32-hal-bt.h"

bool bluetoothStart() {
    esp_err_t err;

    CONSOLE("esp_bt_controller_get_status1: %d", esp_bt_controller_get_status());
    if (!btStarted() && !btStart()) // Почему-то esp_bt_controller_init не работает,
                                    // а так работает
        ERR("btStart fail: %d", esp_bt_controller_get_status());
    
    /*
    if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE) {
        CONSOLE("nvs_flash_init()");
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
        }
        if (ret != ESP_OK) ERR("nvs_flash_init: 0x%04x", ret);
        //ESPRUN(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

        esp_bt_controller_config_t cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
        //cfg.mode = ESP_BT_MODE_BLE;
        ESPRUN(esp_bt_controller_init(&cfg));
        //CONSOLE("esp_bt_controller_init");
        //esp_bt_controller_init(&cfg);
        //while (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE){}
        //CONSOLE("ok");
    }
    */
    CONSOLE("esp_bt_controller_get_status2: %d", esp_bt_controller_get_status());
    
    if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED)
        ESPRUN(esp_bt_controller_enable(ESP_BT_MODE_BLE));
    
    if (esp_bt_controller_get_status() != ESP_BT_CONTROLLER_STATUS_ENABLED)
        ERR("bt_controller status fail: %d", esp_bt_controller_get_status());

    CONSOLE("bluetooth controller started OK");

    if (esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_UNINITIALIZED)
        ESPRUN(esp_bluedroid_init());
    
    if (esp_bluedroid_get_status() != ESP_BLUEDROID_STATUS_ENABLED)
        ESPRUN(esp_bluedroid_enable());

    CONSOLE("bluetooth bluedroid started OK");

    ESPRUN(esp_bt_gap_register_callback(_on_gap));
    
    /*
    char name[32];
    snprintf_P(name, sizeof(name), PSTR("XdeYa-BLE_GAP"));
    ESPRUN(esp_ble_gap_set_device_name(name));

    esp_ble_adv_data_t adv_config = {
        .set_scan_rsp        = false,
        .include_name        = true,
        .include_txpower     = true,
        .min_interval        = 512,
        .max_interval        = 1024,
        .appearance          = 0,
        .manufacturer_len    = 0,
        .p_manufacturer_data = NULL,
        .service_data_len    = 0,
        .p_service_data      = NULL,
        .service_uuid_len    = 0,
        .p_service_uuid      = NULL,
        .flag                = (ESP_BLE_ADV_FLAG_GEN_DISC|ESP_BLE_ADV_FLAG_BREDR_NOT_SPT)
    };
    ESPRUN(esp_ble_gap_config_adv_data(&adv_config));
    
    ESPRUN(esp_ble_gap_register_callback(_on_gap));

    esp_bt_cod_t cod;
    cod.major = 0b00001;
    cod.minor = 0b000100;
    cod.service = 0b00000010110;
    ESPRUN(esp_bt_gap_set_cod(cod, ESP_BT_INIT_COD));
    CONSOLE("ble gap started OK");
    */

    ESPRUN(esp_spp_register_callback(_on_spp));
    ESPRUN(esp_spp_init(ESP_SPP_MODE_CB));

    {
        char name[32];
        snprintf_P(name, sizeof(name), PSTR("XdeYa-Dev"));
        ESPRUN(esp_bt_dev_set_device_name(name));
    }
    
    return true;
}

bool bluetoothStop() {
    esp_err_t err;
    
    if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED) {
        if (esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_ENABLED)
            ESPRUN(esp_bluedroid_disable());
        if (esp_bluedroid_get_status() != ESP_BLUEDROID_STATUS_UNINITIALIZED)
            ESPRUN(esp_bluedroid_deinit());
    }
    
    if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED) {
        ESPRUN(esp_bt_controller_disable());
        while (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED);
        if (esp_bt_controller_get_status() != ESP_BT_CONTROLLER_STATUS_INITED)
            ERR("bt_controller status fail: %d", esp_bt_controller_get_status());
    }

    if (esp_bt_controller_get_status() != ESP_BT_CONTROLLER_STATUS_IDLE) {
        ESPRUN(esp_bt_controller_deinit());
        //vTaskDelay(1);
    }
    if (esp_bt_controller_get_status() != ESP_BT_CONTROLLER_STATUS_IDLE)
        ERR("bt_controller status fail: %d", esp_bt_controller_get_status());

    CONSOLE("bluetooth controller stoped");
    
    return true;
}
#endif // #ifdef USE_BLUETOOTH
