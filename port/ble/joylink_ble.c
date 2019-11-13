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
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"

#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_log.h"

#ifndef CONFIG_IDF_TARGET_ESP8266
#include "esp_bt.h"
#include "esp_bt_device.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#endif

#include "joylink.h"
#include "joylink_sdk.h"
#include "joylink_dev.h"

#include "esp_joylink.h"

#define GATTS_TAG "joylink"

extern bool jd_innet_timer_task_flag;
extern xTaskHandle jd_innet_timer_task_handle;
///Declare the static function 
#ifndef CONFIG_IDF_TARGET_ESP8266
static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
#endif

static xQueueHandle cmd_evt_queue = NULL;
static xQueueHandle cmd_resend_queue = NULL;

//menufactory adv data len 
#define JOY_ADV_MENUFAC_LEN                  14

//packet offset
#define JOY_OFFSET_FULLPACKET_SEQ            0
#define JOY_OFFSET_FULLPACKET_OPERATE        (JOY_OFFSET_FULLPACKET_SEQ + 1)
#define JOY_OFFSET_FULLPACKET_LENGTH         (JOY_OFFSET_FULLPACKET_OPERATE + 1)
#define JOY_OFFSET_FULLPACKET_CONTENT        (JOY_OFFSET_FULLPACKET_LENGTH + 2)
//leghth
#define JOY_SIZE_FULLPACKET_TAG               2
#define JOY_SIZE_FULLPACKET_TAGLEN            1

//oprate
#define JOY_OPERATE_ID_PRD                    0x01    //phone read device
#define JOY_OPERATE_ID_DRP                    0x11    //data return data to phone
#define JOY_OPERATE_ID_PWD_WITHNORES          0x02    //phone write device without response
#define JOY_OPERATE_ID_PWD_WITHRES            0x03    //phone write device with response
#define JOY_OPERATE_ID_DRWRES                 0x13    //device return response of write cmd

#define JOY_OPERATE_ID_DTP_WITHNORES          0x16    //device send data to phone without response
#define JOY_OPERATE_ID_DTP_WITHRES            0x17    //device send data to phone with response
#define JOY_OPERATE_ID_PTD_RES                0x07    //phone return response to device

//property
#define JOY_PROPERTY_TAG_PUID                0xFFFF
#define JOY_PROPERTY_TAG_GUID                0xFFFC
#define JOY_PROPERTY_TAG_LOCALKEY            0xFFFB
#define JOY_PROPERTY_TAG_DEV_SNAPSHOT        0xFEFF
#define JOY_PROPERTY_TAG_PUBKEY_APP          0xFEF9
#define JOY_PROPERTY_TAG_PUBKEY_DEV          0xFEF8
#define JOY_PROPERTY_TAG_SSID                0xFEF7
#define JOY_PROPERTY_TAG_PASSWORD            0xFEF6
#define JOY_PROPERTY_TAG_SECLEVEL            0xFEF5
#define JOY_PROPERTY_TAG_BRAND               0xFEF4
#define JOY_PROPERTY_TAG_CID                 0xFEF3
#define JOY_PROPERTY_TAG_BLE_DEV_CTL         0xFEF2
#define JOY_PROPERTY_TAG_WIFI_STATUS         0xFEF1

//task command
#define JOY_TASK_CMDID_REVDATAFINISH            0xFF01            
#define JOY_TASK_CMDID_TEST_INDICATE            0xFF02
//resend task command
#define JOY_RESENDTASK_RESENDWIFISTATUS         0xEF01    



//BLE service adn charicteristic UUID
#define GATTS_SERVICE_UUID_JD_PROFILE           0xFE70
#define GATTS_CHAR_UUID_WRITE_JD_PROFILE        0xFE71
#define GATTS_CHAR_UUID_INDICATIE_JD_PROFILE    0xFE72
#define GATTS_CHAR_UUID_READ_JD_PROFILE         0xFE73

//response
#define JOY_RESPONSE_OK                           0x00
#define JOY_RESPONSE_ERROR1                       0x01
#define JOY_WIFI_RECONNECT_NUM                    5           //try to connect ap 32 times

//used for device indicate data to phone
#define JOY_SDK_BUFF_LEN                        512
#define JOY_SDK_SENDBUFF_LEN                    512

#define SIZE_SSID                               32
#define SIZE_PWD                                32


uint8_t joy_sdk_buff[JOY_SDK_BUFF_LEN]            = {0};
uint8_t joy_sdk_sendbuf[JOY_SDK_SENDBUFF_LEN]     = {0};


//used for phone write data to device(device receive data)
#define JOY_SDK_RECV_BUFF_LEN        1024
uint8_t joy_sdk_recv_buff[JOY_SDK_RECV_BUFF_LEN] = {0};



#define GATTS_DESCR_UUID_TEST_A     0x3333
#define GATTS_NUM_HANDLE_TEST_A     8

#define GATTS_SERVICE_UUID_TEST_B   0x00EE
#define GATTS_CHAR_UUID_TEST_B      0xEE01
#define GATTS_DESCR_UUID_TEST_B     0x2222
#define GATTS_NUM_HANDLE_TEST_B     4

uint8_t joy_sdk_ssid_rev[SIZE_SSID+1] = {0};
uint8_t joy_sdk_pwd_rev[SIZE_PWD+1]  = {0};

uint8_t seq_num = 0;

#ifndef CONFIG_IDF_TARGET_ESP8266
extern uint8_t compressed_dev_pub_key[21]; 
extern uint8_t compressed_app_pub_key[21];
#else
uint8_t compressed_dev_pub_key[21]; 
uint8_t compressed_app_pub_key[21];
#endif 

extern JLDevice_t  *_g_pdev;
//#define TEST_DEVICE_NAME            "DEMO"

#define JOY_SECRET_LEVEL            0x02 // _g_pdev->joylink_ble_secret_level//0x02
//#define TEST_MANUFACTURER_DATA_LEN  17

static uint8_t joy_uuid128_service[16]      = {0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x70, 0xFE, 0x00, 0x00};
static uint8_t joy_uuid128_chra_write[16]   = {0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x71, 0xFE, 0x00, 0x00};
static uint8_t joy_uuid128_chra_indicate[16]  = {0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x72, 0xFE, 0x00, 0x00};
static uint8_t joy_uuid128_chra_read[16] = {0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x73, 0xFE, 0x00, 0x00};

static uint8_t joy_adv_manufacture[JOY_ADV_MENUFAC_LEN]={0};

#define OFFSET_RAW_ADV_MENUFAC        7

static uint8_t raw_adv_data[] = {
        0x02,0x01,0x06,                                //le support 3
        0x03,0x03,0x70,0xFE,                        //uuid        4
        0x0f,0xFF,0,0,0,0,0,0,0,0,0,0,0,0,0,0,        //menufac    16
        0x04,0x09,'l','x','m',                        //name
        //0x02,0x0A,0xEB
};

