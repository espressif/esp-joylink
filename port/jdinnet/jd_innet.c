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
#include "joylink_thunder_slave_sdk.h"
#include "joylink_auth_md5.h"
#include "joylink_dev.h"
#include "joylink_utils.h"

#define AES_KEY CONFIG_JOYLINK_SMNT_AES_KEY 

#define JOYLINK_SERVER "sbdevicegw.jd.com"
#define JOYLINK_PORT 2002

xTaskHandle jd_innet_timer_task_handle = NULL;
bool jd_innet_timer_task_flag = false;

uint8_t config_stop_flag = 0;

typedef struct _Result
{
	uint8_t ssid_len;
	uint8_t pass_len;
	
	char ssid[33];
	char pass[33];
} Result_t;

Result_t config_result;

/*recv packet callback*/
static void jd_innet_pack_callback(void *buf, wifi_promiscuous_pkt_type_t type)
{

    wifi_promiscuous_pkt_t *pack_all = NULL;
    int len=0;
    PHEADER_802_11 frame = NULL;

    if (type != WIFI_PKT_MISC) {
        pack_all = (wifi_promiscuous_pkt_t *)buf;
        frame = (PHEADER_802_11)pack_all->payload;        
        len = pack_all->rx_ctrl.sig_mode ? pack_all->rx_ctrl.HT_length : pack_all->rx_ctrl.legacy_length;
        joylink_80211_recv(frame, len);
    }
}

#define CHANNEL_ALL_NUM  (13*3)
#define CHANNEL_BIT_MASK ((uint64_t)0x5B6FFFF6DB)

void esp_switch_channel_callback(unsigned char channel)
{
    esp_err_t ret;
    static wifi_second_chan_t second_ch = WIFI_SECOND_CHAN_NONE; //0,1,2
    ret = esp_wifi_set_channel(channel, second_ch);
    // log_debug("channel:%d, second_ch = %d, ret =%d\r\n", channel,second_ch, ret);
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
        esp_wifi_set_promiscuous(0);
        esp_wifi_set_promiscuous_rx_cb(NULL);
        memcpy(config.sta.ssid,result.jd_ssid,result.jd_ssid_len);
        memcpy(config.sta.password,result.jd_password,result.jd_password_len);
        log_debug("ssid:%s\r\n",config.sta.ssid);
        log_debug("password:%s\r\n",config.sta.password);
        esp_joylink_wifi_save_info(config.sta.ssid,config.sta.password);
        esp_wifi_disconnect();
        if (esp_wifi_set_config(ESP_IF_WIFI_STA,&config) != ESP_OK) {
            log_debug("set sta fail\r\n");
        } else {
            if (esp_wifi_connect() != ESP_OK) {
                log_debug("sta connect fail\r\n");
            } else {
                if (jd_innet_timer_task_handle != NULL) {
                    jd_innet_timer_task_flag = true;
                }
            }
        }
	}
}

tc_slave_result_t			tc_slave_result;

static void jd_innet_result_task (void *pvParameters) 
{
    wifi_config_t config;
    tc_slave_result_t	*tthunderresult = &tc_slave_result;

    joylink_thunder_slave_finish(tthunderresult);

    if (tthunderresult->errorcode == MSG_OK) {
        memset(&config,0x0,sizeof(config));
        esp_wifi_set_promiscuous(0);
        esp_wifi_set_promiscuous_rx_cb(NULL);
        memcpy(config.sta.ssid, config_result.ssid, config_result.ssid_len);
        memcpy(config.sta.password, config_result.pass, config_result.pass_len);
        log_debug("ssid:%s\r\n",config.sta.ssid);
        log_debug("password:%s\r\n",config.sta.password);
        esp_joylink_wifi_save_info(config.sta.ssid,config.sta.password);
        esp_wifi_disconnect();
        if (esp_wifi_set_config(ESP_IF_WIFI_STA,&config) != ESP_OK) {
            log_debug("set sta fail\r\n");
        } else {
            if (esp_wifi_connect() != ESP_OK) {
                log_debug("sta connect fail\r\n");
            } else {
                if (jd_innet_timer_task_handle != NULL) {
                    jd_innet_timer_task_flag = true;
                }
            }
        }
    } else {
        log_debug("tthunderresult->errorcode = %d",tthunderresult->errorcode);
    }

    log_debug("Exit jd_innet_result_task, free heap size = %d\n", esp_get_free_heap_size());
    vTaskDelete(NULL);
}

