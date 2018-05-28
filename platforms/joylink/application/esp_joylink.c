/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "esp_common.h"
#include "freertos/semphr.h"
#include "cJSON.h"
#include "jd_innet.h"
#include "joylink_softap_extern.h"
#include "esp_joylink.h"
#include "esp_info_store.h"
#include "esp_joylink_log.h"
#include "joylink_aes.h"

static const char *TAG = "esp_joylink";

static xSemaphoreHandle xSemJoyRead = NULL;
static xSemaphoreHandle xSemJoyWrite = NULL;
static xSemaphoreHandle xSemJoyData = NULL;
static xQueueHandle xQueueJoyUp = NULL;
xSemaphoreHandle xSemJoyReply = NULL;
xQueueHandle xQueueJoyDown = NULL;
xQueueHandle xQueueJoyEvent = NULL;
static joylink_event_cb_t s_joylink_event_cb = NULL;

joylink_err_t joylink_get_dev_json_status(uint8_t *out_data, int max_len)
{
    xSemaphoreTake(xSemJoyData, portMAX_DELAY);
    JOYLINK_PARAM_CHECK(!out_data);
    JOYLINK_PARAM_CHECK(max_len <= 0);

    char *up_cmd = NULL;

    if (xQueueReceive(xQueueJoyUp, &up_cmd, portMAX_DELAY) == pdFALSE) {
        JOYLINK_LOGE("xQueueReceive xQueueJoyUp fail");
        free(up_cmd);
        xSemaphoreGive(xSemJoyData);
        return JOYLINK_ERR;
    }

    int ret = 0;
    int size = strlen(up_cmd)+1;

    if (size > max_len-1) {
        memcpy(out_data, up_cmd, max_len);
        out_data[max_len] = '\0';
        ret = max_len;
    } else {
        memcpy(out_data, up_cmd, size);
        ret = size;
    }

    free(up_cmd);
    xSemaphoreGive(xSemJoyData);
    return ret;
}

joylink_err_t joylink_get_status_cmd_up(int max_len, uint8_t to_server, uint8_t is_json)
{
    joylink_event_param_t *param = (joylink_event_param_t *)malloc(sizeof(joylink_event_param_t));
    param->get.is_server_cmd = to_server;
    param->get.is_json = is_json;
    param->get.max_len = max_len;

    if (JOYLINK_OK != joylink_event_send(JOYLINK_EVENT_GET_DEVICE_DATA, param)) {
        return JOYLINK_ERR;
    }

    /* ctrl dev ok, wait data readly */
	if (pdTRUE != xSemaphoreTake(xSemJoyReply, JOYLINK_WAIT_DEV_STATUS_TIMEOUT / portTICK_RATE_MS)) {
        return JOYLINK_ERR;
    }

    JOYLINK_LOGD("get device status %s %s", to_server?"to server":"to lan", is_json?"json":"script");

    return JOYLINK_OK;
}

joylink_err_t joylink_ctrl_cmd_down(const char *down_cmd, uint8_t from_server, uint8_t is_json)
{
    JOYLINK_PARAM_CHECK(!down_cmd);

    int size = strlen(down_cmd) + 1;
	char *pdata = (char *)malloc(size);
	memset(pdata, 0, size);
	memcpy(pdata, down_cmd, size);

	if (xQueueSend(xQueueJoyDown, &pdata, 0) != pdTRUE) {
		JOYLINK_LOGE("xQueueSend xQueueJoyDown error");
		free(pdata);
		return JOYLINK_ERR;
	}

    joylink_event_param_t *param = (joylink_event_param_t *)malloc(sizeof(joylink_event_param_t));
    param->set.is_server_cmd = from_server;
    param->set.is_json = is_json;
    param->set.cmd_len = strlen(down_cmd);

    if (JOYLINK_OK != joylink_event_send(JOYLINK_EVENT_SET_DEVICE_DATA, param)) {
        return JOYLINK_ERR;
    }

    return JOYLINK_OK;
}

