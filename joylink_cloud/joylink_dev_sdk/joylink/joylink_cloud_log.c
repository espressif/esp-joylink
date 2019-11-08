#include <stdint.h>
#include <string.h>
#include "joylink.h"
#include "joylink_log.h"
#include "cJSON.h"
#include "joylink_extern.h"
#include "joylink_auth_md5.h"
#include "joylink_utils.h"
#include "joylink_dev_active.h"
#define HTTPS_POST_LOG_HEADER "POST /dev HTTP/1.1\r\nHost: %s\r\nAccept: */*\r\ncontent-type: application/x-www-form-urlencoded; charset=utf-8\r\nContent-Length:"

#define METHON_CLOUD_LOG     	"jingdong.smart.api.device.logreport"


#define LOG_DATA_BODY      		"360buy_param_json"
#define LOG_DATA_BIZID			"biz_id"

#define LOG_DATA_DEVICE_ID      "device_id"
#define LOG_DATA_METHOD    		"method"
#define LOG_DATA_UUID           "pro_uuid"
#define LOG_DATA_RANDOM      	"random"
#define LOG_DATA_TIMESTAMP   	"timestamp"

static char gLogURLStr[32+1] = {0};
static char gLogTokenStr[16+1] = {0};

extern int 
joylink_sdk_feedid_get(char *buf,char buflen);

#ifdef ESP_PLATFORM
extern int 
joylink_dev_https_post( char* host, char* query ,char *revbuf,int buflen, char * body);
#else
extern int 
joylink_dev_https_post( char* host, char* query ,char *revbuf,int buflen);
#endif

static char *joylink_cloud_log_post_json_gen(char *tagId, char *result,char *time,char *payload,char *duration,char *feedid)
{
	cJSON *arrary;
	char *jsonOut = NULL;

	cJSON *jsonbody;
	jsonbody = cJSON_CreateObject();
    if(NULL == jsonbody){
    	goto RET;
    }	
	
	arrary = cJSON_CreateArray();
	if(arrary == NULL){
		log_error("cJSON_CreateArray error");
		goto RET;
	}

	cJSON_AddItemToObject(jsonbody,"log_body", arrary);
	cJSON_AddStringToObject(jsonbody,"biz_id","softAp_conf_log");
	
	cJSON *json;
    json = cJSON_CreateObject();
    if(NULL == json){
    	log_error("cJSON_CreateObject error");
		goto RET;
    }
	cJSON_AddItemToArray(arrary, json);

	if(tagId != NULL){
		cJSON_AddStringToObject(json, "tagId", tagId);
	}
	if(result != NULL){
		cJSON_AddStringToObject(json, "result", result);
	}
	if(time != NULL){
		cJSON_AddStringToObject(json, "time", time);
	}
	if(strlen(gLogTokenStr) != 0){
		cJSON_AddStringToObject(json, "token", gLogTokenStr);
	}
	
	if(payload != NULL){
		cJSON_AddStringToObject(json, "payload", payload);
	}

	if(duration != NULL){
		cJSON_AddStringToObject(json, "duration", duration);
	}
	if(feedid != NULL){
		cJSON_AddStringToObject(json, "feedId", feedid);
	}

	cJSON_AddStringToObject(json, "uuid", JLP_UUID);

	char macstr[32] = {0};
	memset(macstr,0,sizeof(macstr));
	if(joylink_dev_get_user_mac(macstr) < 0){
		log_error("device id get error");

	}else{
		log_info("device id->%s",macstr);
		cJSON_AddStringToObject(json, "deviceId", macstr);
	}
	
	jsonOut =cJSON_Print(jsonbody);

RET:
	if(jsonbody!= NULL){
		cJSON_Delete(jsonbody);
	}

	return jsonOut;
}

static char *joylink_cloud_log_post_sign_gen(char *cjson,char *rstr)
{
	MD5_CTX ctx;
	char randomstr[33] = {0};
	char md5_out[16] = {0};

	int i = 0;

	char *temp = (char *)malloc(1024);
	char *sign = (char *)malloc(33);
	char tmp[16] = {0};

	srand((unsigned int)time(NULL));

	for(i = 0; i < sizeof(tmp); i++){
	    tmp[i] = (char)rand();
	}
	memset(randomstr,0,sizeof(randomstr));

	joylink_util_byte2hexstr(tmp,sizeof(tmp),randomstr,sizeof(randomstr));

	memset(temp, 0, 1024);
	memset(sign, 0, 33);
	char macstr[32] = {0};
	memset(macstr,0,sizeof(macstr));
	if(joylink_dev_get_user_mac(macstr) < 0){
		log_error("device id get error");

	}else{
		log_info("device id->%s",macstr);
	}
	
	char privatekey[128];
	memset(privatekey,0,sizeof(privatekey));
	joylink_dev_get_private_key(privatekey);

	sprintf(temp+strlen(temp), "%s", privatekey);
	sprintf(temp+strlen(temp), "%s%s", LOG_DATA_BODY, cjson);
	sprintf(temp+strlen(temp), "%s%s", "device_id", macstr);
	sprintf(temp+strlen(temp), "%s%s", LOG_DATA_METHOD, METHON_CLOUD_LOG);
	sprintf(temp+strlen(temp), "%s%s", "pro_uuid", JLP_UUID);
	sprintf(temp+strlen(temp), "%s%s", "random", rstr);
	sprintf(temp+strlen(temp), "%s%s", "timestamp", "2018-12-21 17:20:20");
	sprintf(temp+strlen(temp), "%s%s", "v", "2.0");
	
	
	sprintf(temp+strlen(temp), "%s", privatekey);


	printf("sign: %s\n\n", temp);

	memset(&ctx, 0, sizeof(MD5_CTX));

	JDMD5Init(&ctx);
	JDMD5Update(&ctx, (unsigned char*)temp, strlen(temp));
	JDMD5Final(&ctx, (unsigned char*)md5_out);

	for(i = 0; i < 16; i++){
		sign[2*i] = (md5_out[i] & 0xf0) >> 4;
		sign[2*i+1] = (md5_out[i] & 0x0f);

		if(sign[2*i] >= 0 && sign[2*i] <= 9){
			sign[2*i] += '0';
		}
		else if(sign[2*i] >= 0x0a && sign[2*i] <= 0x0f){
			sign[2*i] = (sign[2*i] - 10) + 'A';
		}
		if(sign[2*i+1] >= 0 && sign[2*i+1] <= 9){
			sign[2*i+1] += '0';
		}
		else if(sign[2*i+1] >= 0x0a && sign[2*i+1] <= 0x0f){
			sign[2*i+1] = (sign[2*i+1] - 10) + 'A';
		}
	}
	
	if(temp != NULL){
		free(temp);
	}

	return sign;
}