static void jd_innet_timer_task (void *pvParameters) // if using timer,it will stack overflow because of log in joylink_cfg_50msTimer;
{
    int32_t delay_tick = 50;
    uint8_t ret = 0;

    log_debug("jd_innet_timer_task enter\r\n");

    for (;;) {
        if (delay_tick == 0) {
            delay_tick = 1;
        }

        vTaskDelay(delay_tick);
        if (jd_innet_timer_task_flag) {
            break;
        }

        ret = joyThunderSlave50mCycle();
        delay_tick = 5;
        if (ret) {
            goto exit;
        }
    }

exit:

    jd_innet_timer_task_handle = NULL;
    jd_innet_timer_task_flag = false;

    esp_err_t cre_ret = xTaskCreate(jd_innet_result_task, "jd_innet_result_task", 1024*6, NULL, tskIDLE_PRIORITY + 5, NULL);
    log_debug("Exit jd_innet_timer_task, free heap size = %d\n", esp_get_free_heap_size());

    vTaskDelete(NULL);
}

int joylink_config_stop(void)
{
	config_stop_flag = 1;
	joyThunderSlaveStop();
    return 0;
}

int joylink_get_random(void)
{
    return esp_random();
}

void joylink_change_hannel(int ch)
{
    esp_switch_channel_callback(ch);
    return;
}

void joylink_80211_recv(uint8_t *buf, int buflen)
{
    if(buflen > 70){
        if ((buf[64] == 0x4A)&&(buf[65] == 0x4F)&&(buf[66] == 0x59)) {
            joylink_util_fmt_p("joylink_80211_recv:",buf,buflen);
            joyThunderSlaveProbeH(buf, buflen);
        }
    }
}

int joylink_delete_mark(uint8_t *str)
{
	int for_i = 0;
	int for_n = 0;

	uint8_t temp[64] = {0};

	if(str == NULL)	return -1;
	
	for(for_i = 0; for_i < strlen(str); for_i++)
	{
		if((str[for_i] >= '0' && str[for_i] <= '9') \
		|| (str[for_i] >= 'a' && str[for_i] <= 'f') \
		|| (str[for_i] >= 'A' && str[for_i] <= 'F')){
			temp[for_n] = str[for_i];
			for_n++;
		}
	}
	memset(str, 0, sizeof(str));
	memcpy(str, temp, strlen(temp));

	return 0;
}

int joylink_80211_send(uint8_t *buf, int buflen)
{
    esp_err_t ret;

    if( buflen != 60  ){
        joylink_util_fmt_p("joylink_80211_send:",buf,buflen);
    }

    ret = esp_wifi_80211_tx(ESP_IF_WIFI_STA, buf, buflen, true);

    if (ret != ESP_OK) {
        log_error("send fail , ret = %d\r\n", ret);
    }

	return 0;
}

extern E_JLRetCode_t joylink_dev_set_attr_jlp(JLPInfo_t *jlp);

int joylink_thunder_slave_finish(tc_slave_result_t *result)
{
    JLPInfo_t jlp;
	uint8_t temp[32] = {0};
	uint8_t localkey[33] = {0};
	MD5_CTX md5buf;
	
    log_info("joylink thunder slave finish, %d\r\n",sizeof(JLPInfo_t));

	memset(&jlp, 0, sizeof(JLPInfo_t));
    memcpy(jlp.feedid, result->cloud_feedid.value, result->cloud_feedid.length);
	memcpy(jlp.accesskey, result->cloud_ackey.value, result->cloud_ackey.length);

	memset(&md5buf, 0, sizeof(MD5_CTX));
	JDMD5Init(&md5buf);
	JDMD5Update(&md5buf, result->cloud_ackey.value, result->cloud_ackey.length);
	JDMD5Final(&md5buf, temp);
	joylink_util_byte2hexstr(temp, 16, localkey, 32);
	
	memcpy(jlp.localkey, localkey, strlen(localkey));
    memcpy(jlp.joylink_server, JOYLINK_SERVER, strlen(JOYLINK_SERVER));
    jlp.server_port = JOYLINK_PORT;

    log_info("feedid:%s, accesskey:%s, localkey: %s, serverurl:%s\n", result->cloud_feedid.value, result->cloud_ackey.value, localkey, result->cloud_server.value);

    _g_pdev->jlp.is_actived = 1;
    joylink_dev_set_attr_jlp(&jlp);

	memset(&config_result, 0, sizeof(Result_t));
	
	memcpy(config_result.ssid, result->ap_ssid.value, result->ap_ssid.length);
	config_result.ssid_len = result->ap_ssid.length;
	
	memcpy(config_result.pass, result->ap_password.value, result->ap_password.length);
	config_result.pass_len = result->ap_password.length;
	
    log_info("ssid:%s, passwd:%s\n", config_result.ssid, config_result.pass);

    // joylink_dev_init();
    vTaskDelay(200);
    
	joylink_config_stop();

	return 0;
}

