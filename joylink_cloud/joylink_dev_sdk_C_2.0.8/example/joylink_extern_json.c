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

#include "joylink_json.h"
#include "joylink_dev.h"
#include "joylink_log.h"
#include "joylink_extern.h"

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

			if(!strcmp(USER_DATA_POWER, pSId->valuestring)){
				memset(tmp_str, 0, sizeof(tmp_str));
				strcpy(tmp_str, pV->valuestring);
				userDev->power = atoi(tmp_str);
			}
			if(!strcmp(USER_DATA_STATUS, pSId->valuestring)){
				memset(userDev->status, 0, sizeof(userDev->status));
				strcpy(userDev->status, pV->valuestring);
			}
			if(!strcmp(USER_DATA_TEMPERATURE, pSId->valuestring)){
				memset(tmp_str, 0, sizeof(tmp_str));
				strcpy(tmp_str, pV->valuestring);
				userDev->temperature = atof(tmp_str);
			}
			if(!strcmp(USER_DATA_TEMP, pSId->valuestring)){
				memset(tmp_str, 0, sizeof(tmp_str));
				strcpy(tmp_str, pV->valuestring);
				userDev->temp = atof(tmp_str);
			}

			char *dout = cJSON_Print(pSub);
			if(NULL != dout){
				log_debug("org streams:%s", dout);
				free(dout);
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

	char i2str[32];
	cJSON *data_power = cJSON_CreateObject();
	cJSON_AddItemToArray(arrary, data_power);
	cJSON_AddStringToObject(data_power, "stream_id", "power");
	memset(i2str, 0, sizeof(i2str));
	sprintf(i2str, "%d", userDev->power);
	cJSON_AddStringToObject(data_power, "current_value", i2str);

	cJSON *data_status = cJSON_CreateObject();
	cJSON_AddItemToArray(arrary, data_status);
	cJSON_AddStringToObject(data_status, "stream_id", "status");
	cJSON_AddStringToObject(data_status, "current_value", userDev->status);

	cJSON *data_temperature = cJSON_CreateObject();
	cJSON_AddItemToArray(arrary, data_temperature);
	cJSON_AddStringToObject(data_temperature, "stream_id", "temperature");
	memset(i2str, 0, sizeof(i2str));
	sprintf(i2str, "%f", userDev->temperature);
	cJSON_AddStringToObject(data_temperature, "current_value", i2str);

	cJSON *data_temp = cJSON_CreateObject();
	cJSON_AddItemToArray(arrary, data_temp);
	cJSON_AddStringToObject(data_temp, "stream_id", "temp");
	memset(i2str, 0, sizeof(i2str));
	sprintf(i2str, "%f", userDev->temp);
	cJSON_AddStringToObject(data_temp, "current_value", i2str);


	out=cJSON_Print(root);
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
joylink_dev_modelcode_info(const int retCode, user_dev_status_t *userDev)
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
	bzero(i2str, sizeof(i2str));
	cJSON *element = cJSON_CreateObject();
	cJSON_AddItemToArray(arrary, element);
	cJSON_AddStringToObject(element, "feedid", "247828880060773075");
	//sprintf(i2str, "%d", pCtrl->para_value);
	cJSON_AddStringToObject(element, "model_code", "12345678123456781234567812345678");

	out=cJSON_Print(root);  
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
		strcpy(jlp->uuid, pVal->valuestring);
	}

	pVal = cJSON_GetObjectItem(pJson, "feedid");
	if(NULL != pVal){
		strcpy(jlp->feedid, pVal->valuestring);
	}

	pVal = cJSON_GetObjectItem(pJson, "accesskey");
	if(NULL != pVal){
		strcpy(jlp->accesskey, pVal->valuestring);
	}

	pVal = cJSON_GetObjectItem(pJson, "localkey");
	if(NULL != pVal){
		strcpy(jlp->localkey, pVal->valuestring);
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
