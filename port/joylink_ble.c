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

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "string.h"

#ifdef CONFIG_JOYLINK_BLE_ENABLE
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"

#include "joylink_sdk.h"
#include "joylink_extern.h"

#define TAG  "jd_ble"
#define JD_DEVICE_NAME          "JD"    //The Device Name Characteristics in GAP

#define JD_PROFILE_NUM             1
#define JD_PROFILE_APP_IDX         0
#define ESP_SPP_APP_ID              0x56
#define SPP_SVC_INST_ID	            0

// JL_ADAPTER_CB_FUNC jd_ble_cb_fun = {NULL, NULL, NULL, NULL};

static jl_gatt_data_t g_jl_gatt_data;

static esp_ble_adv_params_t jd_adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t conn_id;
    uint16_t mtu_size;
    bool indicate_enable;
};

static uint8_t raw_adv_data[27] = {
        0x02,0x01,0x06,                                //le support 3
        0x03,0x03,0x70,0xFE,                        //uuid        4
        0x0f,0xFF,'4','2','4','C','8','D',0x01 ,0x20 ,0x20 ,0xc4 ,0x0a ,0x24 ,0x00, 0x50,        //menufac    D8C424 +  240AC4202001
        0x03,0x09,'J','D'                       //name
};

typedef enum {
    JD_SVC_IDX,

    JD_CHAR_1_IDX,
    JD_CHAR_1_VAL_IDX,

    JD_CHAR_2_IDX,
    JD_CHAR_2_VAL_IDX,
    JD_CHAR_2_IND_CFG,

    JD_CHAR_3_IDX,
    JD_CHAR_3_VAL_IDX,

    JD_IDX_NB
}joylink_ble_service_index_t;

typedef enum {
    JD_CHAR_1,

    JD_CHAR_2,
    JD_CHAR_2_CCC,

    JD_CHAR_3,

    INVALID_CHAR
} jd_ble_srv_char_t;


static const uint16_t JD_SERVICE_UUID    = 0xFE70;
static const uint16_t JD_CHAR_1_UUID = 0xFE71;
static const uint16_t JD_CHAR_2_UUID    = 0xFE72;
static const uint16_t JD_CHAR_3_UUID    = 0xFE74;

static uint8_t jd_char_1_value[256] = {0x0};
static uint8_t jd_char_2_value[256] = {0x0};
static uint8_t jd_char_2_val_ccc[2] = {0x00, 0x00};
static uint8_t jd_char_3_value[256] = {0x0};

static uint16_t jd_srv_handle_table[JD_IDX_NB];

#define CHAR_DECLARATION_SIZE   (sizeof(uint8_t))

static const uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;

static const uint8_t char_prop_write = ESP_GATT_CHAR_PROP_BIT_WRITE;
static const uint8_t char_prop_indicate = ESP_GATT_CHAR_PROP_BIT_INDICATE;
static const uint8_t char_prop_write_with_nr = ESP_GATT_CHAR_PROP_BIT_WRITE_NR;

/* DIS Full Database Description - Used to add attributes into the database */
static const esp_gatts_attr_db_t jd_gatt_db[JD_IDX_NB] =
{
    // Service Declaration
    [JD_SVC_IDX]        =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
      sizeof(JD_SERVICE_UUID), sizeof(JD_SERVICE_UUID), (uint8_t *)&JD_SERVICE_UUID}},

    /* Characteristic Declaration */
    [JD_CHAR_1_IDX]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_write}},

    /* Characteristic Value */
    [JD_CHAR_1_VAL_IDX]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&JD_CHAR_1_UUID, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(jd_char_1_value), sizeof(jd_char_1_value), (uint8_t *)jd_char_1_value}},

    /* Characteristic Declaration */
    [JD_CHAR_2_IDX]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_indicate}},

    /* Characteristic Value */
    [JD_CHAR_2_VAL_IDX]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&JD_CHAR_2_UUID, ESP_GATT_PERM_READ,
      sizeof(jd_char_2_value), sizeof(jd_char_2_value), (uint8_t *)jd_char_2_value}},

    [JD_CHAR_2_IND_CFG]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(jd_char_2_val_ccc), sizeof(jd_char_2_val_ccc), (uint8_t *)jd_char_2_val_ccc}},

    /* Characteristic Declaration */
    [JD_CHAR_3_IDX]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_write_with_nr}},

    /* Characteristic Value */
    [JD_CHAR_3_VAL_IDX]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&JD_CHAR_3_UUID, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(jd_char_3_value), sizeof(jd_char_3_value), (uint8_t *)jd_char_3_value}},
};

