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



/****************************************************************************
*
* This file is for gatt client. It can scan ble device, connect one device,
*
****************************************************************************/

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

#include "esp_bt.h"
#include "bta_api.h"

#include "esp_bt_device.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"

#include "apps/sntp/sntp.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "button.h"
#include "driver/gpio.h"

#include "esp_joylink.h"

#define GATTS_TAG "GATTS_DEMO"

#define PROFILE_NUM 1
#define PROFILE_JD_APP_ID 0


struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
};

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_JD_APP_ID] = {
        .gatts_cb = esp_joylink_ble_gatts_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
        .app_id   = PROFILE_JD_APP_ID,
    }
};

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    // joylink_ble_gap_event_handler(event, param);
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gl_profile_tab[param->reg.app_id].gatts_if = gatts_if;
        } else {
            ESP_LOGI(GATTS_TAG, "Reg app failed, app_id %04x, status %d\n",param->reg.app_id, param->reg.status);
            return;
        }
    }

    /* If the gatts_if equal to profile A, call profile A cb handler,
         * so here call each profile's callback */
    {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            if (gatts_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                    gatts_if == gl_profile_tab[idx].gatts_if) {
                if (gl_profile_tab[idx].gatts_cb) {
                    gl_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    }

}

void initialise_ble(void)
{
    int ret;
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s initialize controller failed\n", __func__);
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ets_printf("%s enable bt controller failed\n", __func__);
        return;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s init bluetooth failed\n", __func__);
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(GATTS_TAG, "%s enable bluetooth failed\n", __func__);
        return;
    }

    esp_ble_gatts_register_callback(gatts_event_handler);
    esp_ble_gap_register_callback(gap_event_handler);

    {
        int loop = 0;
        for (loop = 0;loop < PROFILE_NUM;loop++) {
            esp_ble_gatts_app_register(gl_profile_tab[loop].app_id);
        }
    }
	
}

esp_err_t event_handler(void *ctx, system_event_t *event)
{
     return esp_joylink_wifi_event_handler(ctx,event);
}

static void initialise_wifi(void)
{
    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );

    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_NULL) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

static void joylink_button_softap_tap_cb(void* arg)
{
    esp_joylink_set_config_network(ESP_JOYLINK_CONFIG_NETWORK_SOFTAP);
    esp_restart();
}

static void joylink_button_smnt_tap_cb(void* arg)
{
    esp_joylink_set_config_network(ESP_JOYLINK_CONFIG_NETWORK_SMNT_BLE);
    esp_restart();
}

static void joylink_button_reset_tap_cb(void* arg)
{
    esp_restore_factory_setting();
    esp_restart();
}

static void initialise_key(void)
{
    button_handle_t btn_handle = button_dev_init(CONFIG_JOYLINK_SMNT_BUTTON_NUM, 0, BUTTON_ACTIVE_LOW);
    button_dev_add_tap_cb(BUTTON_PUSH_CB, joylink_button_smnt_tap_cb, "PUSH", 50 / portTICK_PERIOD_MS, btn_handle);
    btn_handle = button_dev_init(CONFIG_JOYLINK_RESET_BUTTON_NUM, 0, BUTTON_ACTIVE_LOW);
    button_dev_add_tap_cb(BUTTON_PUSH_CB, joylink_button_reset_tap_cb, "PUSH", 50 / portTICK_PERIOD_MS, btn_handle);
    btn_handle = button_dev_init(CONFIG_JOYLINK_SOFTAP_BUTTON_NUM, 0, BUTTON_ACTIVE_LOW);
    button_dev_add_tap_cb(BUTTON_PUSH_CB, joylink_button_softap_tap_cb, "PUSH", 50 / portTICK_PERIOD_MS, btn_handle);
}

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}

void app_main(void)
{
    printf("================== ESPRESSIF ===================\n");
    printf("    ESP_IDF VERSION: %s\n",esp_get_idf_version());
    printf("    JOYLINK VERSION: %s(%s)\n",JOYLINK_VERSION,JOYLINK_COMMIT_ID);
    printf("    Compile time: %s %s\n",__DATE__,__TIME__);
    printf("================================================\n");
    nvs_flash_init();
    initialise_wifi();
	initialize_sntp();
    initialise_key();
    initialise_ble();

    esp_joylink_app_start();
}
