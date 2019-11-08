#include <string.h>
#include "joylink.h"
#include "joylink_log.h"
#include "cJSON.h"
#include "joylink_extern.h"
#include "joylink_auth_md5.h"
#include "joylink_utils.h"
#include "joylink_dev_active.h"
#include "joylink_cloud_log.h"
#include "joylink_dev_active.h"


#define METHON_ACTIVATE     "jingdong.smart.api.activeAndBindForDevice"

#define DATA_METHOD    		"method"
#define DATA_BODY      		"360buy_param_json"
#define DATA_APP_KEY        "app_key"

#define DATA_DEVICE_ID      "device_id"
#define DATA_UUID           "pro_uuid"

#define DATA_RANDOM      	"random"
#define DATA_TIMESTAMP   	"timestamp"
#define DATA_V           	"v"
#define DATA_SIGN			"sign"

#define VERSION_JOY			"2.0"

#define TIME_TEST    "2018-12-21 17:20:20"

#define HTTPS_POST_HEADER "POST /dev HTTP/1.1\r\nHost: %s\r\nAccept: */*\r\ncontent-type: application/x-www-form-urlencoded; charset=utf-8\r\nContent-Length:"

extern int 
joylink_sdk_feedid_get(char *buf,char buflen);

#ifdef ESP_PLATFORM
extern int 
joylink_dev_https_post( char* host, char* query ,char *revbuf,int buflen, char * body);
#else
extern int 
joylink_dev_https_post( char* host, char* query ,char *revbuf,int buflen);
#endif

char gRandomStr[33] = {0};
char gRandomAStr[33] = {0};
char gURLStr[LEN_URL_MAX+1] = {0};
char gTokenStr[LEN_TOKEN_MAX+1] = {0};

//#include "joylink_syshdr.h"
/**
 * brief: 
 *
 * @Returns: 
 */
static int joylink_dev_bind_info_parse(char *msg, DevEnable_t *dev)
{
	int ret = -1;
	if(NULL == msg || NULL == dev){
		printf("--->:ERROR: pMsg is NULL\n");
		goto RET;
	}
	cJSON * pJson = cJSON_Parse(msg);

	if(NULL == pJson){
		printf("--->:cJSON_Parse is error!\n");
		goto RET;
	}
	cJSON * reponse = cJSON_GetObjectItem(pJson, "jingdong_smart_api_activeAndBindForDevice_response");
	if(reponse == NULL){
		printf("--->:reponse is error!\n");
		goto RET;
	}
	cJSON * result = cJSON_GetObjectItem(reponse, "result");
	if(result == NULL){
		printf("--->:reuslt is error!\n");
		goto RET;
	}

	cJSON * code = cJSON_GetObjectItem(reponse, "code");
	if(code == NULL){
		printf("--->:code is error!\n");
		goto RET;
	}

	if(code->valueint != 0){
		log_error("jingdong_smart_api_activeAndBindForDevice_response code error");
		goto RET;
	}

	cJSON *code_result = cJSON_GetObjectItem(result, "code");
	if(code_result == NULL){
		log_error("jingdong_smart_api_activeAndBindForDevice_response result code error");
		goto RET;
	}
	if(strcmp(code_result->valuestring,"200") != 0){
		log_error("jingdong_smart_api_activeAndBindForDevice_response result code error");
		goto RET;
	}

	
	
	cJSON * data = cJSON_GetObjectItem(result, "data");
	if(result == NULL){
		printf("--->:data is error!\n");
		goto RET;
	}
	
	cJSON * activate = cJSON_GetObjectItem(data, "activate");
	if(activate != NULL){
		cJSON * pSub = cJSON_GetObjectItem(activate, "feed_id");
		if(NULL != pSub){
		    strcpy(dev->feedid, pSub->valuestring);
		}
		pSub = cJSON_GetObjectItem(activate, "access_key");
		if(NULL != pSub){
		    strcpy(dev->accesskey, pSub->valuestring);
		}
		pSub = cJSON_GetObjectItem(activate, "c_idt");
		if(NULL != pSub){
			cJSON *sig = cJSON_GetObjectItem(pSub, "device_random_sig");
			if(sig != NULL){
				strcpy(dev->cloud_sig, sig->valuestring);
			}
		}
		pSub = cJSON_GetObjectItem(activate, "servers_info");
		if(pSub != NULL){                                      
			cJSON *server = cJSON_GetObjectItem(pSub, "joylink_server");
			if(NULL != server){
			    if(cJSON_GetArraySize(server) > 0){
				cJSON *pv;
				pv = cJSON_GetArrayItem(server, 0);
				if(NULL != pv){
				    strcpy(dev->joylink_server, pv->valuestring);
				}
			    }
			}	
		}
	}
	ret = 0;
RET:
	if(pJson != NULL){
		cJSON_Delete(pJson);
	}
	return 0;
}