static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst jd_profile_tab[JD_PROFILE_NUM] = {
    [JD_PROFILE_APP_IDX] = {
        .gatts_cb = gatts_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
        .conn_id  = 0xff,
        .mtu_size = 23,
        .indicate_enable = false,
    },
};

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    ESP_LOGI(TAG, "EVT %d, gatts if %d\n", event, gatts_if);

    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            jd_profile_tab[JD_PROFILE_APP_IDX].gatts_if = gatts_if;
        } else {
            ESP_LOGI(TAG, "Reg app failed, app_id %04x, status %d\n",param->reg.app_id, param->reg.status);
            return;
        }
    }

    do {
        int idx;
        for (idx = 0; idx < JD_PROFILE_NUM; idx++) {
            if (gatts_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                    gatts_if == jd_profile_tab[idx].gatts_if) {
                if (jd_profile_tab[idx].gatts_cb) {
                    jd_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    esp_err_t err;
    esp_bd_addr_t bd_addr;
    ESP_LOGI(TAG, "GAP_EVT, event %d\n", event);

    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
        esp_ble_gap_start_advertising(&jd_adv_params);
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        //advertising start complete event to indicate advertising start successfully or failed
        if((err = param->adv_start_cmpl.status) != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "Advertising start failed: %s\n", esp_err_to_name(err));
        }
        break;
    case ESP_GAP_BLE_SEC_REQ_EVT:
        for(int i = 0; i < ESP_BD_ADDR_LEN; i++) {
            ESP_LOGD(TAG, "%x:",param->ble_security.ble_req.bd_addr[i]);
        }
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
        break;
    case ESP_GAP_BLE_AUTH_CMPL_EVT:
        memcpy(bd_addr, param->ble_security.auth_cmpl.bd_addr, sizeof(esp_bd_addr_t));
        ESP_LOGI(TAG, "remote BD_ADDR: %08x%04x",\
                    (bd_addr[0] << 24) + (bd_addr[1] << 16) + (bd_addr[2] << 8) + bd_addr[3],
                    (bd_addr[4] << 8) + bd_addr[5]);
        ESP_LOGI(TAG, "address type = %d", param->ble_security.auth_cmpl.addr_type);
        ESP_LOGI(TAG, "pair status = %s",param->ble_security.auth_cmpl.success ? "success" : "fail");
        if(!param->ble_security.auth_cmpl.success) {
            ESP_LOGE(TAG, "fail reason = 0x%x",param->ble_security.auth_cmpl.fail_reason);
        }
            break;
    default:
        break;
    }
}

static jd_ble_srv_char_t find_ftms_char_and_desr_by_handle(uint16_t handle)
{
    jd_ble_srv_char_t ret = INVALID_CHAR;

    for(int i = 0; i < JD_IDX_NB ; i++){
        if(handle == jd_srv_handle_table[i]){
            switch(i){
                case JD_CHAR_1_VAL_IDX:
                    ret = JD_CHAR_1;
                    break;
                case JD_CHAR_2_VAL_IDX:
                    ret = JD_CHAR_2;
                    break;
                case JD_CHAR_2_IND_CFG:
                    ret = JD_CHAR_2_CCC;
                    break;
                case JD_CHAR_3_VAL_IDX:
                    ret = JD_CHAR_3;
                    break;
                default:
                    ret = INVALID_CHAR;
                    break;
            }
        }
    }

    return ret;
}

extern bool get_rst;

static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    esp_err_t ret;
    jd_ble_srv_char_t jd_char;
    int pkt_process_ret = 0;

    ESP_LOGI(TAG, "event = %x\n",event);
    switch (event) {
    	case ESP_GATTS_REG_EVT:
            ESP_LOGI(TAG, "%s %d\n", __func__, __LINE__);
        	esp_ble_gap_set_device_name(JD_DEVICE_NAME);

        	ESP_LOGI(TAG, "%s %d\n", __func__, __LINE__);
        	esp_ble_gap_config_adv_data_raw((uint8_t *)raw_adv_data, sizeof(raw_adv_data));

            ret = esp_ble_gatts_create_attr_tab(jd_gatt_db, gatts_if, JD_IDX_NB, JD_PROFILE_APP_IDX);
            if (ret){
                ESP_LOGE(TAG, "%s - create attr table failed, error code = %x", __func__, ret);
            }
       	    break;
    	case ESP_GATTS_READ_EVT:
            jd_char = find_ftms_char_and_desr_by_handle(param->read.handle);
            ESP_LOGI(TAG, "client read, jd_char: %d", jd_char);
       	    break;
    	case ESP_GATTS_WRITE_EVT:
            ESP_LOGI(TAG, "ESP_GATTS_WRITE_EVT: handle = %d", param->write.handle);
            jd_char = find_ftms_char_and_desr_by_handle(param->write.handle);
            if(jd_char == JD_CHAR_1){
                ESP_LOGI(TAG, "ESP_GATTS_WRITE_EVT: char = %d", jd_char);
                ESP_LOG_BUFFER_HEX_LEVEL(TAG, param->write.value, param->write.len,ESP_LOG_INFO);
                // pkt_process_ret = jl_process_packet(param->write.value, 0);
                // jl_write_data(0, param->write.value, param->write.len);
                jl_write_data(param->write.value, param->write.len);
                // ESP_LOGI(TAG, "process pkt result: %d", pkt_process_ret);
            } else if ((param->write.len == 2)&&(param->write.value[0] == 0x02)&&(param->write.value[1] == 0x00) && (jd_char == JD_CHAR_2_CCC)) {
                ESP_LOG_BUFFER_HEX_LEVEL(TAG, param->write.value, param->write.len,ESP_LOG_INFO);
                jd_profile_tab[JD_PROFILE_APP_IDX].indicate_enable = true;
                ESP_LOGI(TAG, "enable indicate: char = %d", jd_char);
            } else if ((param->write.len == 2)&&(param->write.value[0] == 0x00)&&(param->write.value[1] == 0x00) && (jd_char == JD_CHAR_2_CCC)) {
                ESP_LOG_BUFFER_HEX_LEVEL(TAG, param->write.value, param->write.len,ESP_LOG_INFO);
                jd_profile_tab[JD_PROFILE_APP_IDX].indicate_enable = false;
                ESP_LOGI(TAG, "disable indicate: char = %d", jd_char);
            } else if (jd_char == JD_CHAR_3) {
                ESP_LOG_BUFFER_HEX_LEVEL(TAG, param->write.value, param->write.len,ESP_LOG_INFO);
                ESP_LOGI(TAG, "ESP_GATTS_WRITE_EVT: char = %d", jd_char);;
            }
      	 	break;
    	case ESP_GATTS_EXEC_WRITE_EVT:
            ESP_LOGI(TAG, "ESP_GATTS_EXEC_WRITE_EVT %s - L%d", __func__, __LINE__); 
    	    break;
    	case ESP_GATTS_MTU_EVT:
            ESP_LOGI(TAG, "ESP_GATTS_MTU_EVT - mtu = %d - %s - L%d", param->mtu.mtu, __func__, __LINE__);
            jd_profile_tab[JD_PROFILE_APP_IDX].mtu_size = param->mtu.mtu;
    	    break;
    	case ESP_GATTS_CONF_EVT:
            ESP_LOGI(TAG, "ESP_GATTS_CONF_EVT - handle = %d, status = %d", param->conf.handle, param->conf.status);
            if (param->conf.status == ESP_GATT_OK) {
                jl_indicate_confirm_send_data();
                get_rst = true;
            }
    	    break;
    	case ESP_GATTS_UNREG_EVT:
        	break;
    	case ESP_GATTS_DELETE_EVT:
        	break;
    	case ESP_GATTS_START_EVT:
            ESP_LOGI(TAG, "SERVICE_START_EVT, status %d, service_handle %d", param->start.status, param->start.service_handle);
            if (param->start.status != ESP_GATT_OK) {
                ESP_LOGE(TAG, "SERVICE START FAIL, status %d", param->start.status);
                break;
            }
        	break;
    	case ESP_GATTS_STOP_EVT:
        	break;
    	case ESP_GATTS_CONNECT_EVT:
            jd_profile_tab[JD_PROFILE_APP_IDX].conn_id = param->connect.conn_id;
        	break;
    	case ESP_GATTS_DISCONNECT_EVT:
            jd_profile_tab[JD_PROFILE_APP_IDX].mtu_size = 23;
            jd_profile_tab[JD_PROFILE_APP_IDX].conn_id = 0xff;
    	    break;
    	case ESP_GATTS_OPEN_EVT:
    	    break;
    	case ESP_GATTS_CANCEL_OPEN_EVT:
    	    break;
    	case ESP_GATTS_CLOSE_EVT:
    	    break;
    	case ESP_GATTS_LISTEN_EVT:
    	    break;
    	case ESP_GATTS_CONGEST_EVT:
    	    break;
    	case ESP_GATTS_CREAT_ATTR_TAB_EVT:
            if (param->add_attr_tab.status != ESP_GATT_OK){
                ESP_LOGE(TAG, "create attribute table failed, error code=0x%x", param->add_attr_tab.status);
            } else if (param->add_attr_tab.num_handle != JD_IDX_NB){
                ESP_LOGE(TAG, "create attribute table abnormally, num_handle (%d) doesn't equal to DIS_IDX_NB(%d)", param->add_attr_tab.num_handle, JD_IDX_NB);
            } else {
                ESP_LOGI(TAG, "create attribute table successfully, the number handle = %d\n",param->add_attr_tab.num_handle);
                memcpy(jd_srv_handle_table, param->add_attr_tab.handles, sizeof(jd_srv_handle_table));
                esp_ble_gatts_start_service(jd_srv_handle_table[JD_SVC_IDX]);
            }

    	    break;
    	default:
    	    break;
    }
}

