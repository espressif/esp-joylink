/* --------------------------------------------------
 * @brief: 
 *
 * @version: 1.0
 *
 * @date: 10/08/2015 09:28:27 AM
 *
 * --------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nvs.h"
#include "nvs_flash.h"

#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_partition.h"

#include "tcpip_adapter.h"

#include "cJSON.h"

#include "joylink.h"
#include "joylink_packets.h"
#include "joylink_json.h"
#include "joylink_extern_json.h"

#include "esp_joylink.h"

#define JOYLINK_NVS_NAMESPACE       "joylink"
#define JOYLINK_NVS_CONFIG_NETWORK  "config_network"

#define JLP_MAC "12:34:56:78:68:88"

#define JLP_DEV_TYPE	  E_JLDEV_TYPE_NORMAL
#define JLP_LAN_CTRL	  E_LAN_CTRL_ENABLE
// #define JLP_LAN_CTRL      E_LAN_CTRL_DISABLE
#define JLP_CMD_TYPE	  E_CMD_TYPE_LUA_SCRIPT
#define JLP_SNAPSHOT      E_SNAPSHOT_NO

#define JLP_UUID          CONFIG_JOYLINK_DEVICE_UUID
#define JLP_CLOUD_PUB_KEY CONFIG_JOYLINK_PUBLIC_KEY
#define JLP_PRIVATE_KEY   CONFIG_JOYLINK_PRIVATE_KEY

typedef struct _light_manage_{
	int conn_st;	
    JLPInfo_t jlp;
    jl2_d_idt_t idt;
	LightCtrl_t lightCtrl;
}LightManage_t;

/*E_JLDEV_TYPE_GW*/
#ifdef _SAVE_FILE_
char  *file = "joylink_info.txt";
#endif

LightManage_t _g_lightMgr = {
	.conn_st = -1,	

    .jlp.version = 1,
    .jlp.uuid = JLP_UUID,

    .jlp.devtype = JLP_DEV_TYPE,
    .jlp.lancon = JLP_LAN_CTRL,
    .jlp.cmd_tran_type = JLP_CMD_TYPE,
    .jlp.noSnapshot = JLP_SNAPSHOT,
    	

    .idt.type = 0,
    .idt.cloud_pub_key = JLP_CLOUD_PUB_KEY,
    .idt.pub_key = "01234567890123456789012345678901",
    .idt.sig = "01234567890123456789012345678901",
    .idt.f_sig = "01234567890123456789012345678901",
    .idt.f_pub_key = "01234567890123456789012345678901",
};

LightManage_t *_g_pLightMgr = &_g_lightMgr;

user_dev_status_t user_dev;

extern int joylink_parse_jlp(JLPInfo_t *jlp, char * pMsg);

extern int joylink_util_cut_ip_port(const char *ipport, char *out_ip, int *out_port);

void suffix_object(cJSON *prev,cJSON *item) {prev->next=item;item->prev=prev;}

/**
 * brief: 
 *
 * @Param:
 *
 * @Returns: 
 */
int
joylink_dev_get_random()
{
    /**
     *FIXME:must to do
     */
    int i;
    srand(time(NULL));
    i = rand();
    return i;
}

/**
 * brief:
 * Check dev net is ok. 
 * @Param: st
 *
 * @Returns: 
 *  E_RET_TRUE
 *  E_RET_FAIL
 */
E_JLRetCode_t
joylink_dev_is_net_ok()
{
    /**
     *FIXME:must to do
     */
    int ret = E_RET_TRUE;
	tcpip_adapter_ip_info_t ip_config;

	if (tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA,&ip_config) != ESP_OK) {
        return E_RET_FAIL;
    }
	
	if(ip_config.ip.addr == 0){
		ret = E_RET_FAIL;
	} else {
		ret = E_RET_TRUE;
	}
	
    return ret;
}


/**
 * brief:
 * When connecting server st changed,
 * this fun will be called.
 *
 * @Param: st
 * JL_SERVER_ST_INIT      (0)
 * JL_SERVER_ST_AUTH      (1)
 * JL_SERVER_ST_WORK      (2)
 *
 * @Returns: 
 *  E_RET_OK
 *  E_RET_ERROR
 */
