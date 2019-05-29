#include "joylink_log.h"
#include "joylink_thunder.h"

#include "joylink_extern.h"
#include "joylink_utils.h"
#include "joylink_auth_md5.h"


#include "joylink_config_handle.h"

#define CYCLE_HANDLE_TIME  50*1000  // 50ms

#define JOY_CONFIG_STAY_COUNT 5

uint8_t config_stop_flag = 0;

typedef struct _Result
{
	uint8_t ssid_len;
	uint8_t pass_len;
	
	char ssid[33];
	char pass[33];
} Result_t;

Result_t config_result;

int joylink_get_random(void)
{
	return 0;
    //return rand();
}

void joylink_change_hannel(int ch)
{
    //mico_wlan_set_channel((int)ch);
    return;
}

void joylink_80211_recv(uint8_t *buf, int buflen)
{
#ifdef JOYLINK_THUNDER_SLAVE
	joyThunderSlaveProbeH(buf, buflen);
#endif
#ifdef JOYLINK_SMART_CONFIG
	joylink_smnt_datahandler((PHEADER_802_11)buf, buflen);
#endif
}

int joylink_80211_send(uint8_t *buf, int buflen)
{
	return 0;
	//return mico_wlan_send_mgnt(buf, buflen);
}

int joylink_delete_mark(uint8_t *str)
{
	int for_i = 0;
	int for_n = 0;

	uint8_t temp[64] = {0};

	if(str == NULL)	return -1;
	
	for(for_i = 0; for_i < strlen(str); for_i++)
	{
		if((str[for_i] >= '0' && str[for_i] <= '9') \
		|| (str[for_i] >= 'a' && str[for_i] <= 'f') \
		|| (str[for_i] >= 'A' && str[for_i] <= 'F')){
			temp[for_n] = str[for_i];
			for_n++;
		}
	}
	memset(str, 0, sizeof(str));
	memcpy(str, temp, strlen(temp));

	return 0;
}

/**
 * brief: 
 *
 * @Param: thunder slave init and finish
 *
 * @Returns: 
 */
extern JLPInfo_t user_jlp;
extern jl2_d_idt_t user_idt;

int 
joylink_thunder_slave_init(void)
{
	tc_slave_func_param_t thunder_param;

	log_info("init thunder slave!\r\n");

	memset(&thunder_param,0,sizeof(tc_slave_func_param_t));

	memcpy(thunder_param.uuid, user_jlp.uuid, 6);

	joylink_delete_mark(user_jlp.mac);
	joylink_util_hexStr2bytes(user_jlp.mac, thunder_param.mac_dev, JOY_MAC_ADDRESS_LEN);

	joylink_util_hexStr2bytes(user_jlp.prikey, thunder_param.prikey_d, JOY_ECC_PRIKEY_LEN);
	joylink_util_hexStr2bytes(user_idt.cloud_pub_key, thunder_param.pubkey_c, JOY_ECC_PUBKEY_LEN);

	thunder_param.deviceid.length = strlen(thunder_param.mac_dev);
	thunder_param.deviceid.value = joylink_util_malloc(thunder_param.deviceid.length);
	memcpy(thunder_param.deviceid.value, thunder_param.mac_dev, thunder_param.deviceid.length);

	thunder_param.switch_channel = (switch_channel_cb_t)joylink_change_hannel;
	thunder_param.get_random	 = 	(get_random_cb_t)joylink_get_random;
	thunder_param.result_notify_cb = (thunder_finish_cb_t)joylink_thunder_slave_finish;
	thunder_param.packet_80211_send_cb = (packet_80211_send_cb_t)joylink_80211_send;
	joyThunderSlaveInit(&thunder_param);

	joyThunderSlaveStart();

	joylink_util_free(thunder_param.deviceid.value);
	thunder_param.deviceid.value = NULL;
	return 0;
}

extern E_JLRetCode_t joylink_dev_set_attr_jlp(JLPInfo_t *jlp);

int 
joylink_thunder_slave_finish(tc_slave_result_t *result)
{
    	JLPInfo_t jlp;

	uint8_t temp[32] = {0};
	uint8_t localkey[33] = {0};
	MD5_CTX md5buf;
	
    	log_info("joylink thunder slave finish\r\n");

    	memset(&jlp, 0, sizeof(JLPInfo_t));
    	memcpy(jlp.feedid, result->cloud_feedid.value, result->cloud_feedid.length);
    	memcpy(jlp.accesskey, result->cloud_ackey.value, result->cloud_ackey.length);

	memset(&md5buf, 0, sizeof(MD5_CTX));
	JDMD5Init(&md5buf);
	JDMD5Update(&md5buf, result->cloud_ackey.value, result->cloud_ackey.length);
	JDMD5Final(&md5buf, temp);
	joylink_util_byte2hexstr(temp, 16, localkey, 32);
	
	memcpy(jlp.localkey, localkey, strlen(localkey));

    	log_info("feedid:%s, accesskey:%s, localkey: %s, serverurl:%s\n", result->cloud_feedid.value, result->cloud_ackey.value, localkey, result->cloud_server.value);

    	joylink_dev_set_attr_jlp(&jlp);

	memset(&config_result, 0, sizeof(Result_t));
	
	memcpy(config_result.ssid, result->ap_ssid.value, result->ap_ssid.length);
	config_result.ssid_len = result->ap_ssid.length;
	
	memcpy(config_result.pass, result->ap_password.value, result->ap_password.length);
	config_result.pass_len = result->ap_password.length;
	
		
    	log_info("ssid:%s, passwd:%s\n", config_result.ssid, config_result.pass);

	joylink_config_stop();

	return 0;
}

