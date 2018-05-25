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

#ifndef __ESP_JOYLINK_H__
#define __ESP_JOYLINK_H__

#include "joylink.h"
#include "joylink_config.h"
#include "esp_common.h"

#ifdef  __cplusplus
extern "C" {
#endif

typedef int32_t joylink_err_t;

#ifndef JOYLINK_TRUE
#define JOYLINK_TRUE  (1)
#endif
#ifndef JOYLINK_FALSE
#define JOYLINK_FALSE (0)
#endif
#ifndef JOYLINK_OK
#define JOYLINK_OK    (0)
#endif
#ifndef JOYLINK_ERR
#define JOYLINK_ERR   (-1)
#endif

#ifndef CONFIG_JOYLINK_TASK_PRIOTY
#define CONFIG_JOYLINK_TASK_PRIOTY    (2)
#endif

#ifndef CONFIG_DOWN_CMD_QUEUE_NUM
#define CONFIG_DOWN_CMD_QUEUE_NUM     (3)
#endif

#ifndef CONFIG_UP_CMD_QUEUE_NUM
#define CONFIG_UP_CMD_QUEUE_NUM       (3)
#endif

#ifndef CONFIG_EVENT_QUEUE_NUM
#define CONFIG_EVENT_QUEUE_NUM        (5)
#endif

#ifndef CONFIG_JOYLINK_DATA_LEN
#define CONFIG_JOYLINK_DATA_LEN       (1024)
#endif

#ifndef CONFIG_EVENT_HANDLER_CB_STACK
#define CONFIG_EVENT_HANDLER_CB_STACK (256-64)
#endif

#ifndef CONFIG_JOYLINK_MAIN_TASK_STACK
#define CONFIG_JOYLINK_MAIN_TASK_STACK (512 + 128)
#endif

#ifndef CONFIG_WIFI_WAIT_TIME
#define CONFIG_WIFI_WAIT_TIME         (1000)
#endif

#define JOYLINK_DOWN_Q_NUM       CONFIG_DOWN_CMD_QUEUE_NUM
#define JOYLINK_UP_Q_NUM         CONFIG_UP_CMD_QUEUE_NUM
#define JOYLINK_EVENT_QUEUE_NUM  CONFIG_EVENT_QUEUE_NUM

#define JOYLINK_TASK_PRIOTY      CONFIG_JOYLINK_TASK_PRIOTY
#define JOYLINK_DATA_LEN         CONFIG_JOYLINK_DATA_LEN
#define JOYLINK_EVENT_TASK_STACK CONFIG_EVENT_HANDLER_CB_STACK
#define JOYLINK_MAIN_TASK_STACK  CONFIG_JOYLINK_MAIN_TASK_STACK
#define JOYLINK_WAIT_DEV_STATUS_TIMEOUT CONFIG_WIFI_WAIT_TIME

#ifdef CONFIG_ALINK_PASSTHROUGH
#define JOYLINK_PASSTHROUGH
#endif

typedef struct {
    JLDevice_t device_info;
    const char *innet_aes_key;
} joylink_info_t;

typedef enum {
    CONFIG_MODE_NONE = 0,       /*!< None */
    CONFIG_MODE_SMARTCONFIG,    /*!< smartconfig mode */
    CONFIG_MODE_SOFTAP          /*!< softap config mode */
} wifi_config_mode_t;

typedef enum {
    JOYLINK_EVENT_NONE = 0,                 /*!< None */
    JOYLINK_EVENT_WIFI_START_SMARTCONFIG,   /*!< ESP8266 start smartconfig */
    JOYLINK_EVENT_WIFI_START_SOFTAP_CONFIG, /*!< ESP8266 start softap config */
    JOYLINK_EVENT_WIFI_GOT_IP,              /*!< ESP8266 station got IP from connected AP */
    JOYLINK_EVENT_WIFI_DISCONNECTED,        /*!< ESP8266 station disconnet with AP */
    JOYLINK_EVENT_CLOUD_CONNECTED,          /*!< ESP8266 connected joylink cloud */
    JOYLINK_EVENT_CLOUD_DISCONNECTED,       /*!< ESP8266 disconnected with joylink cloud */
    JOYLINK_EVENT_GET_DEVICE_DATA,          /*!< ESP8266 get device status */
    JOYLINK_EVENT_SET_DEVICE_DATA,          /*!< ESP8266 set device status */
    JOYLINK_EVENT_OTA_START,                /*!< Start firmware upgrade */
} joylink_event_t;

typedef enum {
    JOYLINK_GOT_IP_WIFI_CONFIG = 10,        /*!< wifi config */
    JOYLINK_GOT_IP_AUTO_CONN   = 11,        /*!< auto connect */
} joylink_reason_t;

typedef union {
    /**
	 * @brief JOYLINK_EVENT_WIFI_GOT_IP
	 */
    struct jl_evt_got_ip {
        struct station_config conf;     /*!< wifi config */
        joylink_reason_t rsn;           /*!< wifi connect reason */
    } conn;
    
    /**
	 * @brief JOYLINK_EVENT_SET_DEVICE_DATA
	 */
    struct jl_evt_cmd_down2dev {
        uint8_t is_server_cmd;          /*!< down cmd from server or lan */
        uint8_t is_json;                /*!< down cmd data is json or script(passthrough) */
        uint16_t cmd_len;               /*!< down cmd length */
    } set;

    /**
	 * @brief JOYLINK_EVENT_GET_DEVICE_DATA
	 */
    struct jl_evt_cmd_up2server {		
        uint8_t is_server_cmd;          /*!< up cmd is to server or lan */
        uint8_t is_json;                /*!< up cmd is is json or script(passthrough) */
        uint16_t max_len;				/*!< max length of up cmd */
    } get;

    /**
	 * @brief JOYLINK_EVENT_OTA_START
	 */
    struct jl_evt_cmd_ota {		
        int version;                    /*!< firmware version */
        char url[100];                  /*!< http url of firmware */
    } ota;

} joylink_event_param_t;

typedef struct {
    joylink_event_t event;
    void *arg;
} joylink_action_t;

/**
 * @brief  Application specified event callback function
 *
 * @param  action parameters related event
 *
 * @note The memory space for the event callback function defaults to 4kBety
 *
 * @return
 *     - JOYLINK_OK : Succeed
 *     - others : fail
 */
typedef joylink_err_t (*joylink_event_cb_t)(joylink_action_t action);

/**
  * @brief register joylink SDK event callback.
  *
  * @param cb  joylink_event_cb_t.
  *
  * @return
  *    - JOYLINK_OK:  ok
  *    - JOYLINK_ERR:  err
  */
joylink_err_t esp_joylink_event_init(joylink_event_cb_t cb);

/**
  * @brief send event.
  *
  * @param event generated events.
  * @param para  parameters related event.
  *
  * @return
  *    - JOYLINK_OK:  ok
  *    - JOYLINK_ERR:  err
  */
joylink_err_t joylink_event_send(joylink_event_t event, joylink_event_param_t *para);

/**
  * @brief device unbundle, clear joylink config info.
  *
  */
void joylink_restore_factory(void *para);

/**
  * @brief reset to innet use smartconfig mode but not unbundle.
  *
  */
void joylink_reset_to_smartconfig(void *para);

/**
  * @brief reset to innet use softap mode but not unbundle.
  *
  */
void joylink_reset_to_softap_innet(void *para);

/**
  * @brief get config mode.
  *
  */
int joylink_get_config_mode(void);

/**
  * @brief get cmd from server or lan.
  *
  * @param down_cmd  [out]cmd download from server or lan.
  * @param len  [in]the lenth of down_cmd, should less than JOYLINK_DATA_LEN.
  * @param milli_seconds  [in]timeout.
  * 
  * @return
  *    - >0 : real cmd length
  *    - <= : read err
  */
ssize_t esp_joylink_read(void *down_cmd, size_t size, int mmilli_seconds);

/**
  * @brief upload device info to joylink SDK.
  *
  * @param up_cmd  [in]cmd upload to server or lan.
  * @param len  [in]the lenth of up_cmd, should include '\0',should less than JOYLINK_DATA_LEN.
  * @param milli_seconds  [in]timeout.
  * 
  * @return
  *    - >0 : real cmd length
  *    - <= : read err
  */
ssize_t esp_joylink_write(void *up_cmd, size_t len, int milli_seconds);

/**
  * @brief joylink SDK init.
  *
  * @param arg  [in] get info from https://devsmart.jd.com.
  *
  * @return
  *    - JOYLINK_OK: set ok
  *    - JOYLINK_ERR: set err
  */
joylink_err_t esp_joylink_init(joylink_info_t * arg);

/**
  * @brief get the system jlp info, e.g. "feedid".
  *
  * @attention 1. This API can only be called after activate the device.
  *
  * @param jlp  memory must be ready.
  *
  * @return
  *    - JOYLINK_OK: get ok
  *    - JOYLINK_ERR: get err
  */
joylink_err_t joylink_get_jlp_info(joylink_info_t *jlp);

/**
  * @brief set the fimware version to the flash.
  *
  * @param version fimware version.
  *
  * @return
  *    - void
  */
void esp_joylink_set_version(uint8_t version);

/**
  * @brief Get the value by a specified key from a JOYLINK json string
  *
  * @param p_cJsonStr  [in] the JSON string
  * @param iStrLen  [in] the JSON string length
  * @param p_cName  [in] the specified key string
  * @param p_iValueLen  [out] the value length
  *
  * @return
  *     - A pointer to the value
  */
/*
 * e.g. (JOYLINK json, StrLen, "power", ValueLen)
 *{
 *   "cmd": 5,
 *   "data": {
 *       "streams": [
 *           {
 *               "current_value": "1",
 *               "stream_id": "power"
 *           }
 *       ]
 *   }
 *}
*/
char *joylink_json_get_value_by_id(char *p_cJsonStr, int iStrLen, const char *p_cName, int *p_iValueLen);

/*
 **** example : the format of light device ***********
 **** from server or lan *****************************
 *{
 *   "cmd": 5,
 *   "data": {
 *       "streams": [
 *           {
 *               "current_value": "1",
 *               "stream_id": "power"
 *           }
 *       ],
 *       "snapshot": [
 *           {
 *               "current_value": "",
 *               "stream_id": "brightness"
 *           },
 *           {
 *               "current_value": "",
 *               "stream_id": "colortemp"
 *           },
 *           {
 *               "current_value": "",
 *               "stream_id": "color"
 *           },
 *           {
 *               "current_value": "0",
 *               "stream_id": "power"
 *           },
 *           {
 *               "current_value": "",
 *               "stream_id": "mode"
 *           }
 *       ]
 *   }
 *}
 **** send server or lan *****************************
 *{
 *   "code":0,
 *   "data":[
 *           {
 *               "current_value": "",
 *               "stream_id": "brightness"
 *           },
 *           {
 *               "current_value": "",
 *               "stream_id": "colortemp"
 *           },
 *           {
 *               "current_value": "",
 *               "stream_id": "color"
 *           },
 *           {
 *               "current_value": "0",
 *               "stream_id": "power"
 *           },
 *           {
 *               "current_value": "",
 *               "stream_id": "mode"
 *           }
 *       ]
 *}
*/

#ifdef __cplusplus
}
#endif

#endif