static esp_err_t esp_ble_ftmp_notification_value(joylink_ble_service_index_t index, uint8_t * value, uint8_t length, bool need_ack)
{
    esp_err_t ret;
    uint16_t offset = 0;

    if (length <= (jd_profile_tab[JD_PROFILE_APP_IDX].mtu_size - 3)) {
        ret = esp_ble_gatts_send_indicate(jd_profile_tab[JD_PROFILE_APP_IDX].gatts_if, jd_profile_tab[JD_PROFILE_APP_IDX].conn_id, jd_srv_handle_table[index], length, value, need_ack);
        if (ret) {
            ESP_LOGE(TAG, "%s send notification fail: %s\n", __func__, esp_err_to_name(ret));
            return ESP_FAIL;
        }
    } else {
        while ((length - offset) > (jd_profile_tab[JD_PROFILE_APP_IDX].mtu_size - 3)) {
            ret = esp_ble_gatts_send_indicate(jd_profile_tab[JD_PROFILE_APP_IDX].gatts_if, jd_profile_tab[JD_PROFILE_APP_IDX].conn_id, jd_srv_handle_table[index], (jd_profile_tab[JD_PROFILE_APP_IDX].mtu_size - 3), value + offset, need_ack);
            if (ret) {
                ESP_LOGE(TAG, "%s send notification fail: %s\n", __func__, esp_err_to_name(ret));
                return ESP_FAIL;
            }
            offset += (jd_profile_tab[JD_PROFILE_APP_IDX].mtu_size - 3);
        }

        if ((length - offset) > 0) {
            ret = esp_ble_gatts_send_indicate(jd_profile_tab[JD_PROFILE_APP_IDX].gatts_if, jd_profile_tab[JD_PROFILE_APP_IDX].conn_id, jd_srv_handle_table[index], (length - offset), value + offset, need_ack);
            if (ret) {
                ESP_LOGE(TAG, "%s send notification fail: %s\n", __func__, esp_err_to_name(ret));
                return ESP_FAIL;
            }
        }
    }

    return ESP_OK;
}

