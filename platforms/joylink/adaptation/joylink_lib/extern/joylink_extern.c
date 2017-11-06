/* --------------------------------------------------
 * @brief: 
 *
 * @version: 1.0
 *
 * @date: 10/08/2015 09:28:27 AM
 *
 * --------------------------------------------------
 */
#include "esp_common.h"
#include "freertos/semphr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "joylink.h"
#include "joylink_packets.h"
#include "joylink_extern.h"
#include "joylink_json.h"
#include "joylink_extern_json.h"
#include "crc.h"
#include "joylink_log.h"
#include "esp_joylink.h"
#include "esp_info_store.h"

extern void joylink_dev_set_ver(short ver);
extern short joylink_dev_get_ver();
extern void joylink_wait_station_got_ip(void);

E_JLRetCode_t
joylink_dev_is_net_ok()
{
    joylink_wait_station_got_ip();
    return E_RET_OK;
}

static uint8_t server_st = 0;

E_JLRetCode_t
joylink_dev_set_connect_st(int st)
{
    if (st == JL_SERVER_ST_WORK) {

        if (server_st == 0) {
			server_st = 1;
    		joylink_event_send(JOYLINK_EVENT_CLOUD_CONNECTED, NULL);
		}
    } else {

        if (server_st == 1) {
			server_st = 0;
			joylink_event_send(JOYLINK_EVENT_CLOUD_DISCONNECTED, NULL);
    	}
	}

    return E_RET_OK;
}

E_JLRetCode_t
joylink_dev_set_attr_jlp(JLPInfo_t *jlp)
{
    JL_PARAM_CHECK(!jlp);

    log_debug("save joylink config into flash");

    if (esp_info_save(JOYLINK_CONFIG_KEY, (void*)jlp, sizeof(JLPInfo_t)) <= 0) {
        log_error("save joylink config into flash");
        return E_RET_ERROR;
    }

    return E_RET_OK;
}

E_JLRetCode_t
joylink_dev_get_jlp_info(JLPInfo_t *jlp)
{
    JL_PARAM_CHECK(!jlp);

    log_debug("read joylink config from flash");
    JLPInfo_t jlp_tmp;

    if (esp_info_load(JOYLINK_CONFIG_KEY, &jlp_tmp, sizeof(JLPInfo_t)) <= 0) {
        log_error("read joylink config from flash error");
        return E_RET_ERROR;
    }
    
    memcpy(jlp, &jlp_tmp, sizeof(JLPInfo_t));
    return E_RET_OK;
}

int
joylink_dev_get_snap_shot(char *out_snap, int32_t out_max, uint8_t to_server)
{
    JL_PARAM_CHECK(!out_snap);
    JL_PARAM_CHECK(out_max <= 0);

#ifdef JOYLINK_PASSTHROUGH

    if (E_RET_OK != joylink_get_status_cmd_up(out_max, 1, 0)) {
        goto ERR;
    }

#else

    if (E_RET_OK != joylink_get_status_cmd_up(out_max, 1, 1)) {
        goto ERR;
    }

#endif

    int len = joylink_get_dev_json_status(out_snap, out_max);

    if (len <= 0) {
        goto ERR;
    }

    log_debug("get snap shot success, out_snap: %s", out_snap);
    return len;

ERR:
    sprintf(out_snap, "{\"code\":1, \"msg\":\"code err or ctrl timeout\"}");
    log_error("get snap shot fail, out_snap:%s", out_snap);
    return strlen(out_snap);
}

E_JLRetCode_t 
joylink_dev_script_ctrl(const char *recPainText, JLContrl_t *ctr, int from_server)
{
    JL_PARAM_CHECK(!recPainText);
    JL_PARAM_CHECK(!ctr);
    
    int ret = E_RET_ERROR;
    ctr->biz_code = (int)(*((int *)(recPainText + 4)));
    ctr->serial = (int)(*((int *)(recPainText +8)));

    log_info("biz code:%d, server:%d", ctr->biz_code, from_server);

    if (ctr->biz_code == JL_BZCODE_GET_SNAPSHOT) {
        /* Nothing to do! */
        log_info("nothing to do");
        ret = E_RET_OK;
    } else if (ctr->biz_code == JL_BZCODE_CTRL) {

#ifdef JOYLINK_PASSTHROUGH
        ret = joylink_ctrl_cmd_down(recPainText + 12, from_server, 0);
#else
        ret = joylink_ctrl_cmd_down(recPainText + 12, from_server, 1);
#endif

    } else {
        log_error("unKown biz_code:%d", ctr->biz_code);
        ret = E_RET_ERROR;
    }
    
    return ret;
}

