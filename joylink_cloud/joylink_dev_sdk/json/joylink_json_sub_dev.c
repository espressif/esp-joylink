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
#include "cJSON.h"

#include "joylink.h"
#include "joylink_json.h"
#include "joylink_log.h"

#define JL_SUB_DEV_MAX  (10)

/**
 * brief: 
 *
 * @Param: de
 * @Param: num
 * @Param: out
 *
 * @Returns: 
 */
int 
joylink_packet_sub_add_rsp(const JLSubDevData_t *dev, int num, char *out)
{
    if(NULL == dev || NULL == out 
            || num < 0 || num > JL_SUB_DEV_MAX){
        return 0;
    }
    cJSON *root;
    int len = 0, i = 0;
    char *js_out;

    root = cJSON_CreateObject();
    if(NULL == root){
        goto RET;
    }

    cJSON_AddNumberToObject(root, "code", dev[i].state);
    cJSON_AddStringToObject(root, "msg", "success");
    cJSON_AddStringToObject(root, "mac", dev[i].mac);
    cJSON_AddStringToObject(root, "productuuid", dev[i].uuid);

    js_out=cJSON_Print(root);  
    len = strlen(js_out);
    time_t c_t = time(NULL);

    memcpy(out, &c_t, 4);
    memcpy(out + 4, js_out, len);
    len += 4;

    cJSON_Delete(root); 
    free(js_out);
RET:
    return len;
}

/**
 * brief: 
 *
 * @Param: pMsg
 * @Param: out_num
 *
 * @Returns: 
 */
JLSubDevData_t *
joylink_parse_sub_add(const uint8_t* pMsg, int* out_num)
{
    JLSubDevData_t * ret = NULL;
    if(NULL == pMsg || NULL == out_num){
        printf("--->:ERROR: pMsg is NULL\n");
        return ret;
    }
    int count = 0;
    cJSON *pSub;
    cJSON * pJson = cJSON_Parse((char *)pMsg);

    if(NULL == pJson){
        printf("--->:cJSON_Parse is error:%s\n", pMsg);
      return ret;
    }

    JLSubDevData_t * devs = (JLSubDevData_t*)malloc(sizeof(JLSubDevData_t) * JL_SUB_DEV_MAX);
    if(NULL == devs){
        return ret;
    }

    bzero(devs, sizeof(JLSubDevData_t) * JL_SUB_DEV_MAX);
    cJSON *pData = cJSON_GetObjectItem(pJson, "data");
    if(NULL != pData){
        pSub = cJSON_GetObjectItem(pData, "mac");
        if(NULL != pSub){
            strcpy(devs[count].mac, pSub->valuestring);
        }
        pSub = cJSON_GetObjectItem(pData, "productuuid");
        if(NULL != pSub){
            strcpy(devs[count].uuid, pSub->valuestring);
        }
        pSub = cJSON_GetObjectItem(pData, "protocol");
        if(NULL != pSub){
            devs[count].protocol = pSub->valueint;
        }
        count++;
    }                                                                                                         
    ret = devs;
    *out_num = count;

    cJSON_Delete(pJson);

    return ret;
}

/**
 * brief: 
 *
 * @Param: pMsg
 * @Param: dev
 *
 * @Returns: 
 */