int32_t esp_ble_ftms_notification_data(uint8_t * value, uint32_t length)
{
    int32_t ret = ESP_FAIL;

    if (jd_profile_tab[JD_PROFILE_APP_IDX].indicate_enable) {
        ret = esp_ble_ftmp_notification_value(JD_CHAR_2_VAL_IDX, value, length, true);
    } else {
        ESP_LOGE(TAG, "indicate isn't enable");
    }

    if (ret) {
        ESP_LOGE(TAG, "%s JD incication fail: %s\n", __func__, esp_err_to_name(ret));
        return ESP_FAIL;
    }

    return ESP_OK;
}

int32_t esp_ble_get_mac(uint8_t *buf, uint32_t size)
{
    //240AC4202001
    buf[0] = 0x24;
    buf[1] = 0x0a;
    buf[2] = 0xc4;
    buf[3] = 0x20;
    buf[4] = 0x20;
    buf[5] = 0x01;

    return 0;
}

uint32_t jh_system_get_time()
{
    uint64_t ret;
    struct timeval  now;

    gettimeofday(&now,NULL);
    ret = 1000000UL * now.tv_sec + now.tv_usec;
    return ret;
}

int esp_ble_gatts_send_data(uint8_t*frame, uint32_t frame_len)
{ 
    int ret = 0;
    // ret = esp_ble_gatts_send_indicate(gl_profile_tab.gatts_if, 
    //                                   gl_profile_tab.conn_id,
    //                                   gl_profile_tab.charindicate_handle,
    //                                   frame_len, frame, 1);

    ret = esp_ble_ftms_notification_data(frame, frame_len);
    return ret;
}