/**
 * brief: 
 *
 * @Param: smartConfig init and finish
 *
 * @Returns: 
 */

#define SMARTCONFIG_KEY "TJZ9M8SXE8IE9B5W"

joylink_smnt_param_t smart_conf_param;

int 
joylink_smart_config_init(void)
{
	memset(&smart_conf_param, 0, sizeof(joylink_smnt_param_t));

    	memcpy(smart_conf_param.secretkey, SMARTCONFIG_KEY, strlen(SMARTCONFIG_KEY));
	smart_conf_param.get_result_callback = joylinke_smart_config_finish;

	log_info("init smart config!\r\n");
	joylink_smnt_init(smart_conf_param);
}

void *
joylinke_smart_config_finish(joylink_smnt_result_t* presult)
{
	int len = 0;
	
	uint8_t p_result[100] = {0};
	uint8_t passOut[100] = {0};
	
	log_info("joylink smart config finish\r\n");
	
	memset(&config_result, 0, sizeof(Result_t));

	memcpy(config_result.pass, presult->jd_password, presult->jd_password_len);
	config_result.pass_len = presult->jd_password_len;

	memcpy(config_result.ssid, presult->jd_ssid, presult->jd_ssid_len);
	config_result.ssid_len = presult->jd_ssid_len;

	log_info("ssid:%s, passwd:%s\n", config_result.ssid, config_result.pass);
	joylink_config_stop();
	return 0;
}

/**
 * brief: 
 *
 * @Param: change channel
 *
 * @Returns: 
 */
static uint8_t config_count = 0;
static uint8_t config_channel = 0;

extern joylinkSmnt_t* pSmnt;
extern tc_slave_ctl_t tc_slave_ctl;

void 
joylink_config_change_channel(void)
{
#ifdef JOYLINK_THUNDER_SLAVE
	if(tc_slave_ctl.thunder_state > sReqChannel)
		return;
#endif
#ifdef JOYLINK_SMART_CONFIG
	if(pSmnt->state > SMART_CH_LOCKING)
		return;
#endif	
	config_count++;
	if(config_count == JOY_CONFIG_STAY_COUNT){
		config_count = 0;

		joylink_change_hannel(config_channel+1);
		log_info("-->switch channel to:%d", config_channel+1);
		
#ifdef JOYLINK_THUNDER_SLAVE	
		tc_slave_ctl.current_channel = config_channel + 1;
#endif
#ifdef JOYLINK_SMART_CONFIG
		pSmnt->chCurrentIndex = config_channel;

		pSmnt->state = SMART_CH_LOCKING;
		pSmnt->syncStepPoint = 0;
		pSmnt->syncCount = 0;
		pSmnt->chCurrentProbability = 0;
#endif	
		config_channel++;
		if(config_channel == 13)
			config_channel = 0;	
	}			
}
/**
 * brief: 
 *
 * @Param: config loop handle
 *
 * @Returns: 
 */
void *
joylink_config_loop_handle(void *data)
{
	time_t *time_out = (time_t *)data;
	time_t time_start = time(NULL);
	double time_old = 0;
	double time_now = 0;

	while(1){
		time_now = clock();
		if(time_now - time_old >= CYCLE_HANDLE_TIME){
			time_old = time_now;
			#ifdef JOYLINK_THUNDER_SLAVE
			joyThunderSlave50mCycle();
			#endif
			#ifdef JOYLINK_SMART_CONFIG
			joylink_smnt_cyclecall();
			#endif
			joylink_config_change_channel();
		}
		#ifdef JOYLINK_THUNDER_SLAVE
		if(config_stop_flag)
			break;
		#endif
		if(time_now - time_start > *time_out){
			joylink_config_stop();
			break;
		}
	}
}

int joylink_config_start(uint32_t time_out)
{
	char check_flag = 0;
#ifdef JOYLINK_THUNDER_SLAVE
	config_stop_flag = 0;
	check_flag = 1;
	joylink_thunder_slave_init();
#endif
#ifdef JOYLINK_SMART_CONFIG
	check_flag = 1;
	joylink_smart_config_init();
#endif	
	if(check_flag == 0){
		log_info("joylink config do not open!");
		return -1;
	}
	pthread_t tidp;
	int ret = pthread_create(&tidp, NULL, joylink_config_loop_handle, (void *)&time_out);
	if(ret < 0){
		log_info("joylink config pthread create failed!");
	}
	return 0;
}
int joylink_config_stop(void)
{
#ifdef JOYLINK_THUNDER_SLAVE
	config_stop_flag = 1;
	joyThunderSlaveStop();
#endif
	return 0;
}