static uint8_t raw_scan_rsp_data[] = {
        0x02,0x01,0x06,                                //le support 3
        0x03,0x03,0x70,0xFE,                        //uuid        4
        0x0f,0xFF,0,0,0,0,0,0,0,0,0,0,0,0,0,0,        //menufac    16
        0x04,0x09,'l','x','m',                        //name
        //0x02,0x0A,0xEB                                //tx power
};

#ifndef CONFIG_IDF_TARGET_ESP8266
static esp_ble_adv_params_t test_adv_params = {
        .adv_int_min        = 0x20,
        .adv_int_max        = 0x40,
        .adv_type           = ADV_TYPE_IND,
        .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
        //.peer_addr            =
        //.peer_addr_type       =
        .channel_map        = ADV_CHNL_ALL,
        .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};
#endif

typedef enum{
    wifi_disconnect = 0,
    wifi_connecting = 1,
    wifi_connected  = 2,
    wifi_error_noap = 3,
    wifi_error_others = 4
}type_wifi_status;

typedef enum{
    broadcast_enble = 0,
    broadcast_disable = 1
}type_ble_broadcast_flag;


typedef enum{
    ble_disconnect     = 0,
    ble_connect        = 1
}type_ble_connect_status;

struct gatts_ctl_status{
    uint8_t is_indicatedata_now;
    type_ble_connect_status ble_connect_status;
    type_ble_broadcast_flag ble_broadcast_flag;
    type_wifi_status wifi_status;
    bool test_mode;
};

#ifndef CONFIG_IDF_TARGET_ESP8266
struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;

    uint16_t charindicate_handle;
    esp_bt_uuid_t charindicate_uuid;

    uint16_t charread_handle;
    esp_bt_uuid_t charread_uuid;

    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst gl_profile_tab = {
    .gatts_cb = gatts_profile_a_event_handler,
    .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
};
#endif

struct gatts_ctl_status joy_gatts_ctl_status;
    
int jh_send(uint8_t* frame);
void joylink_delay_10_min_timer_for_10_min_stop(void);
extern bool joylink_net_configuaring;


static void gatts_start_wifi_connect(void);
void joy_set_wifi_status(type_wifi_status status);
static type_wifi_status joy_get_wifi_status(void);
static void joy_set_ble_broadcast_flag(type_ble_broadcast_flag flag);
static type_ble_broadcast_flag joy_get_ble_broadcast_flag(void);
static void joy_set_ble_connect_status(type_ble_connect_status status);
static type_ble_connect_status joy_get_ble_connect_status(void);
static int joy_tell_wifi_status(type_wifi_status status);

int jh_load_mac(uint8_t * zone);
int jh_load_pid(uint8_t *puid);

static void joy_change_adv_wifistatus(type_wifi_status st);
void joylink_gatts_adv_data_enable(void);

static xTimerHandle joylink_delay_timer = NULL;

void joylink_delay_3_min_timer_for_adv_cb( TimerHandle_t xTimer )
{
    joy_set_ble_broadcast_flag(broadcast_disable);
#ifndef CONFIG_IDF_TARGET_ESP8266
    printf("/*----------------BLE-ADV-STOP(3min)----------------*/\n");
    esp_ble_gap_stop_advertising();
#endif
    if(xTimerDelete(joylink_delay_timer,0) == pdPASS) {
        joylink_delay_timer = NULL;
    }
}

void joylink_delay_3_min_timer_for_adv(void)
{
    joy_set_wifi_status(wifi_connecting);
    if (joy_get_ble_connect_status() == ble_connect) {
        joy_tell_wifi_status(wifi_connected);
    } else {
        joy_change_adv_wifistatus(joy_get_wifi_status());
        joylink_gatts_adv_data_enable();
    }

    if (joylink_delay_timer == NULL) {
        joylink_delay_timer = xTimerCreate("ble_delay",(3*60*1000)/(1000 /xPortGetTickRateHz()),pdFALSE,NULL,joylink_delay_3_min_timer_for_adv_cb);
        xTimerStart(joylink_delay_timer,100 );
    } else {
        xTimerReset(joylink_delay_timer,100);
    }
}

static void gatts_adv_data(void)
{
    memcpy(raw_adv_data+OFFSET_RAW_ADV_MENUFAC+2,joy_adv_manufacture,JOY_ADV_MENUFAC_LEN);
    memcpy(raw_scan_rsp_data+OFFSET_RAW_ADV_MENUFAC+2,joy_adv_manufacture,JOY_ADV_MENUFAC_LEN);

#ifndef CONFIG_IDF_TARGET_ESP8266
    esp_ble_gap_config_adv_data_raw(raw_adv_data, sizeof(raw_adv_data));
    esp_ble_gap_start_advertising(&test_adv_params);
#endif
}

void joylink_gatts_adv_data_enable(void)
{
    gatts_adv_data();
}

static void joy_init_adv_menufac(void)
{
    uint8_t offset = 0;
    uint8_t addr_bt[6];
    uint8_t puid[6];
    memset(joy_adv_manufacture,0,sizeof(joy_adv_manufacture));
    jh_load_pid(puid);
    memcpy(joy_adv_manufacture,puid,6);
    offset += 6;

    jh_load_mac(addr_bt);
    memcpy(joy_adv_manufacture+offset,addr_bt,6);
    offset += 6;

    *(joy_adv_manufacture+offset) = 0x00 | JOY_SECRET_LEVEL;        //versec
    offset += 1;

    *(joy_adv_manufacture+offset) = 0x10;   //expandtag
    offset += 1;
}

static void joy_change_adv_wifistatus(type_wifi_status st)
{
    uint8_t tt;
    tt = *(joy_adv_manufacture+13);
    tt = (tt & 0xF0)|st;
    *(joy_adv_manufacture+13) = tt;
}

