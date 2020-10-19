// Copyright 2015-2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "joylink_softap_start.h"
#include "joylink.h"

#ifdef CONFIG_JOYLINK_BLE_ENABLE
#define IS_CHOOSE_BLE true
#else
#define IS_CHOOSE_BLE false
#endif

#ifdef CONFIG_JOYLINK_BLE_ENABLE
#include "joylink_sdk.h"
#include "joylink_ble.h"
#include "joylink_softap.h"

bool joylink_ble_report_flag = false;
extern wifi_config_t  jd_ble_config;
extern bool get_rst;
#endif
static void esp_start_ap_mode(void)
{
    wifi_config_t config;

    memset(&config, 0x0, sizeof(config));
    esp_wifi_set_mode(WIFI_MODE_APSTA);
    printf("ssid:%s\r\n", CONFIG_JOYLINK_SOFTAP_SSID);

    config.ap.ssid_len = strlen(CONFIG_JOYLINK_SOFTAP_SSID);

    if (config.ap.ssid_len > sizeof(config.ap.ssid)) {
        config.ap.ssid_len = sizeof(config.ap.ssid);
    }

    memcpy(config.ap.ssid, CONFIG_JOYLINK_SOFTAP_SSID, config.ap.ssid_len);
    config.ap.max_connection = 3;
    config.ap.channel = 9;
    esp_wifi_set_config(WIFI_IF_AP, &config);
}

static void joylink_task(void *pvParameters)
{
    nvs_handle out_handle;
    wifi_config_t config;
    size_t size = 0;
    bool flag = false;
    int32_t ret = -1;

    esp_wifi_set_mode(WIFI_MODE_STA);

    if (nvs_open("joylink_wifi", NVS_READONLY, &out_handle) == ESP_OK) {
        memset(&config,0x0,sizeof(config));
        size = sizeof(config.sta.ssid);
        if (nvs_get_str(out_handle,"ssid",(char*)config.sta.ssid,&size) == ESP_OK) {
            if (size > 0) {
                size = sizeof(config.sta.password);
                if (nvs_get_str(out_handle,"password",(char*)config.sta.password,&size) == ESP_OK) {
                    flag = true;
                } else {
                    printf("--get password fail");
                }
            }
        } else {
            printf("--get ssid fail");
        }

        nvs_close(out_handle);
    }

    if (flag) {
        esp_wifi_set_config(ESP_IF_WIFI_STA,&config);
        esp_wifi_connect();
    } else if (IS_CHOOSE_BLE == false)
    {
        esp_start_ap_mode();
        joylink_softap_start(1000*60*60);
    } else if (IS_CHOOSE_BLE == true)
    {
#ifdef CONFIG_JOYLINK_BLE_ENABLE
        esp_wifi_stop();
        esp_wifi_start();
        esp_wifi_set_mode(WIFI_MODE_STA);
        ble_start();
        while(1) {
            vTaskDelay(10);
            if(get_rst) {
                joylink_ble_report_flag = true;
                ret = jl_send_net_config_state(E_JL_NET_CONF_ST_WIFI_CONNECT_START);
                log_debug("E_JL_NET_CONF_ST_WIFI_CONNECT_START ret = %d", ret);

                log_debug("SSID: %s\r\n", (char *)jd_ble_config.sta.ssid);
                log_debug("PWD: %s\r\n", (char *)jd_ble_config.sta.password);
                vTaskDelay(100);
                esp_wifi_set_config(ESP_IF_WIFI_STA, &jd_ble_config);
                esp_wifi_connect();

                esp_joylink_wifi_save_info(jd_ble_config.sta.ssid, jd_ble_config.sta.password);
                vTaskDelete(NULL);
                break;
            }
        }
#endif
    }

    vTaskDelete(NULL);
}

static void joylink_main_task(void *pvParameters)
{
    joylink_main_start();
    vTaskDelete(NULL);
}

void esp_joylink_app_start(void)
{
    xTaskCreate(joylink_main_task, "joylink_main_task", 1024*11, NULL, 2, NULL);
    xTaskCreate(joylink_task, "jl_task", 1024*6, NULL, 2, NULL);
}