char *joylink_cloud_log_postbody_gen(char *cjson)
{	
	char *temp = NULL ;
	char *sign = NULL;
	char randstr[33] = {0};
	
	if(cjson == NULL){
		log_error("param NULL");
		return NULL;
	}
	
	temp = (char *)malloc(1024);
	if(temp == NULL){
		log_error("malloc NULL");
		return NULL;
	}

	
	joylink_util_randstr_gen(randstr,sizeof(randstr) - 1);
	sign = joylink_cloud_log_post_sign_gen(cjson,randstr);
	if(sign == NULL){
		log_error("sign gen error");
		free(temp);
		return NULL;
	}

	
	memset(temp, 0, 1024);
	char macstr[32] = {0};
	memset(macstr,0,sizeof(macstr));
	if(joylink_dev_get_user_mac(macstr) < 0){
		log_error("device id get error");
	}else{
		log_info("device id->%s",macstr);
	}

	sprintf(temp+strlen(temp), "&%s=%s", LOG_DATA_BODY, cjson);
	sprintf(temp+strlen(temp), "&%s=%s", "device_id", macstr);
	sprintf(temp+strlen(temp), "&%s=%s", LOG_DATA_METHOD, METHON_CLOUD_LOG);
	sprintf(temp+strlen(temp), "&%s=%s", "pro_uuid", JLP_UUID);
	sprintf(temp+strlen(temp), "&%s=%s", "random", randstr);
	sprintf(temp+strlen(temp), "&%s=%s", "timestamp", "2018-12-21 17:20:20");
	sprintf(temp+strlen(temp), "&%s=%s", "v", "2.0");
	
	
	sprintf(temp+strlen(temp), "&%s=%s", "sign", sign);

	if(sign != NULL){
		free(sign);
	}
	
	return temp;
}



int joylink_cloud_log_post(char *tagId, char *result,char *time,char *payload,char *duration,char *feedid)
{
	int ret = -1;
	char *json = NULL;
	char txbuf[1400] = {0};
	char txheader[512] = {0};
	char countbuf[1400] = {0};
	char *postbody = NULL;
	
	char token[16+1] = {0};
	char url[32+1] = {0};

	if(tagId == NULL || result == NULL){

		log_error("Para NULL");
		return -1;
	}
	memset(token,0,sizeof(token));
	memset(url,0,sizeof(url));

	if(strlen(gLogURLStr) == 0){
		log_error("url NULL");
		return -1;
	}else{
		strcpy(url,gLogURLStr);
		log_info("url->%s",url);
	}

	if(strlen(gLogTokenStr) == 0){
		log_error("token NULL");
	}else{
		strcpy(token,gLogTokenStr);
		log_info("token->%s",token);
	}
	

	json = joylink_cloud_log_post_json_gen(tagId,result,time,payload,duration,feedid);
	

	if(json == NULL){
		log_error("json gen error");
		ret = -1;
		goto RET;
	}
	
	log_info("cloud log post json: %s", json);

	postbody = joylink_cloud_log_postbody_gen(json);
	if(postbody == NULL){
		log_error("post body gen error");
		ret = -1;
		goto RET;
	}
	log_info("postbody ,len = %d,body=%s",(int)strlen(postbody),postbody);

	memset(txbuf,0,sizeof(txbuf));
	memset(txheader,0,sizeof(txheader));
	
	sprintf(txheader,HTTPS_POST_LOG_HEADER,url);
	sprintf(txbuf,"%s%d%s%s",txheader,(int)strlen(postbody),"\r\n\r\n",postbody);

	log_info("http txbuf = \r\n%s",txbuf);
	
#ifdef ESP_PLATFORM
	ret = joylink_dev_https_post(url,txbuf,countbuf,sizeof(countbuf),postbody);
#else
	ret = joylink_dev_https_post(url,txbuf,countbuf,sizeof(countbuf));
#endif
	if(ret < 0){
		log_error("https error");
		ret = -1;
		goto RET;
	}

RET:
	if(json != NULL){
		free(json);
	}
	if(postbody != NULL){
		free(postbody);
	}
	return ret;
}


int joylink_cloud_log_param_set(char *urlstr,char *tokenstr)
{
	if(urlstr == NULL || tokenstr == NULL){
		log_error("para NULL");
		return -1;
	}
	
	memset(gLogURLStr,0,sizeof(gLogURLStr));
	memset(gLogTokenStr,0,sizeof(gLogTokenStr));
	strcpy(gLogURLStr,urlstr);
	strcpy(gLogTokenStr,tokenstr);
	log_info("set cloud log para,url=%s,token=%s",gLogURLStr,gLogTokenStr);
	return 0;
}


