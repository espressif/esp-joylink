/* --------------------------------------------------
 * @brief: 
 *
 * @version: 1.0
 *
 *
 * @author: 
 * --------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"

#include "joylink_json.h"
#include "joylink_agent.h"
#include "joylink_agent_devs.h"
#include "joylink_agent_json.h"
#include "joylink_log.h"

extern AgentDev_t _g_adevs[AGENT_DEV_MAX];

extern int 
joylink_util_byte2hexstr(const uint8_t *pBytes, int srcLen, uint8_t *pDstStr, int dstLen);

/**
 * brief: 
 *
 * @Param: pMsg
 * @Param: out_num
 *
 * @Returns: 
 */
JLAddAgentDev_t*
joylink_agent_parse_dev_add(const uint8_t* pMsg, int* out_num)
{
    if(NULL == pMsg || NULL == out_num){
        log_error("--->:ERROR: pMsg is NULL\n");
        return NULL;
    }
    int32_t count;
    cJSON * pSub;
    cJSON * pJson = NULL;
    cJSON * pRoot = cJSON_Parse((char *)pMsg);
    JLAddAgentDev_t* devs = NULL;

    if(NULL == pRoot){
        log_error("--->:cJSON_Parse is error:%s\n", pMsg);
        goto ERROR;
    }

    pJson = cJSON_GetObjectItem(pRoot, "list");
    count = cJSON_GetArraySize(pJson);
    devs = (JLAddAgentDev_t*)malloc(sizeof(JLAddAgentDev_t) * count);

    if(NULL == devs){
        log_error("--->:malloc error\n");
        goto ERROR;
    }
    memset(devs, 0, sizeof(JLAddAgentDev_t) * count);

    int i;
    for( i = 0; i < count; i++){
        pSub = cJSON_GetArrayItem(pJson, i);
        if(NULL == pSub){
            continue;
        }
        cJSON *pData= cJSON_GetObjectItem(pSub, "feedid");
        if(NULL == pData){
            continue;
        }
		strcpy((char*)(devs[i].feedid), pData->valuestring);
        pData = cJSON_GetObjectItem(pSub, "ackey");
        if(NULL == pData){
            continue;
        }
		strcpy((char*)(devs[i].ackey), pData->valuestring);
    }

    *out_num = i;
ERROR:
    if(NULL != pJson){
        cJSON_Delete(pJson);
    }
    return devs;
}

/**
 * brief: 
 *
 * @Param: pMsg
 * @Param: out_num
 *
 * @Returns: 
 */
JLAddAgentDev_t*
joylink_agent_parse_dev_del(const uint8_t* pMsg, int* out_num)
{
    if(NULL == pMsg || NULL == out_num){
        log_error("--->:ERROR: pMsg is NULL\n");
        return NULL;
    }
    int32_t count;
    cJSON * pSub;
    cJSON * pJson = NULL;
    cJSON * pRoot = cJSON_Parse((char *)pMsg);
    JLAddAgentDev_t* devs = NULL;

    if(NULL == pRoot){
        log_error("--->:cJSON_Parse is error:%s\n", pMsg);
        goto ERROR;
    }

    pJson = cJSON_GetObjectItem(pRoot, "list");
    if(NULL == pJson){
        log_error("--->:cJSON_Parse is error:%s\n", pMsg);
        goto ERROR;
    }
    count = cJSON_GetArraySize(pJson);
    devs = (JLAddAgentDev_t*)malloc(sizeof(JLAddAgentDev_t) * count);
    if(NULL == devs){
        log_error("--->:malloc error\n");
        goto ERROR;
    }
    memset(devs, 0, sizeof(JLAddAgentDev_t) * count);
    int i;
    if(count > 0){
        for(i = 0; i < count; i++){ 
            cJSON *pv;
            pv = cJSON_GetArrayItem(pJson, i);
            if(NULL != pv){
                strcpy((char*)(devs[i].feedid), pv->valuestring);
            }
        }
    }
    *out_num = i;
ERROR:
    if(NULL != pRoot){
        cJSON_Delete(pRoot);
    }
    return devs;
}

/**
 * brief: 
 *
 * @Returns: 
 */
char * 
joylink_agent_create_cloud_auth_json_req_f()
{
    cJSON *arrary; 
    unsigned int i; 
    int j = 0;
    char *out = NULL; 
    char auth_str[128];
  
    int count = AGENT_DEV_MAX;
    cJSON **js_devs = (cJSON **)malloc(sizeof(cJSON *) * count); 
    if(js_devs == NULL){
        goto RET;
    }
    if(NULL == (arrary = cJSON_CreateArray())){
        free(js_devs);
        goto RET;
    }

    for(i = 0; i < count; i ++){
        if(!IS_AGENT_AUTH_SET(_g_adevs[i].agent_st)){
            continue;
        }
        js_devs[j] =cJSON_CreateObject();
        if(NULL != js_devs[j]){
            cJSON_AddItemToArray(arrary, js_devs[j]);
            cJSON_AddStringToObject(js_devs[j], "feedid", _g_adevs[i].feedid);
            cJSON_AddNumberToObject(js_devs[j], "random", _g_adevs[i].random);

            memset(auth_str, 0, sizeof(auth_str));
            joylink_util_byte2hexstr((uint8_t*)&_g_adevs[j].auth, sizeof(_g_adevs[i].auth), 
                        (uint8_t*)auth_str, sizeof(auth_str));
            cJSON_AddStringToObject(js_devs[j], "cloud_auth_hexstr", auth_str);
        }
        ST_AGENT_AUTH_CLR(_g_adevs[i].agent_st);
        j++;
    }

    if(j > 0){
        out=cJSON_Print(arrary);  
        log_debug("------ json len:%d:%s", (int)strlen(out),  out);
    }
    cJSON_Delete(arrary);
    free(js_devs);
RET:
    return out;
}