#ifndef CONFIG_IDF_TARGET_ESP8266
static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch (event) {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(GATTS_TAG, "REGISTER_APP_EVT, status %d, app_id %d\n", param->reg.status, param->reg.app_id);
        gl_profile_tab.service_id.is_primary = true;
        gl_profile_tab.service_id.id.inst_id = 0x00;

        gl_profile_tab.service_id.id.uuid.len = ESP_UUID_LEN_128;
        memcpy(gl_profile_tab.service_id.id.uuid.uuid.uuid128,joy_uuid128_service,ESP_UUID_LEN_128);
        
        //added by dream
        gl_profile_tab.gatts_if = gatts_if;

        esp_ble_gatts_create_service(gatts_if, &gl_profile_tab.service_id, GATTS_NUM_HANDLE_TEST_A);
        break;
    case ESP_GATTS_READ_EVT: {

        ESP_LOGI(GATTS_TAG, "GATT_READ_EVT, conn_id %d, trans_id %d, handle %d\n", param->read.conn_id, param->read.trans_id, param->read.handle);
        esp_gatt_rsp_t rsp;
        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
        rsp.attr_value.handle = param->read.handle;

        rsp.attr_value.len = 4;
        rsp.attr_value.value[0] = 0xde;
        rsp.attr_value.value[1] = 0xed;
        rsp.attr_value.value[2] = 0xbe;
        rsp.attr_value.value[3] = 0xef;

        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
                ESP_GATT_OK, &rsp);
        break;
    }
    case ESP_GATTS_WRITE_EVT: {
        ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, conn_id %d, trans_id %d, handle %d\n", param->write.conn_id, param->write.trans_id, param->write.handle);
        ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, value len %d, value :\n", param->write.len);

        if(RECEIVE_END == jl_receive(param->write.value)){
            ESP_LOGI(GATTS_TAG, "jl_receive end result get\n");

            uint16_t cmd = JOY_TASK_CMDID_REVDATAFINISH;
            xQueueSend(cmd_evt_queue,&cmd,10/portTICK_PERIOD_MS);
        }else{
            ESP_LOGI(GATTS_TAG, "jl_receive not finish\n");
        }

        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        break;
    }
    case ESP_GATTS_EXEC_WRITE_EVT:
    case ESP_GATTS_MTU_EVT:
    case ESP_GATTS_UNREG_EVT:
        break;
    case ESP_GATTS_CONF_EVT:
        joy_gatts_ctl_status.is_indicatedata_now = 0;
        jl_indication_confirm_cb();
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_CONF_EVT:receive confirm\n");

        if (joy_gatts_ctl_status.test_mode == true) {
            esp_joylink_set_config_network(ESP_JOYLINK_CONFIG_NETWORK_SMNT_BLE);
        }
        break;
    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(GATTS_TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d\n", param->create.status, param->create.service_handle);
        gl_profile_tab.service_handle = param->create.service_handle;

        gl_profile_tab.char_uuid.len = ESP_UUID_LEN_128;
        memcpy(gl_profile_tab.char_uuid.uuid.uuid128,joy_uuid128_chra_write,ESP_UUID_LEN_128);

        esp_ble_gatts_start_service(gl_profile_tab.service_handle);

        esp_ble_gatts_add_char(gl_profile_tab.service_handle, &gl_profile_tab.char_uuid,
                ESP_GATT_PERM_WRITE,
                ESP_GATT_CHAR_PROP_BIT_WRITE,NULL,NULL);

        break;
    case ESP_GATTS_ADD_INCL_SRVC_EVT:

        break;
    case ESP_GATTS_ADD_CHAR_EVT:
        ESP_LOGI(GATTS_TAG, "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d\n,char_uuid %2x\n",
                param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle,param->add_char.char_uuid.uuid.uuid16);

        if(0 == memcmp(param->add_char.char_uuid.uuid.uuid128,joy_uuid128_chra_write,ESP_UUID_LEN_128)){

            gl_profile_tab.char_handle = param->add_char.attr_handle;
            gl_profile_tab.charindicate_uuid.len = ESP_UUID_LEN_128;

            memcpy(gl_profile_tab.charindicate_uuid.uuid.uuid128,joy_uuid128_chra_indicate,ESP_UUID_LEN_128);
            esp_ble_gatts_add_char(gl_profile_tab.service_handle, &gl_profile_tab.charindicate_uuid,
                    ESP_GATT_PERM_READ,
                    ESP_GATT_PERM_READ | ESP_GATT_CHAR_PROP_BIT_INDICATE,NULL,NULL);
        }else if(0 == memcmp(param->add_char.char_uuid.uuid.uuid128,joy_uuid128_chra_indicate,ESP_UUID_LEN_128)){
            gl_profile_tab.charindicate_handle = param->add_char.attr_handle;

            gl_profile_tab.charread_uuid.len = ESP_UUID_LEN_128;
        }else if(0 == memcmp(param->add_char.char_uuid.uuid.uuid128,joy_uuid128_chra_read, ESP_UUID_LEN_128)){
            gl_profile_tab.charread_handle = param->add_char.attr_handle;
            gl_profile_tab.descr_uuid.len = ESP_UUID_LEN_16;
            gl_profile_tab.descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
            esp_ble_gatts_add_char_descr(gl_profile_tab.service_handle, &gl_profile_tab.descr_uuid,
                    ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,NULL,NULL);
        }
        break;
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
        ESP_LOGI(GATTS_TAG, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d\n",
                param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
        break;
    case ESP_GATTS_DELETE_EVT:
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(GATTS_TAG, "SERVICE_START_EVT, status %d, service_handle %d\n",
                param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_STOP_EVT:
        break;
    case ESP_GATTS_CONNECT_EVT:
        printf("/*----------------BLE-ADV-STOP(CONNECT)----------------*/\n");
        joy_set_ble_connect_status(ble_connect);
        ESP_LOGI(GATTS_TAG, "SERVICE_START_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x",
                param->connect.conn_id,
                param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
                param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
        gl_profile_tab.conn_id = param->connect.conn_id;
        break;
    case ESP_GATTS_DISCONNECT_EVT:
        printf("----------ESP_GATTS_DISCONNECT_EVT-------------\n");

        if(joy_get_ble_broadcast_flag() == broadcast_enble){
            joy_change_adv_wifistatus(joy_get_wifi_status());
            //esp_ble_gap_config_adv_data(&test_adv_data);
            joylink_gatts_adv_data_enable();
            printf("/*----------------CFG-ADV-DATA(DISCONNECT)----------------*/\n");
        }
        joy_set_ble_connect_status(ble_disconnect);
        jl_send_reset();
        jl_rcv_reset();
        joy_gatts_ctl_status.is_indicatedata_now = 0;
        break;
    case ESP_GATTS_CLOSE_EVT:
        printf("----------ESP_GATTS_CLOSE_EVT-------------\n");
        if(joy_get_ble_broadcast_flag() == broadcast_enble){
            joy_change_adv_wifistatus(joy_get_wifi_status());
            joylink_gatts_adv_data_enable();
            printf("/*----------------CFG-ADV-DATA(CLOSE)----------------*/\n");
        }

        joy_set_ble_connect_status(ble_disconnect);
        jl_send_reset();
        jl_rcv_reset();
        joy_gatts_ctl_status.is_indicatedata_now = 0;
        break;
    case ESP_GATTS_OPEN_EVT:
        if(param->open.status != ESP_GATT_OK){
            printf("/*--------param->connect.is_connected==false--------*/\n");
        }
        break;
    case ESP_GATTS_CANCEL_OPEN_EVT:

    case ESP_GATTS_LISTEN_EVT:
    case ESP_GATTS_CONGEST_EVT:
    default:
        break;
    }
}

void esp_joylink_ble_gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gl_profile_tab.gatts_if = gatts_if;
        } else {
            ESP_LOGI(GATTS_TAG, "Reg app failed, app_id %04x, status %d\n",
                    param->reg.app_id, 
                    param->reg.status);
            return;
        }
    }

    /* If the gatts_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
    if (gatts_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
            gatts_if == gl_profile_tab.gatts_if) {
        if (gl_profile_tab.gatts_cb) {
            gl_profile_tab.gatts_cb(event, gatts_if, param);
        }
    }
}
#endif

static uint8_t joy_create_tlv(uint16_t tag,uint8_t length,uint8_t *value,uint8_t *dest)
{
    uint8_t size = 0;
    uint8_t offset = 0;
    //tag
    memcpy(dest+offset,&tag,JOY_SIZE_FULLPACKET_TAG);
    offset+= JOY_SIZE_FULLPACKET_TAG;
    //length
    *(dest+offset) = length;
    offset+=JOY_SIZE_FULLPACKET_TAGLEN;
    //value
    memcpy(dest+offset,value,length);
    //size
    size = JOY_SIZE_FULLPACKET_TAG + JOY_SIZE_FULLPACKET_TAGLEN + length;

    return size;
}

static uint16_t joy_sdk_crc16(const uint8_t *buffer, uint32_t size)
{
    uint16_t checksum = 0xFFFF;

    if (buffer && size){
        while (size--) {
            checksum = (checksum >> 8) | (checksum << 8);
            checksum ^= *buffer++;
            checksum ^= ((unsigned char) checksum) >> 4;
            checksum ^= checksum << 12;
            checksum ^= (checksum & 0xFF) << 5;
        }
    }

    return checksum;
}

static int joy_operate_read_handler(uint8_t *cmd)
{
    int ret = -1;
    uint8_t seq,operate;
    uint16_t length,offset;
    uint16_t tag;
    uint8_t tag_lc;
    uint8_t puid[6];
    uint16_t length_sendconetent = 0,offset_send = 0;
    if(cmd == NULL){
        ret = -1;
        goto RET;
    }

    seq = *(cmd + JOY_OFFSET_FULLPACKET_SEQ);
    operate = *(cmd + JOY_OFFSET_FULLPACKET_OPERATE);
    length = *(uint16_t *)(cmd + JOY_OFFSET_FULLPACKET_LENGTH);
    ESP_LOGI(GATTS_TAG, "%s rev data:seq=%x,operate=%x,length=%2x\n", __func__,seq,operate,length);

    if((operate != JOY_OPERATE_ID_PRD) || (length > (JOY_SDK_BUFF_LEN - 6))){
        ESP_LOGE(GATTS_TAG, "%s operate or length error\n", __func__);
        ret = -1;
        goto RET;
    }
    offset = JOY_OFFSET_FULLPACKET_CONTENT;
    memset(joy_sdk_sendbuf,0,sizeof(joy_sdk_sendbuf));
    *(joy_sdk_sendbuf + JOY_OFFSET_FULLPACKET_SEQ)         = 0xA5;
    *(joy_sdk_sendbuf + JOY_OFFSET_FULLPACKET_OPERATE)     = JOY_OPERATE_ID_DRP;
    offset_send = JOY_OFFSET_FULLPACKET_CONTENT;
    length_sendconetent = 0;
    while(offset < (length + JOY_OFFSET_FULLPACKET_CONTENT)){
        tag = *(uint16_t *)(&cmd[offset]);
        switch(tag){
        uint8_t size_tlv=0;
        case JOY_PROPERTY_TAG_PUID:
            memset(puid,0x0,sizeof(puid));
            jh_load_pid(puid);
            size_tlv = joy_create_tlv(JOY_PROPERTY_TAG_PUID,sizeof(puid),puid,joy_sdk_sendbuf + offset_send);
            offset_send = offset_send + size_tlv;
            length_sendconetent = length_sendconetent + size_tlv;

            //offset
            tag_lc = cmd[offset+JOY_SIZE_FULLPACKET_TAG];
            offset += JOY_SIZE_FULLPACKET_TAG + JOY_SIZE_FULLPACKET_TAGLEN + tag_lc;
            break;
        case JOY_PROPERTY_TAG_PUBKEY_DEV:
            size_tlv = joy_create_tlv(JOY_PROPERTY_TAG_PUBKEY_DEV,sizeof(compressed_dev_pub_key),compressed_dev_pub_key, joy_sdk_sendbuf + offset_send);
            offset_send = offset_send + size_tlv;
            length_sendconetent = length_sendconetent + size_tlv;

            //offset
            tag_lc = cmd[offset+JOY_SIZE_FULLPACKET_TAG];
            offset += JOY_SIZE_FULLPACKET_TAG + JOY_SIZE_FULLPACKET_TAGLEN + tag_lc;
            break;
        case JOY_PROPERTY_TAG_GUID:
        case JOY_PROPERTY_TAG_LOCALKEY:
        case JOY_PROPERTY_TAG_DEV_SNAPSHOT:
        case JOY_PROPERTY_TAG_PUBKEY_APP:
        case JOY_PROPERTY_TAG_SECLEVEL:
        case JOY_PROPERTY_TAG_BRAND:
        case JOY_PROPERTY_TAG_CID:
        default:
            offset++;
            break;
        }
    }
#ifndef CONFIG_IDF_TARGET_ESP8266
    printf("jl_rcv_reset 3\r\n");
    jl_rcv_reset();
#endif
    if(length_sendconetent != 0){
        uint16_t crc_16,buflen;

        memcpy(joy_sdk_sendbuf+JOY_OFFSET_FULLPACKET_LENGTH,&length_sendconetent,2);
        crc_16 = joy_sdk_crc16(joy_sdk_sendbuf,length_sendconetent + 4);
        memcpy(joy_sdk_sendbuf+JOY_OFFSET_FULLPACKET_CONTENT + length_sendconetent,&crc_16,2);
        buflen = length_sendconetent + 4 + 2;
        if(buflen < JOY_SDK_BUFF_LEN){
            /*memset(joy_sdk_buff,0,sizeof(joy_sdk_buff));
            memcpy(joy_sdk_buff,joy_sdk_sendbuf,buflen);*/
            ESP_LOGI(GATTS_TAG, "%s [INFO]send buf len = %d, joy_adv_manufacture[12]&0x0f = 0x%02x\r\n", __func__,buflen, joy_adv_manufacture[12]&0x0f);

