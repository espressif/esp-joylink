/* --------------------------------------------------
 * @brief: 
 *
 * @version: 1.0
 *
 * @date: 08/01/2018
 *
 * @author: 
 * --------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"

#include "joylink_log.h"
#include "joylink_extern.h"
#include "joylink_memory.h"
#include "joylink_socket.h"
#include "joylink_string.h"
#include "joylink_stdio.h"
#include "joylink_stdint.h"
#include "joylink_time.h"

/**
 * brief: 
 *
 * @Param: pMsg
 * @Param: user_dev
 *
 * @Returns: 
 */
int 
joylink_dev_parse_ctrl(const char *pMsg, user_dev_status_t *userDev)
{
	int ret = -1;
	if(NULL == pMsg || NULL == userDev){
		goto RET;
	}
	log_debug("json_org:%s", pMsg);
	cJSON * pSub;
	cJSON * pJson = cJSON_Parse(pMsg);

	if(NULL == pJson){
		log_error("--->:ERROR: pMsg is NULL\n");
		goto RET;
	}

	char tmp_str[64];
	cJSON *pStreams = cJSON_GetObjectItem(pJson, "streams");
	if(NULL != pStreams){
		int iSize = cJSON_GetArraySize(pStreams);
  		int iCnt;
		for( iCnt = 0; iCnt < iSize; iCnt++){
			pSub = cJSON_GetArrayItem(pStreams, iCnt);
			if(NULL == pSub){
				continue;
			}

			cJSON *pSId = cJSON_GetObjectItem(pSub, "stream_id");
			if(NULL == pSId){
				break;
			}
			cJSON *pV = cJSON_GetObjectItem(pSub, "current_value");
			if(NULL == pV){
				continue;
			}
		
			if(!jl_platform_strcmp(USER_DATA_POWER, pSId->valuestring)){
			    jl_platform_memset(tmp_str, 0, sizeof(tmp_str));
				jl_platform_strcpy(tmp_str, pV->valuestring);
				userDev->Power = atoi(tmp_str);
			    joylink_dev_user_data_set( USER_DATA_POWER,userDev);
			} else if(!jl_platform_strcmp(USER_DATA_AMBIENTLIGHT, pSId->valuestring)) {
				jl_platform_memset(tmp_str, 0, sizeof(tmp_str));
				jl_platform_strcpy(tmp_str, pV->valuestring);
				userDev->AmbientLight = atoi(tmp_str);
			    joylink_dev_user_data_set(USER_DATA_AMBIENTLIGHT, userDev);
			}

			char *dout = cJSON_PrintUnformatted(pSub);
			if(NULL != dout){
				log_debug("org streams:%s", dout);
				jl_platform_free(dout);
			}
		}
	}                                                                                                         
	cJSON_Delete(pJson);
RET:
	return ret;
}

/**
 * brief: 
 * NOTE: If return is not NULL, 
 * must free it, after use.
 *
 * @Param: retMsg
 * @Param: retCode
 * @Param: wci
 * @Param: devlist
 *
 * @Returns: char * 
 */
char * 
joylink_dev_package_info(const int retCode, user_dev_status_t *userDev)
{
	if(NULL == userDev){
		return NULL;
	}
	cJSON *root, *arrary;
	char *out  = NULL; 

	root = cJSON_CreateObject();
	if(NULL == root){
		goto RET;
	}
	arrary = cJSON_CreateArray();
	if(NULL == arrary){
		cJSON_Delete(root);
		goto RET;
	}
	cJSON_AddNumberToObject(root, "code", retCode);
	cJSON_AddItemToObject(root, "streams", arrary);

	char i2str[64];
	cJSON *data_Power = cJSON_CreateObject();
	cJSON_AddItemToArray(arrary, data_Power);
	cJSON_AddStringToObject(data_Power, "stream_id", "Power");
	jl_platform_memset(i2str, 0, sizeof(i2str));
	jl_platform_sprintf(i2str, "%d", userDev->Power);
	cJSON_AddStringToObject(data_Power, "current_value", i2str);

	out=cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
RET:
	return out;
}

/**
 * brief: 
 *
 * @Param: retCode
 * @Param: userDev
 *
 * @Returns: 
 */
char * 
joylink_dev_modelcode_info(const int retCode, JLPInfo_t *userDev)
{
	if(NULL == userDev){
		return NULL;
	}
	cJSON *root, *arrary;
	char *out  = NULL; 

	root = cJSON_CreateObject();
	if(NULL == root){
		goto RET;
	}
	arrary = cJSON_CreateArray();
	if(NULL == arrary){
		cJSON_Delete(root);
		goto RET;
	}
	cJSON_AddItemToObject(root, "model_codes", arrary);
    
	char i2str[32];
	jl_platform_memset(i2str, 0, sizeof(i2str));
	cJSON *element = cJSON_CreateObject();
	cJSON_AddItemToArray(arrary, element);
	cJSON_AddStringToObject(element, "feedid", userDev->feedid);
	cJSON_AddStringToObject(element, "model_code", "12345678123456781234567812345678");

	out=cJSON_PrintUnformatted(root);  
	cJSON_Delete(root); 
RET:
	return out;
}
/**
 * brief: 
 *
 * @Param: jlp
 * @Param: pMsg
 *
 * @Returns: 
 */
int 
joylink_parse_jlp(JLPInfo_t *jlp, char * pMsg)
{
	int ret = -1;
	if(NULL == pMsg || NULL == jlp){
		return ret;
	}
	cJSON *pVal;
	cJSON * pJson = cJSON_Parse(pMsg);

	if(NULL == pJson){
		log_error("--->:ERROR: pMsg is NULL\n");
		goto RET;
	}

	pVal = cJSON_GetObjectItem(pJson, "uuid");
	if(NULL != pVal){
		jl_platform_strcpy(jlp->uuid, pVal->valuestring);
	}

	pVal = cJSON_GetObjectItem(pJson, "feedid");
	if(NULL != pVal){
		jl_platform_strcpy(jlp->feedid, pVal->valuestring);
	}

	pVal = cJSON_GetObjectItem(pJson, "accesskey");
	if(NULL != pVal){
		jl_platform_strcpy(jlp->accesskey, pVal->valuestring);
	}

	pVal = cJSON_GetObjectItem(pJson, "localkey");
	if(NULL != pVal){
		jl_platform_strcpy(jlp->localkey, pVal->valuestring);
	}

	pVal = cJSON_GetObjectItem(pJson, "version");
	if(NULL != pVal){
		jlp->version = pVal->valueint;
	}

	cJSON_Delete(pJson);
	ret = 0;
RET:
	return ret;
}