int joylink_dev_bind_info_parse_write(char *json, char *randstr)
{
	int ret = -1;
	uint8_t sig[64] = {0}; 
	uint8_t pubkey[33] = {0};

	char ack_json[1400] = {0};

	DevEnable_t dev;
	if(json == NULL){
		log_error("json data null");
		return ret;
	}
	log_info("json->%s",json);
	
	memset(&dev, 0, sizeof(DevEnable_t));
	ret = joylink_dev_bind_info_parse(json,&dev);
	if(ret < 0){
		log_error("json parse error");
		joylink_cloud_log_post(TAGID_LOG_AP_GET_FEEDID_RES, RES_LOG_FAIL,"2018-12-21 17:20:20","[CloudLog]Get active data failed","0",NULL);
		return E_RET_ERROR;
	}else{
		log_info("json parse success");
		joylink_cloud_log_post(TAGID_LOG_AP_GET_FEEDID_RES, RES_LOG_SUCCES,"2018-12-21 17:20:20","[CloudLog]Get active data success","0",dev.feedid);
	}
	
	ret = joylink_active_write_key(&dev,randstr);

	return ret;
}



static char *joylink_dev_active_post_json_gen(char *token, char *signaure, char *rand)
{
	cJSON *root;
	char *out = NULL;

	int i = 0, j = 0;
	char macstr[32] = {0};
	char feedid[64] = {0};
	root = cJSON_CreateObject();
	if(NULL == root){
		return NULL;
	}

	cJSON *json;
    json = cJSON_CreateObject();
    if(NULL == json){
    	return NULL;
    }
#if 0
	for(i = 0; i < strlen(mac); i++){
		if((mac[i] >= '0' && mac[i] <= '9')\
		 || (mac[i] >= 'a' && mac[i] <= 'z')\
		 || (mac[i] >= 'A' && mac[i] <= 'Z')){
			temp_mac[j] = mac[i];
			j++;
		}
	}
#endif

	memset(macstr,0,sizeof(macstr));
	if(joylink_dev_get_user_mac(macstr) < 0){
		log_error("device id get error");
	}else{
		log_info("device id->%s",macstr);
	}

	cJSON_AddStringToObject(json, "device_id", macstr);
	cJSON_AddStringToObject(json, "device_add_info", "");

	cJSON_AddNumberToObject(json, "is_iot_alpha", 1);
	cJSON_AddStringToObject(json, "product_uuid", JLP_UUID);
	cJSON_AddStringToObject(json, "sdk_version", _VERSION_);
	cJSON_AddStringToObject(json, "token", token);

	if(joylink_sdk_feedid_get(feedid,sizeof(feedid)) < 0){
		cJSON_AddNumberToObject(json, "feed_id", 0);
	}else{
		cJSON_AddStringToObject(json, "feed_id", feedid);
	}
	
	cJSON_AddItemToObject(root,"json", json);
	

    cJSON *idt;
    idt = cJSON_CreateObject();
    if(NULL == idt){
    	return NULL;
    }

	cJSON_AddNumberToObject(idt, "t", 0);
	cJSON_AddStringToObject(idt, "d_r", rand);
	cJSON_AddStringToObject(idt, "d_s", signaure);

    cJSON_AddStringToObject(idt, "version", "2.0");
	cJSON_AddStringToObject(idt, "app_rand", "12345678901234567890123456789012");

    cJSON_AddItemToObject(json,"d_idt", idt);

	out=cJSON_Print(root);
	cJSON_Delete(root);

	return out;
}



static char *joylink_dev_active_post_sign_gen(char *cjson,char *random_d)
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

	char macstr[32];
	memset(macstr,0,sizeof(macstr));
	if(joylink_dev_get_user_mac(macstr) < 0){
		log_error("device id get error");
	}else{
		log_info("device id->%s",macstr);
	}

	char privatekey[128];
	memset(privatekey,0,sizeof(privatekey));
	if(joylink_dev_get_private_key(privatekey)<0){
		log_error("private key get error");
		
	}
	
	sprintf(temp+strlen(temp), "%s", privatekey);
	sprintf(temp+strlen(temp), "%s%s", DATA_BODY, cjson);
	sprintf(temp+strlen(temp), "%s%s", DATA_DEVICE_ID, macstr);
	sprintf(temp+strlen(temp), "%s%s", DATA_METHOD, METHON_ACTIVATE);
	sprintf(temp+strlen(temp), "%s%s", DATA_UUID, JLP_UUID);
	//sprintf(temp+strlen(temp), "%s%s", DATA_RANDOM, random_d);
	sprintf(temp+strlen(temp), "%s%s", DATA_RANDOM, gRandomAStr);

	sprintf(temp+strlen(temp), "%s%s", DATA_TIMESTAMP, TIME_TEST);
	sprintf(temp+strlen(temp), "%s%s", DATA_V, VERSION_JOY);
	//sprintf(temp+strlen(temp), "%s%s", DATA_V, TIME_TEST);
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


