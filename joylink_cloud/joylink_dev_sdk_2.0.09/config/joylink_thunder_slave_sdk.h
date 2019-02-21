#ifndef _JOYLINK_THUNDER_SLAVE_SDK_H_
#define _JOYLINK_THUNDER_SLAVE_SDK_H_
//#include "joylink_syshdr.h"
#include "joylink_thunder.h"

#define JOY_REQCHANNEL_STAY_COUNT		5
#define JOY_THUNDER_TIMEOUT_COUNT		(20*5)		//5s
#define JOY_THUNDER_TIMEOUT_FINISH		5
#define JOY_THUNDER_TIMEOUT_REJECT		5

#define JOY_UUID_LEN					6
#define JOY_RANDOM_LEN					0x20

#define JOY_ECC_PRIKEY_LEN				0x20
#define JOY_ECC_PUBKEY_LEN				0x40
#define JOY_ECC_SIG_LEN					0x40

typedef enum{
	sInit,					//init
	sReqChannel,			//requeset channel of thunderconfig master
	sDevInfoUp,				//devinfo transfer
	sDevChallengeUp,		//device challenge up
	sDevSigUp,				//device signature up
	sFinish,
	sReject
}tc_slave_state_t;

typedef struct{
	tc_msg_value_t errorcode;
	tc_vl_t		ap_ssid;
	tc_vl_t		ap_password;
	tc_vl_t		cloud_feedid;
	tc_vl_t		cloud_ackey;
	tc_vl_t		cloud_server;
}tc_slave_result_t;

#define MAX_PROBERESP_LEN 1024
#define MAX_CUSTOM_VENDOR_LEN 256

typedef void (*switch_channel_cb_t)(unsigned char ch);
typedef int (*get_random_cb_t)(void);
typedef int (*thunder_finish_cb_t)(tc_slave_result_t *result);
typedef int (*packet_80211_send_cb_t)(unsigned char *buf,int buflen);

typedef struct 
{
	tc_slave_state_t thunder_state;
	uint8_t 		tcount;
	
	uint8_t 		vendor_tx[MAX_CUSTOM_VENDOR_LEN];
	uint8_t 		vendor_tx_len;
	uint8_t 		current_channel;

	uint8_t			uuid[JOY_UUID_LEN];
	uint8_t			prikey_d[JOY_ECC_PRIKEY_LEN];			
	uint8_t			pubkey_c[JOY_ECC_PUBKEY_LEN];
	
	uint8_t			mac_master[JOY_MAC_ADDRESS_LEN];
	uint8_t			mac_dev[JOY_MAC_ADDRESS_LEN];
	uint8_t			dev_random[JOY_RANDOM_LEN];

	deviceid_t 		deviceid;
	deviceinfo_t 	deviceinfo;
} tc_slave_ctl_t;

typedef struct{
	uint8_t 		uuid[JOY_UUID_LEN];				//UUID,MUST Param
	uint8_t			prikey_d[JOY_ECC_PRIKEY_LEN];	//prikey_d,MUST Param
	uint8_t			pubkey_c[JOY_ECC_PUBKEY_LEN];	//pubkey_c,MUST Param
	
	uint8_t			mac_dev[JOY_MAC_ADDRESS_LEN];	//device mac address,MUSG param
	deviceid_t 		deviceid;						//device id,MUST Param,value--value address,length--value length
	deviceinfo_t	deviceinfo;						//device id,opentional param,if no set len = 0 or value = NULL

	switch_channel_cb_t	switch_channel;				//swithc channel call back function				
	get_random_cb_t	get_random;						//get random call back function
	thunder_finish_cb_t	result_notify_cb;			//result notify call back function
	packet_80211_send_cb_t	packet_80211_send_cb;	//802.11 layer packet send call back function
}tc_slave_func_param_t;

/**
 * brief:  thunder config slave init function,
 * @Param: tc_slave_func_param_t
 			typedef struct{
					uint8_t 		uuid[JOY_UUID_LEN];				//UUID,MUST Param
					uint8_t			prikey_d[JOY_ECC_PRIKEY_LEN];	//prikey_d,MUST Param
					uint8_t			pubkey_c[JOY_ECC_PUBKEY_LEN];	//pubkey_c,MUST Param
	
					uint8_t			mac_dev[JOY_MAC_ADDRESS_LEN];	//device mac address,MUSG param
					deviceid_t 		deviceid;						//device id,MUST Param,value--value address,length--value length
					deviceinfo_t	deviceinfo;						//device id,opentional param,if no set len = 0 or value = NULL
					void 			*switch_channel(unsigned char);	//swithc channel call back function
					int  			*get_random(void);					//get random call back function
					int				*result_notify_cb(tc_slave_result_t *);	//result notify call back function
					int 			*packet_80211_send_cb(unsigned char *buf,int buflen);//802.11 layer packet send call back function
			}tc_slave_func_param_t;
 * @Returns:RET_OK,RET_ERROR;
*/ 
int joyThunderSlaveInit(tc_slave_func_param_t *para);

/**
 * brief:  thunderconfig slave start function
 * @Param: NULL
 * @Returns:RET_OK,RET_ERROR;
*/ 
int joyThunderSlaveStart(void);

/**
 * brief:  thunderconfig slave stop function
 * @Param: NULL
 * @Returns:RET_OK,RET_ERROR;
*/ 
int joyThunderSlaveStop(void);

/**
 * brief:  802.11 packet handler function,the device capture the 802.11 layer packet,
 		transfer it to this function to handle it.
 * @Param: probe_req,packet buffer address
 * @Param: req_len,packet len
 * @Returns:NULL
*/ 
void joyThunderSlaveProbeH(void *probe_req, int req_len);

/**
 * brief:  cycle call function ,the function is expected call every 50ms
 * @Param: NULL
 * @Param: NULL
 * @Returns:NULL
*/ 
void joyThunderSlave50mCycle(void);

#endif