E_JLRetCode_t
joylink_dev_set_connect_st(int st)
{
    /**
     *FIXME:must to do
     */
    char buff[64] = {0};
    int ret = 0;
    sprintf(buff, "{\"conn_status\":\"%d\"}", st);
	log_info("--set_connect_st:%s\n", buff);
	
	// Save the state.
	_g_pLightMgr->conn_st = st;
	
    return ret;
}

/**
 * brief:
 * Save joylink protocol info in flash.
 *
 * @Param:jlp 
 *
 * @Returns: 
 *  E_RET_OK
 *  E_RET_ERROR
 */
E_JLRetCode_t
joylink_dev_set_attr_jlp(JLPInfo_t *jlp)
{
    nvs_handle out_handle;
    if(NULL == jlp){
        return E_RET_ERROR;
    }
    /**
     *FIXME:must to do
     */
    log_debug("--joylink_dev_set_attr_jlp");
    if (nvs_open(JOYLINK_NVS_NAMESPACE, NVS_READWRITE, &out_handle) != ESP_OK) {
        return E_RET_ERROR;
    }
    if(strlen(jlp->feedid)){
        log_debug("--set feedid: %s", jlp->feedid);
        if (nvs_set_str(out_handle,"feedid",jlp->feedid) != ESP_OK) {
            log_debug("--set feedid fail");
            return E_RET_ERROR;
        }
    }

    if(strlen(jlp->accesskey)){
        log_debug("--set accesskey: %s", jlp->accesskey);
        if (nvs_set_str(out_handle,"accesskey",jlp->accesskey) != ESP_OK) {
            log_debug("--set accesskey fail");
            return E_RET_ERROR;
        }
    }

    if(strlen(jlp->localkey)){
        log_debug("--set localkey: %s", jlp->localkey);
        if (nvs_set_str(out_handle,"localkey",jlp->localkey) != ESP_OK) {
            log_debug("--set localkey fail");
            return E_RET_ERROR;
        }
    }

    if(strlen(jlp->joylink_server)){
        log_debug("--set joylink_server: %s", jlp->joylink_server);
        if (nvs_set_str(out_handle,"joylink_server",jlp->joylink_server) != ESP_OK) {
            log_debug("--set joylink_server fail");
            return E_RET_ERROR;
        }

        log_debug("--set server_port: %d", jlp->server_port);
        if (nvs_set_u16(out_handle,"server_port",jlp->server_port) != ESP_OK) {
            log_debug("--set server_port fail");
            return E_RET_ERROR;
        }
    }

    nvs_close(out_handle);
    return E_RET_OK;
}

