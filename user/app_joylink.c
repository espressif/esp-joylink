/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2017 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
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
#include "esp_joylink.h"
#include "esp_joylink_log.h"
#include "button/button.h"
#include "status_led/led.h"

static const char *TAG = "esp_joylink";

#define JOYLINK_WIFI_CFG_BUTTON_NUM       (13) /*!< set button pin, restart to wifi config */
#define JOYLINK_FACTORY_RESET_BUTTON_NUM  (14) /*!< set button pin, clear joylink config info */
#define LED_IO_NUM                        (15) /*!< set led gpio, led status */

#define JSON_VALUE_STR_LEN  (32) /*!< json value string length */

/*
 * light demo parameters:
 * power       |  int     |  0/1
 * brightness  |  int     |  0 ~ 100
 * colortemp   |  int     |  3000 ~ 6500
 * mode        |  int     |  0/1/2/3/4
 * color       |  string  |  string of color number
*/

typedef struct {
    char power;
    char light_value;
    int  temp_value;
    char work_mode;
    char color[10];
} virtual_device_t;

static led_handle_t led_0;
static virtual_device_t virtual_device;
static xSemaphoreHandle xSemWriteInfo = NULL;
static xSemaphoreHandle xSemReadInfo = NULL;

static char *device_attr[5] = { "power", "brightness", "colortemp", "mode", "color"};

const char *joylink_json_upload_str = "{\"code\":%d,\"streams\":[\
{\"current_value\": \"%d\",\"stream_id\": \"power\"},\
{\"current_value\": \"%d\",\"stream_id\": \"brightness\"},\
{\"current_value\": \"%d\",\"stream_id\": \"colortemp\"},\
{\"current_value\": \"%d\",\"stream_id\": \"mode\"},\
{\"current_value\": \"%s\",\"stream_id\": \"color\"}]}";

static uint8_t is_server;

void user_show_rst_info(void)
{
    struct rst_info *rtc_info = system_get_rst_info();
    JOYLINK_LOGI("reset reason: %x", rtc_info->reason);

    if ((rtc_info->reason == REASON_WDT_RST)
            || (rtc_info->reason == REASON_EXCEPTION_RST)
            || (rtc_info->reason == REASON_SOFT_WDT_RST)) {

        if (rtc_info->reason == REASON_EXCEPTION_RST) {
            JOYLINK_LOGI("Fatal exception (%d):", rtc_info->exccause);
        }

        JOYLINK_LOGI("dbg: epc1=0x%08x, epc2=0x%08x, epc3=0x%08x, excvaddr=0x%08x, depc=0x%08x",
                   rtc_info->epc1, rtc_info->epc2, rtc_info->epc3, rtc_info->excvaddr, rtc_info->depc);
    }
}

void get_free_heap_task()
{
    while (1) {
        JOYLINK_LOGD("free RAM size: %d", system_get_free_heap_size());
        vTaskDelay(2000 / portTICK_RATE_MS);
    }
}

int cloud_read(virtual_device_t* virtual_dev, int ms_wait)
{
    JOYLINK_PARAM_CHECK(!virtual_dev);

    int i, valueLen = 0;
    char data_buff[JSON_VALUE_STR_LEN];
    char *down_cmd = (char *)malloc(JOYLINK_DATA_LEN);

    if (esp_joylink_read(down_cmd, JOYLINK_DATA_LEN, ms_wait) < 0) {
        JOYLINK_LOGE("esp_joylink_read is error");
        return E_RET_ERROR;
    }

    for (i = 0; i < 5; i++) {
        char *valueStr = joylink_json_get_value_by_id(down_cmd, strlen(down_cmd), device_attr[i], &valueLen);

        if (valueStr && valueLen > 0) {
            memset(data_buff, 0, JSON_VALUE_STR_LEN);
            memcpy(data_buff, valueStr, valueLen);
            JOYLINK_LOGD("valueStr:%s, attrLen:%d", data_buff, valueLen);

            switch (i) {
                case 0:
                    virtual_dev->power = atoi(data_buff);
                    break;

                case 1:
                    virtual_dev->light_value = atoi(data_buff);
                    break;

                case 2:
                    virtual_dev->temp_value = atoi(data_buff);
                    break;

                case 3:
                    virtual_dev->work_mode = atoi(data_buff);
                    break;

                case 4:
                    strcpy(virtual_dev->color, data_buff);
                    break;

                default:
                    break;
            }
        }
    }

    if (NULL != down_cmd) {
        free(down_cmd);
    }

    return E_RET_OK;
}