ssize_t esp_joylink_read(void *down_cmd, size_t size, int milli_seconds)
{
	xSemaphoreTake(xSemJoyRead, portMAX_DELAY);
	JOYLINK_PARAM_CHECK(!down_cmd);
	JOYLINK_PARAM_CHECK((size <= 0) || (size > JL_MAX_PACKET_LEN));

	int ret = JOYLINK_OK;
	char *pdata = NULL;

	if (xQueueReceive(xQueueJoyDown, &pdata, milli_seconds / portTICK_RATE_MS) == pdFALSE) {
		JOYLINK_LOGE("xQueueReceive xQueueDownCmd fail, wait_time: %d", milli_seconds);

		if(NULL != pdata) {
	        free(pdata);
	    }

	    xSemaphoreGive(xSemJoyRead);
	    return JOYLINK_ERR;
	}

	int size_tmp = strlen(pdata)+1;

	if (size_tmp > size) {
	    JOYLINK_LOGE("down cmd len is error, down cmd len: %d", size_tmp);

        if(NULL != pdata) {
            free(pdata);
        }

        xSemaphoreGive(xSemJoyRead);
        return JOYLINK_ERR;
	}

	memcpy(down_cmd, pdata, size_tmp);

	if(NULL != pdata) {
		free(pdata);
	}

	xSemaphoreGive(xSemJoyRead);
	return size_tmp;
}

ssize_t esp_joylink_write(void *up_cmd, size_t len, int milli_seconds)
{
	xSemaphoreTake(xSemJoyWrite, portMAX_DELAY);
	JOYLINK_PARAM_CHECK(!up_cmd);
	JOYLINK_PARAM_CHECK((len <= 0) || (len > JL_MAX_PACKET_LEN));

    char *pdata = (char *)malloc(len);
    memset(pdata, 0, len);
    memcpy(pdata, up_cmd, len);

    if (xQueueSend(xQueueJoyUp, &pdata, milli_seconds) != pdTRUE) {
        JOYLINK_LOGE("xQueueSend xQueueJoyUp error, wait_time: %d", milli_seconds);
        free(pdata);
        xSemaphoreGive(xSemJoyWrite);
        return JOYLINK_ERR;
    } else {
        xSemaphoreGive(xSemJoyReply); /* ctrl dev ok, data readly */
        xSemaphoreGive(xSemJoyWrite);
        return len;
    }
}

joylink_err_t joylink_trans_init()
{
	if(NULL == xSemJoyRead)
    	xSemJoyRead = xSemaphoreCreateMutex();
    JOYLINK_PARAM_CHECK(xSemJoyRead == NULL);
	if(NULL == xSemJoyWrite)
    	xSemJoyWrite = xSemaphoreCreateMutex();
    JOYLINK_PARAM_CHECK(xSemJoyWrite == NULL);
	if(NULL == xSemJoyData)
    	xSemJoyData = xSemaphoreCreateMutex();
    JOYLINK_PARAM_CHECK(xSemJoyData == NULL);
	if(NULL == xSemJoyReply)
    	xSemJoyReply = (xSemaphoreHandle)xSemaphoreCreateBinary();
    JOYLINK_PARAM_CHECK(xSemJoyReply == NULL);
	if(NULL == xQueueJoyDown)
    	xQueueJoyDown = xQueueCreate(JOYLINK_DOWN_Q_NUM, sizeof(char*));
    JOYLINK_PARAM_CHECK(xQueueJoyDown == NULL);	
	if(NULL == xQueueJoyUp)
    	xQueueJoyUp = xQueueCreate(JOYLINK_UP_Q_NUM, sizeof(char*));
    JOYLINK_PARAM_CHECK(xQueueJoyUp == NULL);

    return JOYLINK_OK;
}

joylink_err_t joylink_event_send(joylink_event_t event, joylink_event_param_t *para)
{
    joylink_action_t action;
    action.event = event;
    action.arg = (void*)para;

    if (xQueueJoyEvent == NULL) {
        xQueueJoyEvent = xQueueCreate(JOYLINK_EVENT_QUEUE_NUM, sizeof(joylink_action_t));
    }

    if (xQueueSend(xQueueJoyEvent, &action, 0) != pdTRUE) {
        JOYLINK_LOGE("xQueueSendToBack xQueueJoyEvent fail!");
        return JOYLINK_ERR;
    }

    return JOYLINK_OK;
}