/**
 * brief: 
 *
 * @Param: jlp
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_dev_get_idt(jl2_d_idt_t *pidt)
{
    if(NULL == pidt){
        return E_RET_ERROR; 
    }

    strcpy(pidt->pub_key, _g_pLightMgr->idt.pub_key);
    strcpy(pidt->sig, _g_pLightMgr->idt.sig);
    strcpy(pidt->f_sig, _g_pLightMgr->idt.f_sig);
    strcpy(pidt->f_pub_key, _g_pLightMgr->idt.f_pub_key);
    strcpy(pidt->cloud_pub_key, _g_pLightMgr->idt.cloud_pub_key);

    return E_RET_OK;
}

/**
 * brief: 
 *
 * @Param: jlp
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_dev_get_jlp_info(JLPInfo_t *jlp)
{
    nvs_handle out_handle;

    if(NULL == jlp){
        return E_RET_ERROR;
    }
    /**
     *FIXME:must to do
     */
    int ret = E_RET_ERROR;
    size_t size = 0;
	static bool init_flag = false;
    log_debug("--joylink_dev_get_jlp_info");

	if (nvs_open(JOYLINK_NVS_NAMESPACE, NVS_READONLY, &out_handle) == ESP_OK) {        
		size = sizeof(_g_pLightMgr->jlp.feedid);
	    if (nvs_get_str(out_handle,"feedid",_g_pLightMgr->jlp.feedid,&size) != ESP_OK) {
		    log_debug("--get feedid fail");
		}
        log_debug("--get feedid: %s", jlp->feedid);

		size = sizeof(_g_pLightMgr->jlp.accesskey);
	    if (nvs_get_str(out_handle,"accesskey",_g_pLightMgr->jlp.accesskey,&size) != ESP_OK) {
		    log_debug("--get accesskey fail");
	    }
        log_debug("--get accesskey: %s", jlp->accesskey);

		size = sizeof(_g_pLightMgr->jlp.localkey);
	    if (nvs_get_str(out_handle,"localkey",_g_pLightMgr->jlp.localkey,&size) != ESP_OK) {
		    log_debug("--get localkey fail");
	    }
        log_debug("--set localkey: %s", jlp->localkey);

		size = sizeof(_g_pLightMgr->jlp.joylink_server);
	    if (nvs_get_str(out_handle,"joylink_server",_g_pLightMgr->jlp.joylink_server,&size) != ESP_OK) {
		    log_debug("--get joylink_server fail");
	    }
        log_debug("--set joylink_server: %s", jlp->joylink_server);

		if (nvs_get_u16(out_handle,"server_port",&_g_pLightMgr->jlp.server_port) != ESP_OK) {
		    log_debug("--get server_port fail");
	    }
        log_debug("--set server_port: %d", jlp->server_port);

        _g_pLightMgr->jlp.is_actived = 1;
		
		nvs_close(out_handle);
	}

    if(joylink_dev_get_user_mac(jlp->mac) < 0){
		log_info("can't get mac!\n");
	}

	if(joylink_dev_get_private_key(jlp->prikey) < 0){
		log_info("can't get private_key!\n");
	}
    
    strcpy(jlp->feedid, _g_pLightMgr->jlp.feedid);
    strcpy(jlp->accesskey, _g_pLightMgr->jlp.accesskey);
    strcpy(jlp->localkey, _g_pLightMgr->jlp.localkey);
    strcpy(jlp->joylink_server, _g_pLightMgr->jlp.joylink_server);
    jlp->server_port =  _g_pLightMgr->jlp.server_port;
	jlp->devtype = _g_pLightMgr->jlp.devtype;
    jlp->lancon = _g_pLightMgr->jlp.lancon;
    jlp->cmd_tran_type = _g_pLightMgr->jlp.cmd_tran_type;
    jlp->version = _g_pLightMgr->jlp.version;
    jlp->is_actived = _g_pLightMgr->jlp.is_actived;
    strcpy(jlp->mac, _g_pLightMgr->jlp.mac);
    strcpy(jlp->uuid, _g_pLightMgr->jlp.uuid);

    return ret;
}

/**
 * brief: 
 *
 * @Param: out_modelcode
 * @Param: out_max
 *
 * @Returns: 
 */
int
joylink_dev_get_modelcode(char *out_modelcode, int32_t out_max)
{
    if(NULL == out_modelcode || out_max < 0){
        return 0;
    }
    /**
     *FIXME:must to do
     */
    int len = 0;
	
    char *packet_data =  joylink_dev_modelcode_info(0, &user_dev);
    if(NULL !=  packet_data){
        len = strlen(packet_data);
        log_info("------>%s:len:%d\n", packet_data, len);
        if(len < out_max){
            memcpy(out_modelcode, packet_data, len); 
        }else{
            len = 0;
        }
    }

    if(NULL !=  packet_data){
        free(packet_data);
    }

    return len;
}

int joylink_dev_user_data_get(user_dev_status_t *user_data)
{
	/**
	*FIXME:must to do
	*/

    memcpy(user_data, &user_dev, sizeof(user_dev_status_t));

    // user_data->Filter1Life = 11;
    // user_data->Filter5Life = 22;
    // user_data->Filter6Life = 33;
    
	return 0;
}

/**
 * brief: 
 *
 * @Param: out_snap
 * @Param: out_max
 *
 * @Returns: 
 */