int cloud_write(virtual_device_t virtual_device, int ms_wait)
{
    char *up_cmd = (char *)malloc(JOYLINK_DATA_LEN);
    memset(up_cmd, 0, JOYLINK_DATA_LEN);
    sprintf(up_cmd, joylink_json_upload_str, 0, virtual_device.power, virtual_device.light_value,
            virtual_device.temp_value, virtual_device.work_mode, virtual_device.color);
    int ret = esp_joylink_write(up_cmd, strlen(up_cmd)+1, ms_wait);
    free(up_cmd);
    return ret;
}

void read_task_test(void *arg)
{
    int cnt = 0;

    for (;;) {
        xSemaphoreTake(xSemReadInfo, portMAX_DELAY);
        cloud_read(&virtual_device, portMAX_DELAY);

        if (cnt++ % 2) {
            led_state_write(led_0, LED_NORMAL_ON);
        } else {
            led_state_write(led_0, LED_NORMAL_OFF);
        }

        JOYLINK_LOGD("highwater %d", uxTaskGetStackHighWaterMark(NULL));
    }
}

void write_task_test(void *arg)
{
    for (;;) {
        xSemaphoreTake(xSemWriteInfo, portMAX_DELAY);
        int ret = cloud_write(virtual_device, 500 / portTICK_RATE_MS);

        if (ret == E_RET_ERROR) {
            JOYLINK_LOGE("esp_write is err");
        }

        vTaskDelay(500 / portTICK_RATE_MS);
        JOYLINK_LOGD("highwater %d", uxTaskGetStackHighWaterMark(NULL));
    }
}

int joylink_event_handler(joylink_action_t action)
{
    joylink_event_param_t *param = NULL;

    if (action.arg) {
        param = ((joylink_event_param_t*)action.arg);
    }

    switch (action.event) {
        case JOYLINK_EVENT_WIFI_START_SMARTCONFIG:
            JOYLINK_LOGD("JOYLINK_EVENT_WIFI_START_SMARTCONFIG");
            led_state_write(led_0, LED_SLOW_BLINK);
            break;

        case JOYLINK_EVENT_WIFI_GOT_IP:
            JOYLINK_LOGD("JOYLINK_EVENT_WIFI_GOT_IP");
            led_state_write(led_0, LED_QUICK_BLINK);
            JOYLINK_LOGD("conn reason:%d, ssid:%s, password:%s", param->conn.rsn,
                    param->conn.conf.ssid, param->conn.conf.password);
            break;

        case JOYLINK_EVENT_CLOUD_CONNECTED:
            JOYLINK_LOGD("JOYLINK_EVENT_CLOUD_CONNECTED");
            led_state_write(led_0, LED_NORMAL_ON);
            break;

        case JOYLINK_EVENT_CLOUD_DISCONNECTED:
            log_warn("JOYLINK_EVENT_CLOUD_DISCONNECTED");
            break;

        case JOYLINK_EVENT_GET_DEVICE_DATA:
            JOYLINK_LOGD("JOYLINK_EVENT_GET_DEVICE_DATA");
            JOYLINK_LOGD("must to post cmd, max len:%d, %s, %s", \
                      param->get.max_len, param->get.is_json ? "json" : "script", \
                      param->get.is_server_cmd ? "server" : "lan");
            is_server = param->get.is_server_cmd;
            xSemaphoreGive(xSemWriteInfo);
            break;

        case JOYLINK_EVENT_SET_DEVICE_DATA:
            JOYLINK_LOGD("JOYLINK_EVENT_SET_DEVICE_DATA");
            JOYLINK_LOGD("must to read cmd, cmd len:%d, %s, %s", \
                      param->set.cmd_len, param->set.is_json ? "json" : "script", \
                      param->set.is_server_cmd ? "server" : "lan");
            is_server = param->get.is_server_cmd;
            xSemaphoreGive(xSemReadInfo);
            break;

        case JOYLINK_EVENT_OTA_START:
            JOYLINK_LOGD("JOYLINK_EVENT_OTA_START");
            JOYLINK_LOGD("start to ota, ver: %d, url: %s", param->ota.version, param->ota.url);
            break;

        default:
            break;
    }

    return E_RET_OK;
}

static void initialise_key(void)
{
    button_handle_t btn_handle1 = button_dev_init(JOYLINK_WIFI_CFG_BUTTON_NUM, 1, BUTTON_ACTIVE_LOW);
    button_dev_add_tap_cb(BUTTON_TAP_CB, joylink_reset_to_smartconfig, "PUSH", 50 / portTICK_RATE_MS, btn_handle1);
    button_dev_add_press_cb(0, joylink_reset_to_softap_innet, "long press", 3000 / portTICK_RATE_MS, btn_handle1);

    button_handle_t btn_handle2 = button_dev_init(JOYLINK_FACTORY_RESET_BUTTON_NUM, 0, BUTTON_ACTIVE_LOW);
    button_dev_add_tap_cb(BUTTON_PUSH_CB, joylink_restore_factory, "PUSH", 50 / portTICK_RATE_MS, btn_handle2);
}