#ifndef CONFIG_IDF_TARGET_ESP8266
            switch(joy_adv_manufacture[12]&0x0f)
            {
            case 0:
                ret = jl_send_seclevel_0(buflen,joy_sdk_sendbuf);
                break;
            case 1:
                ret = jl_send_seclevel_1(buflen,joy_sdk_sendbuf);
                break;
            case 2:
                ret = jl_send_seclevel_2(buflen,joy_sdk_sendbuf);
                break;
            default:
                ret = jl_send_seclevel_0(buflen,joy_sdk_sendbuf);
                break;
            }

            printf("ret=%d\r\n",ret);
            if (joy_get_ble_connect_status() == ble_disconnect) {
                jl_send_reset();
                jl_rcv_reset();
                joy_gatts_ctl_status.is_indicatedata_now = 0;
            }
#endif
        }else{
            ESP_LOGE(GATTS_TAG, "%s [ERROR]bufer len error\n", __func__);
            goto RET;
        }
    }else{
        ESP_LOGE(GATTS_TAG, "%s [ERROR]analyze data error\n", __func__);
    }
    RET:
    return ret;
}

uint8_t try_to_start_wifi = 0;
static int joylink_operate_write_handler(uint8_t *cmd)
{
    int ret = -1;
    uint8_t seq,operate;
    uint16_t length,offset;
    uint16_t tag;
    uint8_t tag_lc;
    uint16_t length_sendconetent = 0,offset_send = 0;
    uint8_t size_tlv = 0;
    uint8_t value_response;
    //uint8_t operate_stop_gatts = 0;
    if(cmd == NULL){
        ret = -1;
        goto RET;
    }

    seq = *(cmd + JOY_OFFSET_FULLPACKET_SEQ);
    operate = *(cmd + JOY_OFFSET_FULLPACKET_OPERATE);
    length = *(uint16_t *)(cmd + JOY_OFFSET_FULLPACKET_LENGTH);
    ESP_LOGI(GATTS_TAG, "%s rev data:seq=%x,operate=%x,length=%2x,conent:\n", __func__,seq,operate,length);

    if(length > (JOY_SDK_BUFF_LEN - 6)){
        ESP_LOGE(GATTS_TAG, "%s operate or length error\n", __func__);
        ret = -1;
        goto RET;
    }

    if(operate == JOY_OPERATE_ID_PWD_WITHRES)
    {
        ESP_LOGI(GATTS_TAG, "%s init send buffer\n", __func__);
        memset(joy_sdk_sendbuf,0,sizeof(joy_sdk_sendbuf));
        *(joy_sdk_sendbuf + JOY_OFFSET_FULLPACKET_SEQ)        = seq;
        *(joy_sdk_sendbuf + JOY_OFFSET_FULLPACKET_OPERATE)    = JOY_OPERATE_ID_DRWRES;
        offset_send = JOY_OFFSET_FULLPACKET_CONTENT;
        length_sendconetent = 0;
    }

    offset = JOY_OFFSET_FULLPACKET_CONTENT;
    while(offset < (length + JOY_OFFSET_FULLPACKET_CONTENT)){
        tag = *(uint16_t *)(&cmd[offset]);
        switch(tag){
        case JOY_PROPERTY_TAG_SSID:
            offset +=  JOY_SIZE_FULLPACKET_TAG;
            tag_lc = cmd[offset];
            offset+= JOY_SIZE_FULLPACKET_TAGLEN;

            if(tag_lc <= SIZE_SSID){
                uint8_t convert_buf[SIZE_SSID+1],t;
                memset(convert_buf,0,sizeof(convert_buf));
                memset(joy_sdk_ssid_rev,0,sizeof(joy_sdk_ssid_rev));
                memcpy(convert_buf,cmd+offset,tag_lc);
                for(t = 0; t < tag_lc;t++){
                    joy_sdk_ssid_rev[tag_lc -1 -t] = convert_buf[t];
                }
                value_response = JOY_RESPONSE_OK;
                ESP_LOGI(GATTS_TAG, "%s rev ssidlen = %x,ssid = %s\n", __func__,tag_lc,joy_sdk_ssid_rev);
            }else{
                value_response = JOY_RESPONSE_ERROR1;
            }

            //offset
            offset +=tag_lc;
            if(operate == JOY_OPERATE_ID_PWD_WITHRES){
                //send buffer
                size_tlv = joy_create_tlv(JOY_PROPERTY_TAG_SSID,1,&value_response,joy_sdk_sendbuf + offset_send);
                offset_send = offset_send + size_tlv;
                length_sendconetent = length_sendconetent + size_tlv;
            }
            break;
        case JOY_PROPERTY_TAG_PASSWORD:
            offset +=  JOY_SIZE_FULLPACKET_TAG;
            tag_lc = cmd[offset];
            offset+= JOY_SIZE_FULLPACKET_TAGLEN;

            if(tag_lc <= SIZE_PWD){
                uint8_t convert_buf[SIZE_SSID+1],t;
                memset(convert_buf,0,sizeof(convert_buf));
                memset(joy_sdk_pwd_rev,0,sizeof(joy_sdk_pwd_rev));
                memcpy(convert_buf,cmd+offset,tag_lc);

                for(t = 0; t < tag_lc;t++){
                    joy_sdk_pwd_rev[tag_lc -1 -t] = convert_buf[t];
                }

                value_response = JOY_RESPONSE_OK;

                try_to_start_wifi = 1;
                ESP_LOGI(GATTS_TAG, "%s rev pwdlen = %x,pwd = %s\n", __func__,tag_lc,joy_sdk_pwd_rev);
            }else{
                value_response = JOY_RESPONSE_ERROR1;
            }

            //offset
            offset +=tag_lc;
            if(operate == JOY_OPERATE_ID_PWD_WITHRES){
                //send buffer
                size_tlv = joy_create_tlv(JOY_PROPERTY_TAG_SSID,1,&value_response,joy_sdk_sendbuf + offset_send);
                offset_send = offset_send + size_tlv;
                length_sendconetent = length_sendconetent + size_tlv;
            }
            break;
        case JOY_PROPERTY_TAG_BLE_DEV_CTL:
            offset+=JOY_SIZE_FULLPACKET_TAG;
            tag_lc = cmd[offset];
            offset += JOY_SIZE_FULLPACKET_TAGLEN;

            if(tag_lc != 1){
                offset += tag_lc;
                value_response = JOY_RESPONSE_ERROR1;
                ESP_LOGE(GATTS_TAG, "%s rev JOY_PROPERTY_TAG_BLE_DEV_CTL tag length error\n", __func__);
                break;
            }else{
                uint8_t value_ctl;
                value_ctl = cmd[offset];
                ESP_LOGI(GATTS_TAG, "%s rev JOY_PROPERTY_TAG_BLE_DEV_CTL tag with value :%d\n", __func__,value_ctl);
                if(value_ctl == 0x01 ){
                    //disconnect ble and enter to broadcast status
                    joy_set_ble_broadcast_flag(broadcast_enble);
                } else if(value_ctl == 0x02){
                    //disconnect ble and close ble
                    joy_set_ble_broadcast_flag(broadcast_disable);
                } else if(value_ctl == 0x03){
                    //disconnect ble and close ble
                    joy_set_ble_broadcast_flag(broadcast_disable);
                    joy_gatts_ctl_status.test_mode = true;
                }
                value_response = JOY_RESPONSE_OK;
            }
            offset += tag_lc;

            if(operate == JOY_OPERATE_ID_PWD_WITHRES){
                //send buffer
                size_tlv = joy_create_tlv(JOY_PROPERTY_TAG_BLE_DEV_CTL,1,&value_response,joy_sdk_sendbuf + offset_send);
                offset_send = offset_send + size_tlv;
                length_sendconetent = length_sendconetent + size_tlv;
            }
            break;
        case JOY_PROPERTY_TAG_PUBKEY_APP:
            offset +=  JOY_SIZE_FULLPACKET_TAG;
            tag_lc = cmd[offset];
            offset+= JOY_SIZE_FULLPACKET_TAGLEN;
            if(tag_lc == 21)
            {
                memcpy(compressed_app_pub_key,cmd+offset,tag_lc);
#ifndef CONFIG_IDF_TARGET_ESP8266
                jl_secure_prepare();
#endif
                value_response = JOY_RESPONSE_OK;
            }
            else
            {
                value_response = JOY_RESPONSE_ERROR1;
            }

            //offset
            offset +=tag_lc;
            if(operate == JOY_OPERATE_ID_PWD_WITHRES){
                //send buffer
                size_tlv = joy_create_tlv(JOY_PROPERTY_TAG_PUBKEY_APP,1,&value_response,joy_sdk_sendbuf + offset_send);
                offset_send = offset_send + size_tlv;
                length_sendconetent = length_sendconetent + size_tlv;
            }
            break;
        case JOY_PROPERTY_TAG_PUID:
        case JOY_PROPERTY_TAG_GUID:
        case JOY_PROPERTY_TAG_LOCALKEY:
        case JOY_PROPERTY_TAG_DEV_SNAPSHOT:
        case JOY_PROPERTY_TAG_PUBKEY_DEV:
        case JOY_PROPERTY_TAG_SECLEVEL:
        case JOY_PROPERTY_TAG_BRAND    :
        case JOY_PROPERTY_TAG_CID:
        default:
            offset+=JOY_SIZE_FULLPACKET_TAG;
            tag_lc = cmd[offset];
            offset += JOY_SIZE_FULLPACKET_TAGLEN + tag_lc;
            break;
        }
    }
#ifndef CONFIG_IDF_TARGET_ESP8266
    printf("jl_rcv_reset 1\r\n");
    jl_rcv_reset();
#endif
    if(length_sendconetent != 0)
    {
        uint16_t crc_16,buflen;

        memcpy(joy_sdk_sendbuf+JOY_OFFSET_FULLPACKET_LENGTH,&length_sendconetent,2);
        crc_16 = joy_sdk_crc16(joy_sdk_sendbuf,length_sendconetent + 4);

        memcpy(joy_sdk_sendbuf+JOY_OFFSET_FULLPACKET_CONTENT + length_sendconetent,&crc_16,2);

        buflen = length_sendconetent + 4 + 2;

#ifndef CONFIG_IDF_TARGET_ESP8266
        if(buflen < JOY_SDK_BUFF_LEN){
            ESP_LOGI(GATTS_TAG, "%s [INFO]send buf len = %x data,%d:\n", __func__,buflen,joy_gatts_ctl_status.is_indicatedata_now);
            int ret = -1;
            switch(joy_adv_manufacture[12]&0x0f)
            {
            case 0:
                printf("jl_send_seclevel_0 is executed\r\n");
                ret = jl_send_seclevel_0(buflen,joy_sdk_sendbuf);
                break;
            case 1:
                printf("jl_send_seclevel_1 is executed\r\n");
                ret = jl_send_seclevel_1(buflen,joy_sdk_sendbuf);
                break;
            case 2:
                printf("jl_send_seclevel_2 is executed\r\n");
                ret = jl_send_seclevel_2(buflen,joy_sdk_sendbuf);
                break;
            }
            printf("ret=%d\r\n",ret);
            if (joy_get_ble_connect_status() == ble_disconnect) {
                jl_send_reset();
                jl_rcv_reset();
                joy_gatts_ctl_status.is_indicatedata_now = 0;
            }
        }else{
            ESP_LOGI(GATTS_TAG, "%s no need to response\n", __func__);
        }
#endif
    }

    if(try_to_start_wifi == 1){
        ESP_LOGI(GATTS_TAG, "%s ----------start wifi-------------\n", __func__);
        esp_joylink_wifi_save_info (joy_sdk_ssid_rev,joy_sdk_pwd_rev);
        if (jd_innet_timer_task_handle != NULL) {
            jd_innet_timer_task_flag = true;
        }
        gatts_start_wifi_connect();
        joy_set_wifi_status(wifi_connecting);
        try_to_start_wifi = 0;
    }

    RET:
    return ret;
}

