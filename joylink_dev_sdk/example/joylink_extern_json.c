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
#ifdef JOYLINK_SDK_EXAMPLE_TEST
			if(!jl_platform_strcmp(USER_DATA_POWER, pSId->valuestring)){
			    if(pV->type == cJSON_String){
				userDev->Power = jl_platform_atoi(pV->valuestring);
			    }
			    else if(pV->type == cJSON_Number){
			        userDev->Power = pV->valueint;
			    }
			    joylink_dev_user_data_set( USER_DATA_POWER,userDev);
			}
			if(!jl_platform_strcmp(USER_DATA_MODE, pSId->valuestring)){
			    if(pV->type == cJSON_String){
				userDev->Mode = jl_platform_atoi(pV->valuestring);
			    }
			    else if(pV->type == cJSON_Number){
			        userDev->Mode = pV->valueint;
			    }
			    joylink_dev_user_data_set( USER_DATA_MODE,userDev);
			}
			if(!jl_platform_strcmp(USER_DATA_STATE, pSId->valuestring)){
			    if(pV->type == cJSON_String){
				userDev->State = jl_platform_atoi(pV->valuestring);
			    }
			    else if(pV->type == cJSON_Number){
			        userDev->State = pV->valueint;
			    }
			    joylink_dev_user_data_set( USER_DATA_STATE,userDev);
			}
#endif

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

#ifdef JOYLINK_SDK_EXAMPLE_TEST
	char i2str[64];
	cJSON *data_Power = cJSON_CreateObject();
	cJSON_AddItemToArray(arrary, data_Power);
	cJSON_AddStringToObject(data_Power, "stream_id", "Power");
	jl_platform_memset(i2str, 0, sizeof(i2str));
	jl_platform_sprintf(i2str, "%d", userDev->Power);
	cJSON_AddStringToObject(data_Power, "current_value", i2str);

	cJSON *data_Mode = cJSON_CreateObject();
	cJSON_AddItemToArray(arrary, data_Mode);
	cJSON_AddStringToObject(data_Mode, "stream_id", "Mode");
	jl_platform_memset(i2str, 0, sizeof(i2str));
	jl_platform_sprintf(i2str, "%d", userDev->Mode);
	cJSON_AddStringToObject(data_Mode, "current_value", i2str);

	cJSON *data_State = cJSON_CreateObject();
	cJSON_AddItemToArray(arrary, data_State);
	cJSON_AddStringToObject(data_State, "stream_id", "State");
	jl_platform_memset(i2str, 0, sizeof(i2str));
	jl_platform_sprintf(i2str, "%d", userDev->State);
	cJSON_AddStringToObject(data_State, "current_value", i2str);
#endif

	out=cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
RET:
	return out;
}