char *joylink_dev_active_postbody_gen(char *cjson,char *random_d)
{	
	char *temp = NULL ;
	char *sign = NULL;
	
	if(cjson == NULL){
		log_error("param NULL");
		return NULL;
	}
	sign = joylink_dev_active_post_sign_gen(cjson,random_d);
	if(sign == NULL){
		log_error("sign NULL");
		return NULL;
	}

	temp = (char *)malloc(1024);
	if(temp == NULL){
		log_error("malloc NULL");
		free(sign);
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

	sprintf(temp+strlen(temp), "&%s=%s", DATA_BODY, cjson);
	sprintf(temp+strlen(temp), "&%s=%s", DATA_DEVICE_ID, macstr);
	sprintf(temp+strlen(temp), "&%s=%s", DATA_METHOD, METHON_ACTIVATE);
	sprintf(temp+strlen(temp), "&%s=%s", DATA_UUID, JLP_UUID);

	sprintf(temp+strlen(temp), "&%s=%s", DATA_RANDOM, gRandomAStr);
	sprintf(temp+strlen(temp), "&%s=%s", DATA_TIMESTAMP, TIME_TEST);
	sprintf(temp+strlen(temp), "&%s=%s", DATA_V, VERSION_JOY);

	sprintf(temp+strlen(temp), "&%s=%s", DATA_SIGN, sign);
		
	if(sign != NULL)
		free(sign);
	
	return temp;
}


static int joylink_dev_active_post(char *token, char *url)
{
	int ret = -1;
	char *json = NULL;
	char txbuf[1400] = {0};
	char txheader[512] = {0};
	char countbuf[1400] = {0};
	char *postbody = NULL;
	
	uint8_t signaure[129] = {0};

	if(token == NULL || url == NULL){
		return -1;
	}
	
	joylink_util_randstr_gen(gRandomStr,sizeof(gRandomStr) - 1);
	joylink_util_randstr_gen(gRandomAStr,sizeof(gRandomAStr) - 1);
	log_info("random->%s",gRandomStr);
	
	if(strlen(token) != 0 && strlen(url) != 0){
		int ret_sign = 0;
		uint8_t prikey_buf[65] = {0};
		uint8_t sign_buf[65] = {0};

		log_info("test prikey: %s", _g_pdev->jlp.prikey);
		log_info("test token: %s, len: %d", token, (int)strlen(token));
		
        joylink_util_hexStr2bytes(_g_pdev->jlp.prikey, prikey_buf, 32);
		
		ret_sign = jl3_uECC_sign(prikey_buf, token, strlen(token), sign_buf, uECC_secp256r1());
		if(ret_sign == 1){
			log_info("gen devsigature to cloud random success");
		}else{
			log_error("gen dev sigature to cloud random error");
			return -1;
		}

		joylink_util_byte2hexstr(sign_buf, 64, signaure, 64*2);
	}else{
		ret = -1;
		goto RET;
	}

	json = joylink_dev_active_post_json_gen(token, (char *)signaure, gRandomStr);
	if(json == NULL){
		log_error("json gen error");
		ret = -1;
		goto RET;
	}
	
	log_info("activate req json: %s", json);

	postbody = joylink_dev_active_postbody_gen(json,gRandomStr);
	if(postbody == NULL){
		log_error("post body gen error");
		ret = -1;
		goto RET;
	}
	log_info("postbody ,len = %d,body=%s",(int)strlen(postbody),postbody);

	memset(txbuf,0,sizeof(txbuf));
	memset(txheader,0,sizeof(txheader));

	sprintf(txheader,HTTPS_POST_HEADER,url);
	sprintf(txbuf,"%s%d%s%s",txheader,(int)strlen(postbody),"\r\n\r\n",postbody);

	log_info("http txbuf = \n%s",txbuf);
	
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

	ret = joylink_dev_bind_info_parse_write(countbuf,gRandomStr);
	if(ret < 0){
		log_error("dev active error");
		ret = -1;
		joylink_cloud_log_post(TAGID_LOG_AP_FULL_RES, RES_LOG_FAIL,"2018-12-21 17:20:20","[CloudLog]device active failed","0",NULL);
		
		goto RET;
	}else{
			char feedid[64] = {0};
			if(joylink_sdk_feedid_get(feedid,sizeof(feedid)) >= 0){
				joylink_cloud_log_post(TAGID_LOG_AP_FULL_RES, RES_LOG_SUCCES,"2018-12-21 17:20:20","[CloudLog]device active success","0",feedid);
			}
	}
	ret = 0;
RET:
	if(json != NULL){
		free(json);
	}
	if(postbody != NULL){
		free(postbody);
	}
	return ret;
}

int joylink_dev_active_param_set(char *urlstr,char *tokenstr)
{
	if(urlstr == NULL || tokenstr == NULL){
		log_error("para NULL");
		return -1;
	}
	
	memset(gURLStr,0,sizeof(gURLStr));
	memset(gTokenStr,0,sizeof(gTokenStr));
	strcpy(gURLStr,urlstr);
	strcpy(gTokenStr,tokenstr);
	log_info("set active para,url=%s,token=%s",gURLStr,gTokenStr);
	return 0;
}

int joylink_dev_active_req(void)
{
	return joylink_dev_active_post(gTokenStr,gURLStr);
}