int
joylink_dev_get_snap_shot_with_retcode(int32_t ret_code, char *out_snap, int32_t out_max)
{
    if(NULL == out_snap || out_max < 0){
        return 0;
    }
    /**
     *FIXME:must to do
     */
    int len = 0;
	
    joylink_dev_user_data_get(&user_dev);
	
    char *packet_data =  joylink_dev_package_info(ret_code, &user_dev);
    if(NULL !=  packet_data){
        len = strlen(packet_data);
        log_info("------>%s:len:%d\n", packet_data, len);
        if(len < out_max){
            memcpy(out_snap, packet_data, len); 
        }else{
            len = 0;
        }
    }

    if(NULL !=  packet_data){
        free(packet_data);
    }

    return len;
}

/**
 * brief: 
 *
 * @Param: out_snap
 * @Param: out_max
 *
 * @Returns: 
 */
int
joylink_dev_get_snap_shot(char *out_snap, int32_t out_max)
{
    return joylink_dev_get_snap_shot_with_retcode(0, out_snap, out_max); 
}

/**
 * brief: 
 *
 * @Param: out_snap
 * @Param: out_max
 * @Param: code
 * @Param: feedid
 *
 * @Returns: 
 */
int
joylink_dev_get_json_snap_shot(char *out_snap, int32_t out_max, int code, char *feedid)
{
    /**
     *FIXME:must to do
     */
    sprintf(out_snap, "{\"code\":%d, \"feedid\":\"%s\"}", code, feedid);

    return strlen(out_snap);
}

/**
 * brief: 
 *
 * @Param: json_cmd
 *
 * @Returns: 
 */
E_JLRetCode_t 
joylink_dev_lan_json_ctrl(const char *json_cmd)
{
    /**
     *FIXME:must to do
     */
    log_debug("json ctrl:%s", json_cmd);
    joylink_dev_parse_ctrl(json_cmd, &user_dev);

    return E_RET_OK;
}

/**
 * brief: 
 *
 * @Param: src
 * @Param: src_len
 * @Param: ctr
 * @Param: from_server
 *
 * @Returns: 
 */
E_JLRetCode_t 
joylink_dev_script_ctrl(const char *src, int src_len, JLContrl_t *ctr, int from_server)
{
    if(NULL == src || NULL == ctr){
        return -1;
    }
    /**
     *FIXME:must to do
     */
    int ret = -1;
    ctr->biz_code = (int)(*((int *)(src + 4)));
    ctr->serial = (int)(*((int *)(src +8)));
	
    time_t tt = time(NULL);
    log_info("bcode:%d:server:%d:time:%ld", ctr->biz_code, from_server,(long)tt);

    if(ctr->biz_code == JL_BZCODE_GET_SNAPSHOT){
        /*
         *Nothing to do!
         */
        ret = 0;
    }else if(ctr->biz_code == JL_BZCODE_CTRL){
        joylink_dev_parse_ctrl(src + 12, &user_dev);
#ifdef __MTK_7687__
		gpio_ligt_ctrl(user_dev.power);
#endif
        ret = 0;
    }else{
        log_error("unKown biz_code:%d", ctr->biz_code);
    }
    
    return ret;
}

/**
 * brief: 
 *
 * @Param: otaOrder
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_dev_ota(JLOtaOrder_t *otaOrder)
{
    if(NULL == otaOrder){
        return -1;
    }
    int ret = E_RET_OK;

    log_debug("serial:%d | feedid:%s | productuuid:%s | version:%d | versionname:%s | crc32:%d | url:%s\n",
     otaOrder->serial, otaOrder->feedid, otaOrder->productuuid, otaOrder->version, 
     otaOrder->versionname, otaOrder->crc32, otaOrder->url);

    esp_ota_task_start((const char*)otaOrder->url);

    return ret;
}

/**
 * brief: 
 */
void
joylink_dev_ota_status_upload()
{
    JLOtaUpload_t otaUpload;    
    strcpy(otaUpload.feedid, _g_pdev->jlp.feedid);
    strcpy(otaUpload.productuuid, _g_pdev->jlp.uuid);	

#ifdef __MTK_7687__
    otaUpload.status = _g_fota_ctx.upgrade_status;
    otaUpload.progress = _g_fota_ctx.progress;
    strcpy(otaUpload.status_desc, joylink_fota_get_status_desc(FOTA_ERROR_CODE_NONE));
#endif
    joylink_server_ota_status_upload_req(&otaUpload);   
}