void joylink_event_ctrl_task(void *arg)
{
	/* loop to recv event, and callback */
	for(;;) {
	    joylink_action_t action;

		if (xQueueReceive(xQueueJoyEvent, &action, portMAX_DELAY) == pdPASS) {

		    if ((*s_joylink_event_cb)(action) != JOYLINK_OK) {
                JOYLINK_LOGE("event handle fail!");
            }

			if (action.arg != NULL) {
                free(action.arg);
                action.arg = NULL;
            }
		}

        JOYLINK_LOGD("highwater %d",uxTaskGetStackHighWaterMark(NULL));
	}

	vTaskDelete(NULL);
}

joylink_err_t esp_joylink_event_init(joylink_event_cb_t cb)
{
	if (cb != NULL) {
		s_joylink_event_cb = cb;
	}

	if (xQueueJoyEvent == NULL) {
		xQueueJoyEvent = xQueueCreate(JOYLINK_EVENT_QUEUE_NUM, sizeof(joylink_action_t));
	}

	xTaskCreate(joylink_event_ctrl_task, "jl_event", JOYLINK_EVENT_TASK_STACK, NULL, JOYLINK_TASK_PRIOTY, NULL);
	return JOYLINK_OK;
}

void joylink_restore_factory(void *para)
{
    JOYLINK_LOGI("clear joylink config and restart system");
    esp_info_erase(JOYLINK_CONFIG_KEY); /* clear joylink config (include feedid, accesskey, and so on) */
    system_restore();
    system_restart();
}

void joylink_reset_to_smartconfig(void *para)
{
    JOYLINK_LOGI("clear wifi config");
    esp_info_erase(NVS_KEY_WIFI_CONFIG);

    JOYLINK_LOGI("restart and enter smartconfig");
    int config_mode = CONFIG_MODE_SMARTCONFIG;
    esp_info_save(WIFI_CONFIG_MODE_KEY, &config_mode, sizeof(int));
    system_restart();
}

void joylink_reset_to_softap_innet(void *para)
{
    JOYLINK_LOGI("clear wifi config");
    esp_info_erase(NVS_KEY_WIFI_CONFIG);

    JOYLINK_LOGI("restart and enter softap innet config");
    int config_mode = CONFIG_MODE_SOFTAP;
    esp_info_save(WIFI_CONFIG_MODE_KEY, &config_mode, sizeof(int));
    system_restart();
}

int joylink_get_config_mode(void)
{
    int config_mode;
    esp_info_load(WIFI_CONFIG_MODE_KEY, &config_mode, sizeof(int));
    return config_mode;
}

void joylink_wait_station_got_ip(void)
{
    while (1) {

        if (wifi_station_get_connect_status() == STATION_GOT_IP) {
            break;
        }

        vTaskDelay(200 / portTICK_RATE_MS);
    }
}

//extern int joylink_main_start();

