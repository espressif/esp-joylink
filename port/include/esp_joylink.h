
#ifndef __ESP_JOYLINK_H__
#define __ESP_JOYLINK_H__

#include "joylink.h"

#include "esp_bt.h"
#include "esp_gatts_api.h"

#define JOYLINK_VERSION     _VERSION_
#define JOYLINK_COMMIT_ID   _GIT_HEAD_


typedef enum {
    ESP_JOYLINK_CONFIG_NETWORK_NONE = 0,
    ESP_JOYLINK_CONFIG_NETWORK_SMNT,
    ESP_JOYLINK_CONFIG_NETWORK_SMNT_BLE,
    ESP_JOYLINK_CONFIG_NETWORK_SOFTAP,
    
    ESP_JOYLINK_CONFIG_NETWORK_MAX = ESP_JOYLINK_CONFIG_NETWORK_SOFTAP
} esp_joylink_config_network_t;

// start joylink app
void esp_joylink_app_start(void);

// clear joylink wifi information
void esp_joylink_wifi_clear_info(void);

// clear joylink joylink device information,eg ss
void esp_joylink_dev_clear_info(void);

esp_joylink_config_network_t esp_joylink_get_config_network(void);

void esp_joylink_set_config_network(esp_joylink_config_network_t config_network);

void esp_restore_factory_setting(void);

esp_err_t esp_joylink_wifi_event_handler(void *ctx, system_event_t *event);

void esp_joylink_ble_gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);


void esp_joylink_wifi_save_info(uint8_t*ssid,uint8_t*password);

void esp_joylink_softap_innet(void);
#endif