int
joylink_parse_sub_auth(const uint8_t* pMsg, JLSubDevData_t *dev)
{
    int ret = E_RET_ERROR;
    if(NULL == pMsg || NULL == dev){
        log_error("--->:ERROR: pMsg is NULL\n");
        return ret;
    }
    cJSON * pSub;
    cJSON * pJson = cJSON_Parse((char *)pMsg);

    if(NULL == pJson){
        log_error("--->:cJSON_Parse is error:%s\n", pMsg);
      return ret;
    }

    cJSON *jdev= cJSON_GetObjectItem(pJson, "data");
    if(NULL != jdev){
        pSub = cJSON_GetObjectItem(jdev, "feedid");
        if(NULL != pSub){
            strcpy(dev->feedid, pSub->valuestring);
        }
        pSub = cJSON_GetObjectItem(jdev, "accesskey");
        if(NULL != pSub){
            strcpy(dev->accesskey, pSub->valuestring);
        }
        pSub = cJSON_GetObjectItem(jdev, "localkey");
        if(NULL != pSub){
            strcpy(dev->localkey, pSub->valuestring);
        }
        pSub = cJSON_GetObjectItem(jdev, "mac");
        if(NULL != pSub){
            strcpy(dev->mac, pSub->valuestring);
        }
        pSub = cJSON_GetObjectItem(jdev, "productuuid");
        if(NULL != pSub){
            strcpy(dev->uuid, pSub->valuestring);
        }
        pSub = cJSON_GetObjectItem(jdev, "cloudAuthValue");
        if(NULL != pSub){
	    memset(dev->cloudAuthValue, 0, sizeof(dev->cloudAuthValue));
	    if(pSub->valuestring != NULL){
            	strcpy(dev->cloudAuthValue, pSub->valuestring);
	    }
        }
	pSub = cJSON_GetObjectItem(jdev, "lancon");
        if(NULL != pSub){
            dev->lancon = pSub->valueint;
        }
        pSub = cJSON_GetObjectItem(jdev, "trantype");
        if(NULL != pSub){
            dev->cmd_tran_type = pSub->valueint;
        }
    }

    cJSON_Delete(pJson);
    ret = E_RET_OK;

    return ret;
}

/**
 * brief: 
 *
 * @Param: de
 * @Param: out
 *
 * @Returns: 
 */
int 
joylink_packet_sub_auth_rsp(const JLSubDevData_t *dev, char *out)
{
    if(NULL == dev || NULL == out ){
        return 0;
    }
    cJSON *root;
    int len = 0;
    char *js_out = NULL;

    root = cJSON_CreateObject();
    if(NULL == root){
        goto RET;
    }

    if(dev->state == 0){
    	cJSON_AddNumberToObject(root, "code", 0);
        cJSON_AddStringToObject(root, "msg", "success");
    }
    else if(dev->state == 3){
        cJSON_AddNumberToObject(root, "code", -1);
        cJSON_AddStringToObject(root, "msg", "failed");
    }
    cJSON_AddStringToObject(root, "mac", dev->mac);
    cJSON_AddStringToObject(root, "productuuid", dev->uuid);

    js_out=cJSON_Print(root);  
    len = strlen(js_out);

    memcpy(out , js_out, len);

    cJSON_Delete(root); 
    if(js_out != NULL){
        free(js_out);
    }
RET:
    return len;
}

/**
 * brief: 
 *
 * @Param: sdev
 * @Param: count
 *
 * @Returns: 
 */
char * 
joylink_package_subdev(JLSubDevData_t *sdev, int count)
{
    if(NULL == sdev){
        return NULL;
    }
    cJSON *arrary; 
    char *out = NULL; 
    unsigned int i; 
   
    cJSON **js_devs = (cJSON **)malloc(sizeof(cJSON *) * count); 
    if(js_devs == NULL){
        goto RET;
    }
    if(NULL == (arrary = cJSON_CreateArray())){
        free(js_devs);
        goto RET;
    }

    for(i = 0; i < count; i ++){
        js_devs[i] =cJSON_CreateObject();
        if(NULL != js_devs[i]){
            cJSON_AddItemToArray(arrary, js_devs[i]);
            cJSON_AddNumberToObject(js_devs[i], "state", sdev[i].state);
            cJSON_AddNumberToObject(js_devs[i], "protocol", sdev[i].protocol);
            cJSON_AddStringToObject(js_devs[i], "mac", sdev[i].mac);
            cJSON_AddStringToObject(js_devs[i], "productuuid", sdev[i].uuid);
            cJSON_AddNumberToObject(js_devs[i], "lancon", sdev[i].lancon);
            cJSON_AddNumberToObject(js_devs[i], "trantype", sdev[i].cmd_tran_type);
            cJSON_AddStringToObject(js_devs[i], "feedid", sdev[i].feedid);
            cJSON_AddStringToObject(js_devs[i], "devkey", sdev[i].pubkey);
	    cJSON_AddStringToObject(js_devs[i], "devAuthValue", sdev[i].devAuthValue);
	    cJSON_AddNumberToObject(js_devs[i], "subNoSnapshot", sdev[i].subNoSnapshot);
        }
    }

    out=cJSON_Print(arrary);  
    cJSON_Delete(arrary);
RET:
    return out;
}