static int joylink_operate_response_handler(uint8_t *cmd)
{
    int ret = -1;
    uint16_t length,offset;
    uint16_t tag;
    uint8_t tag_lc;
    if(cmd == NULL){
        ret = -1;
        goto RET;
    }

    length = *(uint16_t *)(cmd + JOY_OFFSET_FULLPACKET_LENGTH);
    ESP_LOGI(GATTS_TAG, "%s rev response data,length=%2x,conent:\n", __func__,length);

    if(length > (JOY_SDK_BUFF_LEN - 6)){
        ESP_LOGE(GATTS_TAG, "%s operate or length error\n", __func__);
        ret = -1;
        goto RET;
    }

    offset = JOY_OFFSET_FULLPACKET_CONTENT;
    while(offset < (length + JOY_OFFSET_FULLPACKET_CONTENT)){
        tag = *(uint16_t *)(&cmd[offset]);
        switch(tag){
        case JOY_PROPERTY_TAG_WIFI_STATUS:
            offset +=  JOY_SIZE_FULLPACKET_TAG;
            tag_lc = cmd[offset];
            offset+= JOY_SIZE_FULLPACKET_TAGLEN;
            ESP_LOGI(GATTS_TAG, "%s receive JOY_PROPERTY_TAG_WIFI_STATUS response\n", __func__);
            offset +=tag_lc;
            break;
        default:
            offset+=JOY_SIZE_FULLPACKET_TAG;
            tag_lc = cmd[offset];
            offset += JOY_SIZE_FULLPACKET_TAGLEN + tag_lc;
            break;
        }
    }
#ifndef CONFIG_IDF_TARGET_ESP8266
    printf("jl_rcv_reset 10\r\n");
    jl_rcv_reset();
#endif
    RET:
    return ret;
}