int
joylink_dev_get_json_snap_shot(char *out_snap, int32_t out_max, int code, char *data)
{   
    JL_PARAM_CHECK(!out_snap);
    JL_PARAM_CHECK(out_max <= 0);

    if(E_RET_OK != code) {
        goto ERR;
    }

    if (E_RET_OK != joylink_get_status_cmd_up(out_max, 0, 1)) {
        goto ERR;
    }

	int len = joylink_get_dev_json_status(out_snap, out_max);

	if (len <= 0) {
        goto ERR;
    }

    log_debug("get snap shot success, out_snap: %s", out_snap);
    return len;

ERR:
	if (data) {
        sprintf(out_snap, "{\"code\":1, \"feedid\":\"%s\", \"msg\":\"code err or ctrl timeout\"}", data);
    } else {
        sprintf(out_snap, "{\"code\":1, \"msg\":\"code err or ctrl timeout\"}");
    }

    log_error("get snap shot fail, out_snap:%s", out_snap);
    return strlen(out_snap);
}

E_JLRetCode_t 
joylink_dev_lan_json_ctrl(const char *json_cmd)
{
    JL_PARAM_CHECK(!json_cmd);

    E_JLRetCode_t ret = E_RET_ERROR;
    char *pstr = NULL;
    char *ptail = NULL;
    char cmd[3] = {0};
    int cmd_type = 0;

    /* parse json to get cmd type */
    pstr = strstr(json_cmd, "\"cmd\":");
    if (pstr) {
        ptail = strstr(pstr, ",");
        if (ptail) {
            memcpy(cmd, pstr+6, ptail-pstr-6);
            cmd_type = atoi(cmd);
        }
    }
    
    if (cmd_type == JL_JSON_CMD_GET) {
        /* get json snap shot, so nothing to do */
        log_info("nothing to do");
        ret = E_RET_OK;
    } else if (cmd_type == JL_JSON_CMD_CTRL) {
        ret = joylink_ctrl_cmd_down(json_cmd, 0, 1);
    } else {
        log_error("other cmd type");
    }
    
	log_debug("json ctrl:%s", json_cmd);
    return ret;
}

E_JLRetCode_t
joylink_dev_ota(JLOtaOrder_t *otaOrder)
{
    JL_PARAM_CHECK(!otaOrder);

    int ret = E_RET_ERROR;
    log_debug("serial:%d | feedid:%s | productuuid:%s | version:%d | versionname:%s | crc32:%d | url:%s\n",
        otaOrder->serial, otaOrder->feedid, otaOrder->productuuid, otaOrder->version,
        otaOrder->versionname, otaOrder->crc32, otaOrder->url);

    extern joylink_upgrade_start(const char * url);
    ret = joylink_upgrade_start((const char*)otaOrder->url);

    if (E_RET_OK == ret) {
        joylink_event_param_t *param;
        param = (joylink_event_param_t *)malloc(sizeof(joylink_event_param_t));
        param->ota.version = otaOrder->version;
        strcpy(param->ota.url, otaOrder->url);
        joylink_event_send(JOYLINK_EVENT_OTA_START, param);
    }

    return ret;
}

xSemaphoreHandle xsemOtaUpload = NULL;

int joylink_ota_upload_rsp_wait(uint16_t time_ms)
{
    if (NULL == xsemOtaUpload) {
        xsemOtaUpload = (xSemaphoreHandle)xSemaphoreCreateBinary();
    }
    
    if (pdTRUE == xSemaphoreTake(xsemOtaUpload, time_ms / portTICK_RATE_MS)) {
        return E_RET_OK;
    } else {
        return E_RET_ERROR;
    }
}

void joylink_ota_upload_rsp_ok()
{
    if (NULL == xsemOtaUpload) {
        xsemOtaUpload = (xSemaphoreHandle)xSemaphoreCreateBinary();
    }
    xSemaphoreGive(xsemOtaUpload);
}

void
joylink_dev_ota_status_upload(int status, int process, char *status_desc, uint8_t is_block)
{
    log_debug("ota process %d, st %d, desc %s", process, status, status_desc);
    JLOtaUpload_t otaUpload;
    strcpy(otaUpload.feedid, _g_pdev->jlp.feedid);
    strcpy(otaUpload.productuuid, _g_pdev->jlp.uuid);
    otaUpload.status = status;
    otaUpload.progress = process;
    strcpy(otaUpload.status_desc, status_desc);
    joylink_server_ota_status_upload_req(&otaUpload);

    if(is_block) {
        while (1) {

            if(joylink_ota_upload_rsp_wait(2000) == E_RET_OK) {
                log_debug("ota upload rsp ok");
                break;
            } else {
                joylink_server_ota_status_upload_req(&otaUpload);
            }
        }
    }
}