static void initialise_led(void)
{
    led_0 = led_create(LED_IO_NUM, LED_DARK_LOW);
    led_state_write(led_0, LED_NORMAL_OFF);
}

int joylink_start(void)
{
    joylink_info_t *product_info = (joylink_info_t*)malloc(sizeof(joylink_info_t));
    
    if (!product_info) {
        JOYLINK_LOGE("malloc fail\n");
    }
    
    memset(product_info,0x0,sizeof(joylink_info_t));
    
    product_info->innet_aes_key = JOYLINK_AES_KEY;
    product_info->device_info.local_port = JOYLINK_LOCAL_PORT;
    product_info->device_info.jlp.version = JOYLINK_VERSION;
    product_info->device_info.jlp.devtype = JOYLINK_DEVTYPE;
    strncpy(product_info->device_info.jlp.joylink_server, JOYLINK_SERVER,sizeof(product_info->device_info.jlp.joylink_server));
    product_info->device_info.jlp.server_port = JOYLINK_SERVER_PORT;
    strncpy(product_info->device_info.jlp.firmwareVersion, JOYLINK_FW_VERSION, sizeof(product_info->device_info.jlp.firmwareVersion));
    strncpy(product_info->device_info.jlp.modelCode, JOYLINK_MODEL_CODE, sizeof(product_info->device_info.jlp.modelCode));
    strncpy(product_info->device_info.jlp.uuid, JOYLINK_UUID, sizeof(product_info->device_info.jlp.uuid));
    product_info->device_info.jlp.lancon = JOYLINK_LAN_CTRL;
    product_info->device_info.jlp.cmd_tran_type = JOYLINK_CMD_TYPE;
    strncpy(product_info->device_info.idt.cloud_pub_key, CLOUD_PUB_KEY, sizeof(product_info->device_info.idt.cloud_pub_key));
        
    uint8 macaddr[6];
    char mac_str[18] = {0};
    wifi_get_macaddr(0, macaddr);
    snprintf(product_info->device_info.jlp.mac, sizeof(product_info->device_info.jlp.mac), MACSTR, MAC2STR(macaddr));

    JOYLINK_LOGI("*********************************");
    JOYLINK_LOGI("*         JOYLINK INFO          *");
    JOYLINK_LOGI("*********************************");
    JOYLINK_LOGI("UUID     : %s", product_info->device_info.jlp.uuid);
    JOYLINK_LOGI("mac      : %s", product_info->device_info.jlp.mac);
    JOYLINK_LOGI("type     : %d", product_info->device_info.jlp.devtype);
    JOYLINK_LOGI("version  : %d", product_info->device_info.jlp.version);
    JOYLINK_LOGI("fw_ver   : %s", product_info->device_info.jlp.firmwareVersion);
    JOYLINK_LOGI("model    : %s", product_info->device_info.jlp.modelCode);
    JOYLINK_LOGI("tran type: %d", product_info->device_info.jlp.cmd_tran_type);
    JOYLINK_LOGI("innet key: %s", product_info->innet_aes_key);
    JOYLINK_LOGI("public key: %s", product_info->device_info.idt.cloud_pub_key);
    JOYLINK_LOGI("sys run  : %s", system_upgrade_userbin_check() ? "user2" : "user1");
    JOYLINK_LOGI("*********************************");

    esp_joylink_event_init(joylink_event_handler);
    initialise_key();
    initialise_led();
    esp_info_init();
    esp_joylink_init(product_info);
    esp_joylink_set_version(JOYLINK_VERSION);

    if (xSemWriteInfo == NULL) {
        xSemWriteInfo = (xSemaphoreHandle)xSemaphoreCreateBinary();
    }

    if (xSemReadInfo == NULL) {
        xSemReadInfo = (xSemaphoreHandle)xSemaphoreCreateBinary();
    }

    xTaskCreate(get_free_heap_task, "get_free_heap_task", 128, NULL, 5, NULL);
    xTaskCreate(read_task_test, "read_task_test", 256, NULL, 5, NULL);
    xTaskCreate(write_task_test, "write_task_test", 256, NULL, 4, NULL);

    if (!product_info) {
        free(product_info);
    }
    
    return E_RET_OK;
}

extern void joylink_init_aes_table(void);

void user_demo(void)
{
    JOYLINK_LOGI("*********************************************");
    JOYLINK_LOGI("* user.bin compiled @ %s %s *", __DATE__, __TIME__);
    JOYLINK_LOGI("* free RAM size %d ", system_get_free_heap_size());
    user_show_rst_info();
    JOYLINK_LOGI("*********************************************");

    joylink_init_aes_table();
    joylink_start();  /* start joylink */
}