static int joy_notify_wifi_status(type_wifi_status sta)
{
    int ret = -1;
    uint16_t length_sendconetent = 0,offset_send = 0;
    uint8_t size_tlv = 0;
    uint8_t value;
    uint16_t crc_16,buflen;

    memset(joy_sdk_sendbuf,0,sizeof(joy_sdk_sendbuf));
    *(joy_sdk_sendbuf + JOY_OFFSET_FULLPACKET_SEQ)        = seq_num++;
    *(joy_sdk_sendbuf + JOY_OFFSET_FULLPACKET_OPERATE)    = JOY_OPERATE_ID_DTP_WITHRES;
    offset_send = JOY_OFFSET_FULLPACKET_CONTENT;
    length_sendconetent = 0;

    value = (uint8_t)sta;
    size_tlv = joy_create_tlv(JOY_PROPERTY_TAG_WIFI_STATUS,1,&value,joy_sdk_sendbuf + offset_send);
    offset_send = offset_send + size_tlv;
    length_sendconetent = length_sendconetent + size_tlv;

    memcpy(joy_sdk_sendbuf+JOY_OFFSET_FULLPACKET_LENGTH,&length_sendconetent,2);

    crc_16 = joy_sdk_crc16(joy_sdk_sendbuf,length_sendconetent + 4);

    memcpy(joy_sdk_sendbuf+JOY_OFFSET_FULLPACKET_CONTENT + length_sendconetent,&crc_16,2);

    buflen = length_sendconetent + 4 + 2;

    if(buflen < JOY_SDK_BUFF_LEN){
        ESP_LOGI(GATTS_TAG, "%s [INFO]send buf len = %x data:\n", __func__,buflen);

#ifndef CONFIG_IDF_TARGET_ESP8266
        switch(joy_adv_manufacture[12]&0x0f)
        {
        case 0:
            printf("jl_send_seclevel_0 is executed\r\n");
            ret = jl_send_seclevel_0(buflen,joy_sdk_sendbuf);
            break;
        case 1:
            printf("jl_send_seclevel_1 is executed\r\n");
            ret = jl_send_seclevel_1(buflen,joy_sdk_sendbuf);
            break;
        case 2:
            printf("jl_send_seclevel_2 is executed\r\n");
            ret =jl_send_seclevel_2(buflen,joy_sdk_sendbuf);
            break;
        default:
            ret = 0;
            break;
        }
        printf("ret=%d\r\n",ret);
        if (joy_get_ble_connect_status() == ble_disconnect) {
            jl_send_reset();
            jl_rcv_reset();
            joy_gatts_ctl_status.is_indicatedata_now = 0;
        }
#endif
    }else{
        ret = 0;
        ESP_LOGE(GATTS_TAG, "%s no need to response\n", __func__);
    }

    return ret;
}

