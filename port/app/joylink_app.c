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

#include "esp_gap_ble_api.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "jd_innet.h"
#include "joylink_dev.h"
#include "esp_joylink.h"

#define GATTS_TAG "joylink"

bool joylink_net_configuaring = false;
xTimerHandle joylink_check_timer_for_10_min_timer;


extern bool jd_innet_timer_task_flag;
extern xTaskHandle jd_innet_timer_task_handle;

void joylink_delay_10_min_timer_for_10_min_start(void);

extern int joylink_main_start();

extern void joylink_ble_init(void);
extern void joylink_gatts_adv_data_enable(void);
void joylink_entry_net_config(void);

static void joylink_main_task(void *pvParameters)
{
    joylink_main_start();
    vTaskDelete(NULL);
}

static void joylink_task(void *pvParameters)
{
    nvs_handle out_handle;
    wifi_config_t config;
    size_t size = 0;
    bool flag = false;
    esp_joylink_config_network_t config_mode = esp_joylink_get_config_network();
    ets_printf("config_mode=%d\r\n",config_mode);
    esp_wifi_set_mode(WIFI_MODE_STA);
    if ((config_mode > ESP_JOYLINK_CONFIG_NETWORK_NONE) && (config_mode <= ESP_JOYLINK_CONFIG_NETWORK_MAX)) {
        esp_joylink_set_config_network(ESP_JOYLINK_CONFIG_NETWORK_NONE);
        
        if ((config_mode == ESP_JOYLINK_CONFIG_NETWORK_SMNT_BLE) || (config_mode == ESP_JOYLINK_CONFIG_NETWORK_SMNT)) {
            joylink_entry_net_config();
        } else if (config_mode == ESP_JOYLINK_CONFIG_NETWORK_SOFTAP) {
            esp_joylink_softap_innet();
        }
    } else {
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

           joylink_net_configuaring = true;
           joylink_delay_10_min_timer_for_10_min_start();
           joylink_gatts_adv_data_enable();
        } else {
           joylink_entry_net_config();
        }
    }

    vTaskDelete(NULL);
}

void esp_joylink_wifi_clear_info(void)
{
    nvs_handle out_handle;
    if (nvs_open("joylink_wifi", NVS_READWRITE, &out_handle) == ESP_OK) {
        nvs_erase_all(out_handle);
        nvs_close(out_handle);
    }
}

void esp_joylink_wifi_save_info(uint8_t*ssid,uint8_t*password)
{
    nvs_handle out_handle;
    char data[65];
    if (nvs_open("joylink_wifi", NVS_READWRITE, &out_handle) != ESP_OK) {
        return;
    }

    memset(data,0x0,sizeof(data));
    strncpy(data,(char*)ssid,strlen((char*)ssid));
    if (nvs_set_str(out_handle,"ssid",data) != ESP_OK) {
        printf("--set ssid fail");
    }

    memset(data,0x0,sizeof(data));
    strncpy(data,(char*)password,strlen((char*)password));
    if (nvs_set_str(out_handle,"password",data) != ESP_OK) {
        printf("--set password fail");
    }
    nvs_close(out_handle);
}

void joylink_check_timer_for_10_min( TimerHandle_t xTimer )
{
    nvs_handle out_handle;
    wifi_config_t config;
    size_t size;
    joylink_net_configuaring = false;
    ets_printf("/*----------------BLE-ADV-STOP(10min)----------------*/\n");
    esp_ble_gap_stop_advertising();
    if (jd_innet_timer_task_handle != NULL) {
        jd_innet_timer_task_flag = true;
    }
    esp_wifi_set_promiscuous(0);
    if(xTimerDelete(joylink_check_timer_for_10_min_timer,0) == pdPASS) {
        joylink_check_timer_for_10_min_timer = NULL;
    }

    if (nvs_open("joylink_wifi", NVS_READONLY, &out_handle) == ESP_OK) {
        memset(&config,0x0,sizeof(config));
        size = sizeof(config.sta.ssid);
        if (nvs_get_str(out_handle,"ssid",(char*)config.sta.ssid,&size) == ESP_OK) {
            if (size > 0) {
                size = sizeof(config.sta.password);
                if (nvs_get_str(out_handle,"password",(char*)config.sta.password,&size) == ESP_OK) {
                    esp_wifi_set_config(ESP_IF_WIFI_STA,&config);
                    esp_wifi_connect();
                } else {
                    printf("--get password fail");
                }
            }
        } else {
            printf("--get ssid fail");
        }

        nvs_close(out_handle);
    }
}

void joylink_delay_10_min_timer_for_10_min_start(void)
{
    if (joylink_check_timer_for_10_min_timer == NULL) {
        joylink_check_timer_for_10_min_timer = xTimerCreate("check",(10*60*1000)/(1000 /xPortGetTickRateHz()),pdFALSE,NULL,joylink_check_timer_for_10_min);
        xTimerStart(joylink_check_timer_for_10_min_timer,100 );
    } else {
        xTimerReset(joylink_check_timer_for_10_min_timer,100);
    }
}

void joylink_delay_10_min_timer_for_10_min_stop(void)
{
    if (joylink_check_timer_for_10_min_timer) {
        if (xTimerDelete(joylink_check_timer_for_10_min_timer,100) == pdTRUE) {
            joylink_check_timer_for_10_min_timer = NULL;
        }
    }
}

void joylink_entry_net_config(void)
{
    ets_printf("--joylink net config\r\n");
    joylink_net_configuaring = true;
    joylink_delay_10_min_timer_for_10_min_start();
    joylink_gatts_adv_data_enable();
    ets_printf("/*----------------CFG-ADV-DATA(NET-CFG)----------------*/\n");
    jd_innet_start_task();
}

void esp_joylink_app_start(void)
{
    xTaskCreate(joylink_main_task, "joylink_main_task", 1024*10, NULL, 2, NULL);
    joylink_ble_init();
    xTaskCreate(joylink_task, "jl_task", 2048, NULL, 1, NULL);
}
