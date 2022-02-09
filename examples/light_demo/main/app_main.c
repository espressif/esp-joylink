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

#include <stdio.h>

#include "esp_system.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"

#if !defined(ESP_IDF_VERSION)
#include "apps/sntp/sntp.h"
#elif (ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(3,4,0))
#include "apps/sntp/sntp.h"
#else
#include "esp_sntp.h"
#endif
#include "esp_joylink_app.h"
#include "joylink_light.h"

#include "joylink_upgrade.h"

#ifdef CONFIG_JOYLINK_BLE_ENABLE
#include "joylink_sdk.h"
#include "joylink_dev_active.h"

extern bool get_rst;
extern jl_net_config_data_t ble_net_cfg_data;
extern bool joylink_ble_report_flag;
#endif

void esp_ota_task_start(char* url)
{
    ota_task_start(url);
}

esp_err_t event_handler(void *ctx, system_event_t *event)
{
    int32_t ret = -1;
    switch(event->event_id){
        case SYSTEM_EVENT_STA_GOT_IP:
#ifdef CONFIG_JOYLINK_BLE_ENABLE
            if (get_rst) {
                ret = jl_send_net_config_state(E_JL_NET_CONF_ST_WIFI_CONNECT_SUCCEED, NULL, 0);
                log_debug("E_JL_NET_CONF_ST_WIFI_CONNECT_SUCCEED ret = %d", ret);
                printf("token: %s\n", ble_net_cfg_data.token);
                printf("url: %s\n", ble_net_cfg_data.url);
                joylink_dev_active_start((char *)ble_net_cfg_data.url, (char *)ble_net_cfg_data.token);
                free(ble_net_cfg_data.token);
                free(ble_net_cfg_data.url);
                // joylink_dev_active_req();
                joylink_ble_report_flag = false;
            }
#endif
            break;
        default:
            break;
    }
    return ESP_OK;
}

static void initialise_wifi(void)
{
    tcpip_adapter_init();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_AP) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

static void initialize_sntp(void)
{
    /* Start SNTP service */
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
}

void app_main(void)
{
    printf("================== ESPRESSIF ===================\n");
    printf("    ESP_IDF VERSION: %s\n",esp_get_idf_version());
    printf("    JOYLINK COMMIT: %s\n",ESP_JOYLINK_COMMIT_ID);
    printf("    Compile time: %s %s\n",__DATE__,__TIME__);
    printf("================================================\n");

    nvs_flash_init();
    initialise_wifi();
    initialize_sntp();
    light_init();
    
    esp_joylink_app_start();
}