static void joylink_task(void *pvParameters)
{
    int rsn = 0;
    struct station_config wifi_config;
    joylink_event_param_t *param = (joylink_event_param_t *)malloc(sizeof(joylink_event_param_t));

    wifi_set_opmode(STATION_MODE);
    int ret = esp_info_load(NVS_KEY_WIFI_CONFIG, &wifi_config, sizeof(struct station_config));
    JOYLINK_LOGI("check sta config, ssid: %s  password: %s", wifi_config.ssid, wifi_config.password);

    if (ret <= 0 ) {  /* without connected AP, start wifi config process */
        int config_mode = joylink_get_config_mode();
        JOYLINK_LOGI("config mode is %d (1:smartconfig  2:softap innet)", config_mode);

        if (config_mode == CONFIG_MODE_SMARTCONFIG) {  /* start innet smart config process */
            joylink_event_send(JOYLINK_EVENT_WIFI_START_SMARTCONFIG, NULL);
            jd_innet_start();
        } else if (config_mode == CONFIG_MODE_SOFTAP) {  /* start softap innet process */
            joylink_event_send(JOYLINK_EVENT_WIFI_START_SOFTAP_CONFIG, NULL);
            joylink_softap_innet();
        } else {
            joylink_event_send(JOYLINK_EVENT_WIFI_START_SMARTCONFIG, NULL);
            jd_innet_start();
        }

        rsn = JOYLINK_GOT_IP_WIFI_CONFIG;
    }else {  /* have connectend AP, connect to AP driect */
        rsn = JOYLINK_GOT_IP_AUTO_CONN;
        wifi_station_connect();
    }


    joylink_wait_station_got_ip();   /* wait connect to ap, no timeout */
    param->conn.rsn = rsn;
    wifi_station_get_config_default(&(param->conn.conf));

    if (memcmp(&wifi_config, &(param->conn.conf), sizeof(struct station_config)) !=0) {
        esp_info_save(NVS_KEY_WIFI_CONFIG, &(param->conn.conf), sizeof(struct station_config));
    }

    JOYLINK_LOGI("connected to %s password is %s", param->conn.conf.ssid, param->conn.conf.password);
    joylink_event_send(JOYLINK_EVENT_WIFI_GOT_IP, param);
    joylink_main_start();  /* joylink SDK loop task */

    vTaskDelete(NULL);
}

joylink_err_t esp_joylink_init(joylink_info_t *joylink_info)
{
    JOYLINK_PARAM_CHECK(!joylink_info);

    /* init joylink config of product */
    memcpy(_g_pdev, &(joylink_info->device_info), sizeof(JLDevice_t));
    /* init wifi config */
    jd_innet_set_aes_key((const char*)(joylink_info->innet_aes_key));
    /* init Queue and Semaphore */
    joylink_trans_init();
    /* init event task */
    xTaskCreate(joylink_task, "jl_task", (1024*5), NULL, 1, NULL);

    return JOYLINK_OK;
}

joylink_err_t joylink_get_jlp_info(joylink_info_t *joylink_info)
{
    JOYLINK_PARAM_CHECK(!joylink_info);

    memcpy(&(joylink_info->device_info), &(_g_pdev), sizeof(JLDevice_t));
    joylink_info->innet_aes_key = jd_innet_get_aes_key();

    return JOYLINK_OK;
}

void esp_joylink_set_version(uint8_t version)
{
    joylink_dev_get_jlp_info(&_g_pdev->jlp);

    if (_g_pdev->jlp.version != version) {
        _g_pdev->jlp.version = version;
        JOYLINK_LOGI("set firmware version as %d\n", version);
        joylink_dev_set_attr_jlp(&(_g_pdev->jlp));
    }
}

char *joylink_json_get_value_by_id(char *p_cJsonStr, int iStrLen, const char *p_cName, int *p_iValueLen)
{
	JOYLINK_PARAM_CHECK((p_cJsonStr == NULL) || (p_cName == NULL));

	int i;
	char *pTarget = NULL;
	char *str1 = malloc(iStrLen+1);
	memset(str1, 0, iStrLen+1);
	memcpy(str1, p_cJsonStr, iStrLen);
	
	cJSON *pData = cJSON_Parse(str1);
	if (pData != NULL) {
		cJSON *pStm = cJSON_GetObjectItem(pData, "streams");

		if (NULL != pStm) {
			int stmNum = cJSON_GetArraySize(pStm);

			for (i = 0; i < stmNum; i++) {
				cJSON *pItem = cJSON_GetArrayItem(pStm, i);

				if (pItem != NULL) {
					cJSON *pTar = cJSON_GetObjectItem(pItem, "stream_id");

					if(pTar != NULL) {

					    if(0 == strcmp(p_cName, pTar->valuestring)) {
							cJSON *pTar = cJSON_GetObjectItem(pItem, "current_value");

							if(pTar != NULL) {
								pTarget = pTar->valuestring;
							}
						}
					}
				}
			}
		}
	}
	
	if(str1) {
		free(str1);
	}

	cJSON_Delete(pData);

	if (pTarget != NULL) {
		*p_iValueLen = strlen(pTarget);
	} else {
		*p_iValueLen = 0;
	}

	return pTarget;
}