/**
 * brief: 
 *
 * @Param: thunder slave init and finish
 *
 * @Returns: 
 */
extern Joylink_info_t joylink_info;

int joylink_thunder_slave_init(void)
{
    JLPInfo_t user_jlp;
    jl2_d_idt_t user_idt;
	tc_slave_func_param_t thunder_param;

    memcpy(&user_jlp,&(joylink_info.jlp) , sizeof(JLPInfo_t));
    memcpy(&user_idt,&(joylink_info.idt) , sizeof(jl2_d_idt_t));

	log_info("init thunder slave!\r\n");

	memset(&thunder_param,0,sizeof(tc_slave_func_param_t));

	memcpy(thunder_param.uuid, user_jlp.uuid, 6);

	joylink_delete_mark(user_jlp.mac);
	joylink_util_hexStr2bytes(user_jlp.mac, thunder_param.mac_dev, JOY_MAC_ADDRESS_LEN);

	joylink_util_hexStr2bytes(user_jlp.prikey, thunder_param.prikey_d, JOY_ECC_PRIKEY_LEN);
	joylink_util_hexStr2bytes(user_idt.cloud_pub_key, thunder_param.pubkey_c, JOY_ECC_PUBKEY_LEN);

	thunder_param.deviceid.length = strlen(thunder_param.mac_dev);
	thunder_param.deviceid.value = joylink_util_malloc(thunder_param.deviceid.length);
	memcpy(thunder_param.deviceid.value, thunder_param.mac_dev, thunder_param.deviceid.length);

	thunder_param.switch_channel = (switch_channel_cb_t)joylink_change_hannel;
	thunder_param.get_random	 = 	(get_random_cb_t)joylink_get_random;
	thunder_param.result_notify_cb = (thunder_finish_cb_t)joylink_thunder_slave_finish;
	thunder_param.packet_80211_send_cb = (packet_80211_send_cb_t)joylink_80211_send;
	joyThunderSlaveInit(&thunder_param);

	joyThunderSlaveStart();

	joylink_util_free(thunder_param.deviceid.value);
	thunder_param.deviceid.value = NULL;
	return 0;
}

static void jd_innet_start (void *pvParameters)
{
    log_debug ("[%s] jd_innet_start enter\n\r",__func__);
    
    config_stop_flag = 0;
    joylink_thunder_slave_init();

#define WIFI_PROMIS_FILTER_MASK_PROREQ  (1<<7)
#define WIFI_PROMIS_FILTER_MASK_PRORSP  (1<<8)
    const wifi_promiscuous_filter_t filter = {
        .filter_mask = (WIFI_PROMIS_FILTER_MASK_MGMT | WIFI_PROMIS_FILTER_MASK_PROREQ |WIFI_PROMIS_FILTER_MASK_PRORSP),
    };
	esp_wifi_set_promiscuous_len_filter(200);
	esp_wifi_set_promiscuous_data_len(200);
    esp_wifi_set_promiscuous_filter(&filter);

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
    esp_err_t ret = xTaskCreate(jd_innet_timer_task, "jd_innet_timer_task", 2048, NULL, tskIDLE_PRIORITY + 5, &jd_innet_timer_task_handle);
    log_debug("Exit jd_innet_start, free heap size = %d\n", esp_get_free_heap_size());
    vTaskDelete(NULL);
}

void jd_innet_start_task(void)
{
    xTaskCreate(jd_innet_start, "jd_innet_start", 1024*6, NULL, tskIDLE_PRIORITY + 2, NULL);
}