/**
 * brief: 
 *
 * @Returns: 
 */
char * 
joylink_agent_create_cloud_hb_json_req()
{
    cJSON *arrary; 
    unsigned int i; 
    int j = 0;
    char *out = NULL; 
    char auth_str[128];
  
    int count = AGENT_DEV_MAX;
    cJSON **js_devs = (cJSON **)malloc(sizeof(cJSON *) * count); 
    if(js_devs == NULL){
        goto RET;
    }
    if(NULL == (arrary = cJSON_CreateArray())){
        free(js_devs);
        goto RET;
    }

    for(i = 0; i < count; i ++){
        if(!IS_AGENT_HB_SET(_g_adevs[i].agent_st)){
            continue;
        }
        js_devs[j] =cJSON_CreateObject();
        if(NULL != js_devs[j]){
            cJSON_AddItemToArray(arrary, js_devs[j]);
            cJSON_AddStringToObject(js_devs[j], "feedid", _g_adevs[i].feedid);
            cJSON_AddNumberToObject(js_devs[j], "version", _g_adevs[i].hb.verion);
            cJSON_AddNumberToObject(js_devs[j], "rssi", _g_adevs[i].hb.rssi);
        }
        ST_AGENT_HB_CLR(_g_adevs[i].agent_st);
        j++;
    }

    if(j > 0){
        out=cJSON_Print(arrary);  
        log_debug("------ json len:%d:%s", (int)strlen(out),  out);
    }

    cJSON_Delete(arrary);
    free(js_devs);
RET:
    return (char*)out;
}

/**
 * brief: 
 *
 * @Returns: 
 */
char * 
joylink_agent_create_cloud_snap_json_req()
{
    cJSON *arrary; 
    unsigned int i; 
    int j = 0;
    char *out = NULL; 
    char auth_str[128];
  
    int count = AGENT_DEV_MAX;
    cJSON **js_devs = (cJSON **)malloc(sizeof(cJSON *) * count); 
    if(js_devs == NULL){
        goto RET;
    }
    if(NULL == (arrary = cJSON_CreateArray())){
        free(js_devs);
        goto RET;
    }

    for(i = 0; i < count; i ++){
        if(!IS_AGENT_SNAP_SET(_g_adevs[i].agent_st)){
            continue;
        }
        js_devs[j] =cJSON_CreateObject();
        if(NULL != js_devs[j]){
            cJSON_AddItemToArray(arrary, js_devs[j]);
            cJSON_AddStringToObject(js_devs[j], "feedid", _g_adevs[i].feedid);
            cJSON_AddStringToObject(js_devs[j], "snap_hex_str", _g_adevs[i].snap);
            char tb[1024] = {0};

            joylink_util_hexStr2bytes(_g_adevs[i].snap, 
                    tb, sizeof(tb));

            log_info("SNAP:%s", tb + 4);
        }
        ST_AGENT_SNAP_CLR(_g_adevs[i].agent_st);
        j++;
    }

    if(j > 0){
        out=cJSON_Print(arrary);  
        log_debug("------ json len:%d:%s", (int)strlen(out),  out);
    }
    cJSON_Delete(arrary);
    free(js_devs);
RET:
    return out;
}

extern void suffix_object(cJSON *prev,cJSON *item);
/**
 * brief: 
 *
 * @Returns: 
 */
char * 
joylink_agent_json_packet_dev_list(int32_t ret_code, char* ret_msg)
{
    cJSON *arrary; 
    unsigned int i; 
    int j = 0;
    char *out = NULL; 
    char auth_str[128];
    int count = AGENT_DEV_MAX;

    cJSON *root = NULL;

    root = cJSON_CreateObject();
    if(NULL == root){
        goto RET;
    }

    if(NULL == (arrary = cJSON_CreateArray())){
        goto RET;
    }

    cJSON_AddNumberToObject(root, "code", ret_code);
    cJSON_AddStringToObject(root, "msg", ret_msg); 
    cJSON_AddItemToObject(root,"list", arrary);

    char fs[AGENT_DEV_MAX][33];
    for(i = 0; i < count; i ++){
        if(!strcmp(_g_adevs[i].feedid, "")){
            continue;
        }
        memset(fs[i], 0, 33);
        strcpy(fs[j], _g_adevs[i].feedid);
        j++;
    }
    cJSON *n = NULL, *p = NULL;
    for(i=0; i<j ;i++){
        n=cJSON_CreateString(fs[i]);
        if(!i)arrary->child=n;else suffix_object(p,n);
        p=n;
    }

    out=cJSON_Print(root);  
    cJSON_Delete(root);
    log_debug("------ json len:%d:%s", (int)strlen(out),  out);
RET:
    return out;
}