void joy_handler_task(void* arg)
{
    uint16_t cmd_id;
    for(;;) {
        printf("Free memory: %d bytes\n", esp_get_free_heap_size());
        if(xQueueReceive(cmd_evt_queue, &cmd_id, portMAX_DELAY)) {

            if(cmd_id == JOY_TASK_CMDID_REVDATAFINISH)
            {
                uint8_t operate_id;
                operate_id = *(joy_sdk_recv_buff+JOY_OFFSET_FULLPACKET_OPERATE);
                ESP_LOGI(GATTS_TAG, "%s rev full operate = %x\n", __func__,operate_id);

                switch(operate_id){
                case JOY_OPERATE_ID_PRD:
                    //phone read device property
                    joy_operate_read_handler(joy_sdk_recv_buff);
                    break;

                case JOY_OPERATE_ID_PWD_WITHNORES:
                case JOY_OPERATE_ID_PWD_WITHRES:
                    //phone write device property
                    joylink_operate_write_handler(joy_sdk_recv_buff);
                    break;

                case JOY_OPERATE_ID_PTD_RES:
                    joylink_operate_response_handler(joy_sdk_recv_buff);
                    break;

                case JOY_OPERATE_ID_DRP:
                case JOY_OPERATE_ID_DRWRES:
                case JOY_OPERATE_ID_DTP_WITHNORES:
                case JOY_OPERATE_ID_DTP_WITHRES:
                default:
#ifndef CONFIG_IDF_TARGET_ESP8266
                    printf("jl_rcv_reset 2\r\n");
                    jl_rcv_reset();
#endif
                    break;
                }
            }
            else if(cmd_id == JOY_TASK_CMDID_TEST_INDICATE)
            {
                uint8_t test_sendbuf[20] = {0x89};
                jh_send(test_sendbuf);
                ESP_LOGI(GATTS_TAG, "%s indicate send finish\n", __func__);
            }
        }
    }
}

void joy_data_resend_task(void* arg)
{
    uint16_t cmd_id;
    for(;;) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
        if(xQueueReceive(cmd_resend_queue, &cmd_id, portMAX_DELAY)) {

            if(cmd_id == JOY_RESENDTASK_RESENDWIFISTATUS)
            {
                int ret = joy_notify_wifi_status(joy_get_wifi_status());
                ESP_LOGI(GATTS_TAG, "%s-------------resend wifi status:%d\n", __func__,ret);

                if(ret == -2){
                    uint16_t cmd = JOY_RESENDTASK_RESENDWIFISTATUS;
                    xQueueSend(cmd_resend_queue,&cmd,10/portTICK_PERIOD_MS);
                }
            }
        }
    }
}