esp_joylink_config_network_t esp_joylink_get_config_network(void)
{
	nvs_handle out_handle;
	uint8_t config_network;
	if (nvs_open(JOYLINK_NVS_CONFIG_NETWORK, NVS_READONLY, &out_handle) != ESP_OK) {
		return ESP_JOYLINK_CONFIG_NETWORK_NONE;
	}

	if (nvs_get_u8(out_handle,"config_network",&config_network) != ESP_OK) {
		log_debug("--get config_network fail");
		return ESP_JOYLINK_CONFIG_NETWORK_NONE;
	}

	nvs_close(out_handle);
	printf("esp_joylink_get_config_network:%d\r\n",config_network);
	if ((config_network >= ESP_JOYLINK_CONFIG_NETWORK_NONE)
            && (config_network <= ESP_JOYLINK_CONFIG_NETWORK_MAX)) {
        return config_network;
    }
    
	return ESP_JOYLINK_CONFIG_NETWORK_NONE;;
}
void esp_joylink_set_config_network(esp_joylink_config_network_t config_network)
{
	nvs_handle out_handle;
    
    if ((config_network < ESP_JOYLINK_CONFIG_NETWORK_NONE)
       || (config_network > ESP_JOYLINK_CONFIG_NETWORK_MAX)) {
        return;
    }
	if (nvs_open(JOYLINK_NVS_CONFIG_NETWORK, NVS_READWRITE, &out_handle) != ESP_OK) {
		return;
	}

	if (nvs_set_u8(out_handle,"config_network",config_network) != ESP_OK) {
		log_debug("--set config_network fail");
		return;
	}
	nvs_close(out_handle);
	printf("esp_joylink_set_config_network:%d\r\n",config_network);
	if (config_network) {
		esp_restart();
		for(;;){

		}
	}
}

void esp_restore_factory_setting(void)
{
	const esp_partition_t *p = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, NULL);
	if (p == NULL) {
		return;
	}

	esp_partition_erase_range(p, 0, p->size);
	esp_restart();
	for(;;){

	}
}

/**
 * brief: 
 *
 * @Returns: 
 */
int
joylink_dev_get_user_mac(char *out)
{
	/**
	*FIXME:must to do
	*/

    // 伪造一个MAC，因为需要一机一密
    uint8_t buffer[18] = {0};
    uint8_t tesp_buffer[18] = {0};

    memcpy(tesp_buffer, CONFIG_JOYLINK_DEVICE_MAC, strlen(CONFIG_JOYLINK_DEVICE_MAC));

    buffer[0] = tesp_buffer[0];
    buffer[1] = tesp_buffer[1];
    buffer[2] = ':';

    buffer[3] = tesp_buffer[2];
    buffer[4] = tesp_buffer[3];
    buffer[5] = ':';

    buffer[6] = tesp_buffer[4];
    buffer[7] = tesp_buffer[5];
    buffer[8] = ':';

    buffer[9] = tesp_buffer[6];
    buffer[10] = tesp_buffer[7];
    buffer[11] = ':';

    buffer[12] = tesp_buffer[8];
    buffer[13] = tesp_buffer[9];
    buffer[14] = ':';

    buffer[15] = tesp_buffer[10];
    buffer[16] = tesp_buffer[11];
    buffer[17] = 0x0;

    for(int i = 0; i < 17; i++){
        _g_pLightMgr->jlp.mac[i] = buffer[i];
    }

    memcpy(out, _g_pLightMgr->jlp.mac, strlen(JLP_MAC));
    return 0;
}

/**
 * brief: 
 *
 * @Returns: 
 */
int
joylink_dev_get_private_key(char *out)
{
	/**
	*FIXME:must to do
	*/
    memcpy(out, JLP_PRIVATE_KEY, strlen(JLP_PRIVATE_KEY));

	return 0;
}

/**
 * brief: 
 *
 * @Returns: 
 */
int
joylink_dev_user_data_set(char *cmd, user_dev_status_t *user_data)
{
	/**
	*FIXME:must to do
	*/
    log_debug("--cmd:%s, ", cmd);

	return 0;
}
