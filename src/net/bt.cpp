/*
    Bluetooth functions
*/

#include "bt.h"

#ifdef USE_BLUETOOTH
#include "../log.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "freertos/task.h"

#define ERR(txt, ...)   do { CONSOLE(txt, ##__VA_ARGS__); return false; } while (0)
#define ESPRUN(func)    do { CONSOLE(TOSTRING(func)); if ((err = func) != ESP_OK) ERR(TOSTRING(func) ": %d", err); } while (0)

static void _on_gap(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param){
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

bool bluetoothStart() {
    esp_err_t err;
    
    if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE) {
        esp_bt_controller_config_t cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
        ESPRUN(esp_bt_controller_init(&cfg));
        while (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE){}
    }
    
    if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED)
        ESPRUN(esp_bt_controller_enable(ESP_BT_MODE_BLE));
    
    if (esp_bt_controller_get_status() != ESP_BT_CONTROLLER_STATUS_ENABLED)
        ERR("bt_controller status fail: %d", esp_bt_controller_get_status());

    CONSOLE("bluetooth controller started OK");

    if (esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_UNINITIALIZED)
        ESPRUN(esp_bluedroid_init());
    
    if (esp_bluedroid_get_status() != ESP_BLUEDROID_STATUS_ENABLED) {
        ESPRUN(esp_bluedroid_enable());
    
        char name[32];
        snprintf_P(name, sizeof(name), PSTR("XdeYa"));
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

        CONSOLE("ble gap started OK");
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
        vTaskDelay(1);
    }
    if (esp_bt_controller_get_status() != ESP_BT_CONTROLLER_STATUS_IDLE)
        ERR("bt_controller status fail: %d", esp_bt_controller_get_status());

    CONSOLE("bluetooth controller stoped");
    
    return true;
}
#endif // #ifdef USE_BLUETOOTH