void ble_start(void)
{
    esp_err_t ret;
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    jl_dev_info_t dev_info;
	
    memcpy(dev_info.product_uuid, CONFIG_JOYLINK_DEVICE_UUID, 6);
    for(int i = 0; i < 6; i++) {
        raw_adv_data[14-i] = dev_info.product_uuid[i];
    }

    char mac_buf[12] = {0x0};
    joylink_dev_get_user_mac(mac_buf);
	// uint8_t mac[6] = {0x24,0x0a,0xc4,0x20,0x20,0x01};
    uint8_t mac[6] = {0};

    for (int i = 0; i < 6 ; i++) {
        mac[i] = ((mac_buf[2*i] - 0x30) << 4) + (mac_buf[2*i + 1] - 0x30);
    }

	memcpy(dev_info.mac, mac, 6);

	jl_init(&dev_info);

	jl_get_gatt_config_data(&g_jl_gatt_data);

    jl_gap_data_t gap_data;
    jl_get_gap_config_data(&gap_data);

    printf("service_uuid16: 0x%02x 0x%02x\r\n", gap_data.service_uuid16[0], gap_data.service_uuid16[1]);

    printf("manufacture_data:");

    for(int i = 0; i < 14; i++) {
        printf("0x%02x ", gap_data.manufacture_data[i]);
    }

    printf("\r\n");

    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "%s init bluetooth\n", __func__);
    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(TAG, "%s init bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(TAG, "%s enable bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    esp_ble_gatts_register_callback(gatts_event_handler);
    esp_ble_gap_register_callback(gap_event_handler);
    esp_ble_gatts_app_register(ESP_SPP_APP_ID);

    /* set the security iocap & auth_req & key size & init key response key parameters to the stack*/
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_BOND;     //bonding with peer device after authentication
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;           //set the IO capability to No output No input
    uint8_t key_size = 16;      //the key size should be 7~16 bytes
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    /* If your BLE device act as a Slave, the init_key means you hope which types of key of the master should distribute to you,
    and the response key means which key you can distribute to the Master;
    If your BLE device act as a master, the response key means you hope which types of key of the slave should distribute to you, 
    and the init key means which key you can distribute to the slave. */
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));


    return;
}
#endif