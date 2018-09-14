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


#include "string.h"

#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_system.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sys/socket.h"
#include "netdb.h"

#include "joylink.h"

#include "jd_innet.h"

#include "joylink_smnt.h"

#include "esp_joylink.h"

#include "joylink_log.h"
#include "nvs.h"
#include "nvs_flash.h"

#define AES_KEY  CONFIG_JOYLINK_SMNT_AES_KEY

xTaskHandle jd_innet_timer_task_handle = NULL;
bool jd_innet_timer_task_flag = false;
void joylink_delay_3_min_timer_for_adv(void);


/*recv packet callback*/
static void jd_innet_pack_callback(void *buf, wifi_promiscuous_pkt_type_t type)
{

    wifi_promiscuous_pkt_t *pack_all = NULL;
    int len=0;
    PHEADER_802_11 frame = NULL;

    if (type != WIFI_PKT_MISC) {
        pack_all = (wifi_promiscuous_pkt_t *)buf;
        frame = (PHEADER_802_11)pack_all->payload;
        len = pack_all->rx_ctrl.sig_len;
		joylink_smnt_datahandler(frame, len);

    }
}

#define CHANNEL_ALL_NUM  (13*3)
#define CHANNEL_BIT_MASK ((uint64_t)0x5B6FFFF6DB)

void esp_switch_channel_callback(unsigned char channel)
{
    static wifi_second_chan_t second_ch = WIFI_SECOND_CHAN_NONE; //0,1,2
    // log_debug("ch:%d, sec:%d \r\n", i, second_ch);
    esp_wifi_set_channel(channel, second_ch);
    second_ch++;
    if (second_ch > WIFI_SECOND_CHAN_BELOW){
        second_ch = WIFI_SECOND_CHAN_NONE;
        return ; //must to change channel
    } else {
        return ; //don't to change channel
    }
}

void esp_get_result_callback(joylink_smnt_result_t result)
{
	wifi_config_t config;
	if (result.smnt_result_status == smnt_result_ok) {
        memset(&config,0x0,sizeof(config));
                    
        memcpy(config.sta.password,result.jd_password,result.jd_password_len);
        memcpy(config.sta.ssid,result.jd_ssid,result.jd_ssid_len);
        log_debug("ssid:%s\r\n",config.sta.ssid);
        log_debug("password:%s\r\n",config.sta.password);
        if (esp_wifi_set_config(ESP_IF_WIFI_STA,&config) != ESP_OK) {
            log_debug("set sta fail\r\n");
        } else {
            if (esp_wifi_connect() != ESP_OK) {
                log_debug("sta connect fail\r\n");
            } else {
                if (jd_innet_timer_task_handle != NULL) {
                    jd_innet_timer_task_flag = true;
                }
                esp_wifi_set_promiscuous(0);
				esp_wifi_set_promiscuous_rx_cb(NULL);
                // save flash
                esp_joylink_wifi_save_info(config.sta.ssid,config.sta.password);
                //ble delay 3 min
                joylink_delay_3_min_timer_for_adv();
				// joylink_smnt_release();
            }
        }
	}
}

static void jd_innet_timer_task (void *pvParameters) // if using timer,it will stack overflow because of log in joylink_cfg_50msTimer;
{
    int32_t delay_tick = 50;
    for (;;) {
        if (delay_tick == 0) {
            delay_tick = 1;
        }
        vTaskDelay(delay_tick);
        if (jd_innet_timer_task_flag) {
            break;
        }
        delay_tick = joylink_smnt_cyclecall()/portTICK_PERIOD_MS;
    }

    jd_innet_timer_task_handle = NULL;
    jd_innet_timer_task_flag = false;
    log_debug("jd_innet_timer_task delete\r\n");
    vTaskDelete(NULL);
}

static void jd_innet_start (void *pvParameters)
{
    unsigned char* pBuffer = NULL;
    joylink_smnt_param_t param;
    esp_wifi_set_promiscuous(0);

    pBuffer = (unsigned char*)malloc(1024);
    if (pBuffer == NULL) {
        log_debug("%s,%d\r\n",__func__,__LINE__);
        vTaskDelete(NULL);
    }
    
	memset(&param,0x0,sizeof(param));
	memcpy(param.secretkey,AES_KEY,sizeof(param.secretkey));
	param.switch_channel_callback = esp_switch_channel_callback;
	param.get_result_callback = esp_get_result_callback;
    joylink_smnt_init(param);
    if (ESP_OK != esp_wifi_set_promiscuous_rx_cb(jd_innet_pack_callback)){
        log_debug ("[%s] set_promiscuous fail\n\r",__func__);
    }

    if (ESP_OK != esp_wifi_set_promiscuous(1)){
        log_debug ("[%s] open promiscuous fail\n\r",__func__);
    }

    if (jd_innet_timer_task_handle != NULL) {
        jd_innet_timer_task_flag = true;
        while(jd_innet_timer_task_flag) {
            vTaskDelay(10);
        }
    }
    xTaskCreate(jd_innet_timer_task, "jd_innet_timer_task", 2048, NULL, tskIDLE_PRIORITY + 5, &jd_innet_timer_task_handle);

    vTaskDelete(NULL);
}

void jd_innet_start_task(void)
{
    xTaskCreate(jd_innet_start, "jd_innet_start", 1024*3, NULL, tskIDLE_PRIORITY + 2, NULL);
}