static int joy_tell_wifi_status(type_wifi_status status)
{
    int ret = -1;
    type_ble_broadcast_flag broadcast_flag     = joy_get_ble_broadcast_flag();
    type_ble_connect_status connect_flag     = joy_get_ble_connect_status();

    if(connect_flag == ble_connect){
        ret = joy_notify_wifi_status(status);
        if(ret == -2){
            uint16_t cmd = JOY_RESENDTASK_RESENDWIFISTATUS;
            xQueueSend(cmd_resend_queue,&cmd,10/portTICK_PERIOD_MS);

            ESP_LOGE(GATTS_TAG, "ERROR:%s not ok to send now\n", __func__);
        }
    }else{
        if(broadcast_flag == broadcast_enble){
            joy_change_adv_wifistatus(status);
            //esp_ble_gap_config_adv_data(&test_adv_data);
            joylink_gatts_adv_data_enable();
            ret = 0;
        }else{
            ret = -1;
        }
    }
    return ret;
}
esp_err_t esp_joylink_wifi_event_handler(void *ctx, system_event_t *event)
{
    static uint8_t reconnect_count = 0;


    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        ESP_LOGI(GATTS_TAG, "%s ++++++++++++++SYSTEM_EVENT_STA_START+++++++++++++\n", __func__);
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(GATTS_TAG, "%s ++++++++++++++wifi connected+++++++++++++\n", __func__);
#ifndef CONFIG_IDF_TARGET_ESP8266
        esp_wifi_set_promiscuous(0);
#endif
        joy_set_wifi_status(wifi_connected);

        if (joylink_net_configuaring) {
            if (joy_get_ble_connect_status() == ble_connect) {
                joy_tell_wifi_status(wifi_connected);
            } else {
                joy_change_adv_wifistatus(joy_get_wifi_status());
                joylink_gatts_adv_data_enable();
            }
        }
        joylink_net_configuaring = false;
        reconnect_count = 0;
        joylink_delay_10_min_timer_for_10_min_stop();
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently auto-reassociate. */
        if (joylink_net_configuaring) {
            if (reconnect_count < JOY_WIFI_RECONNECT_NUM) {
                reconnect_count++;
                joy_set_wifi_status(wifi_disconnect);
                if (joy_get_ble_connect_status() == ble_connect) {
                    joy_tell_wifi_status(wifi_disconnect);
                } else {
                    joy_change_adv_wifistatus(joy_get_wifi_status());
                    joylink_gatts_adv_data_enable();
                }
                ESP_LOGI(GATTS_TAG, "try to connect wifi\n");
                esp_wifi_connect();
            } else {
                void jd_innet_start_task(void);
                jd_innet_start_task ();
                reconnect_count = 0;
            }
        } else {
            esp_wifi_connect();
        }
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void gatts_start_wifi_connect(void)
{
    wifi_config_t wifi_config;

    memset(&wifi_config,0,sizeof(wifi_config));
    memcpy(wifi_config.sta.ssid,joy_sdk_ssid_rev,32);
    memcpy(wifi_config.sta.password,joy_sdk_pwd_rev,32);

    esp_wifi_set_promiscuous(0);
    // esp_wifi_disconnect();
    ESP_LOGI(GATTS_TAG, "%s Setting WiFi configuration SSID %s...start to connect\n", __func__,wifi_config.sta.ssid);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_connect() );
}

static void joy_gatts_ctl_init(void)
{
    memset(&joy_gatts_ctl_status,0,sizeof(joy_gatts_ctl_status));
    joy_gatts_ctl_status.ble_broadcast_flag = broadcast_enble;
    joy_gatts_ctl_status.wifi_status        = wifi_disconnect;
}

void joylink_ble_init(void)
{
    joy_set_wifi_status(wifi_disconnect);
#ifndef CONFIG_IDF_TARGET_ESP8266
    jl_init(joy_sdk_buff,JOY_SDK_BUFF_LEN, joy_sdk_recv_buff, JOY_SDK_RECV_BUFF_LEN);
#endif
    joy_init_adv_menufac();
    joy_gatts_ctl_init();


    cmd_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(joy_handler_task, "joy_handler_task", 4096, NULL, 10, NULL);

    cmd_resend_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(joy_data_resend_task, "joy_data_resend_task", 2048, NULL, 10, NULL);
}

int jh_get_locallkey (unsigned char *localkey)
{
    unsigned char localkey_test[16];
    memset(localkey_test,0x98,sizeof(localkey_test));
    memcpy(localkey,localkey_test,sizeof(localkey_test));
    return 0;
}

int jh_send(uint8_t* frame)
{
    if(joy_get_ble_connect_status() == ble_connect){
        joy_gatts_ctl_status.is_indicatedata_now = 1;
#ifdef CONFIG_IDF_TARGET_ESP8266
        return 0;
#else
        ESP_LOGI(GATTS_TAG, "%s gatts_if=%2x,conn_id = %2x,attr_handle=%2x\n", __func__,
                gl_profile_tab.gatts_if,
                gl_profile_tab.conn_id,
                gl_profile_tab.charindicate_handle);

        return esp_ble_gatts_send_indicate(gl_profile_tab.gatts_if,
                gl_profile_tab.conn_id,
                gl_profile_tab.charindicate_handle,
                20,
                frame,
                1);
        return 0;
#endif
    }else{
        return -1;
    }
}

int jh_load_mac(uint8_t * zone)
{
    if(zone != NULL){
        uint8_t loop = 0;
#ifdef CONFIG_IDF_TARGET_ESP8266
        for (loop = 0;loop < 6;loop++) {
            zone[loop] = 0x1;
        }
#else
        uint8_t const *addr_bt = esp_bt_dev_get_address();
        for (loop = 0;loop < 6;loop++) {
            zone[loop] = addr_bt[5 - loop];
        }
#endif
    }

    return 0;
}

int jh_load_pid(uint8_t *puid)
{
    uint8_t loop = 0;
    if ((puid != NULL) && (_g_pdev != NULL)) {
        for (loop = 0;loop < 6;loop++) {
            puid[loop] = _g_pdev->jlp.uuid[5 - loop];
        }
    }
    return 0;
}

int jh_check_ready_indication(void)
{
    int status;
    status = (joy_gatts_ctl_status.is_indicatedata_now == 1)?0:1;
    return status;
}

void joy_set_wifi_status(type_wifi_status status)
{
    joy_gatts_ctl_status.wifi_status = status;
}

static type_wifi_status joy_get_wifi_status(void)
{
    return joy_gatts_ctl_status.wifi_status;
}

static void joy_set_ble_broadcast_flag(type_ble_broadcast_flag flag)
{
    joy_gatts_ctl_status.ble_broadcast_flag = flag;
}

static type_ble_broadcast_flag joy_get_ble_broadcast_flag(void)
{
    return joy_gatts_ctl_status.ble_broadcast_flag;
}

static void joy_set_ble_connect_status(type_ble_connect_status status)
{
    joy_gatts_ctl_status.ble_connect_status = status;
}

static type_ble_connect_status joy_get_ble_connect_status(void)
{
    return joy_gatts_ctl_status.ble_connect_status;
}


uint32_t jh_system_get_time()
{
    uint64_t ret;
    struct timeval  now;

    gettimeofday(&now,NULL);
    ret = 1000000UL * now.tv_sec + now.tv_usec;
    return ret;
}

static SemaphoreHandle_t jh_semaphore;
void jh_init_semaphore()
{
    vSemaphoreCreateBinary(jh_semaphore);
}

int jh_wait_semaphore()
{
    if (jh_semaphore) {
        if(xSemaphoreTake(jh_semaphore, 0 ) == pdTRUE ) {
            return 1;
        }
    }
    return 0;
}

void jh_post_semaphore()
{
    if (jh_semaphore) {
        xSemaphoreGive(jh_semaphore);
    }
}

void jh_delete_semaphore()
{
    if (jh_semaphore) {
        vSemaphoreDelete(jh_semaphore);
        jh_semaphore = NULL;
    }
}
