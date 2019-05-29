/*************************************

Copyright (c) 2015-2050, JD Smart All rights reserved. 

*************************************/
#ifndef _JOYLINK_SMNT_H_
#define _JOYLINK_SMNT_H_
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAC_ADDR_LEN            6
#define SUBTYPE_PROBE_REQ       4
#define VERSION_SMNT                                    "3.0.11"
typedef  unsigned char        	uint8;
typedef  char                  	int8;
typedef  unsigned short      	uint16;


typedef struct{
	unsigned char type;	                        // 0:NotReady, 1:ControlPacketOK, 2:BroadcastOK, 3:MulticastOK
	unsigned char encData[1+32+32+32];            // length + EncodeData
}smnt_encrypt_data_t;

typedef enum{
	SMART_CH_INIT 		=	0x1,
	SMART_CH_LOCKING 	=	0x2,
	SMART_CH_LOCKED 	=	0x4,
	SMART_FINISH 		= 	0x8
}smnt_status_t;

typedef struct{
    uint16        Ver:2;                // Protocol version
    uint16        Type:2;                // MSDU type
    uint16        SubType:4;            // MSDU subtype
    uint16        ToDs:1;                // To DS indication
    uint16        FrDs:1;                // From DS indication
    uint16        MoreFrag:1;            // More fragment bit
    uint16        Retry:1;            // Retry status bit
    uint16        PwrMgmt:1;            // Power management bit
    uint16        MoreData:1;            // More data bit
    uint16        Wep:1;                // Wep data
    uint16        Order:1;            // Strict order expected
} FRAME_CONTROL;

typedef struct{
    FRAME_CONTROL   FC;
    uint16          Duration;
    uint8           Addr1[MAC_ADDR_LEN];
    uint8           Addr2[MAC_ADDR_LEN];
    uint8            Addr3[MAC_ADDR_LEN];
    uint16            Frag:4;
    uint16            Sequence:12;
    uint8            Octet[0];
} *PHEADER_802_11;


typedef enum{
	smnt_result_ok,
	smnt_result_decrypt_error,
	smnt_result_ssidlen_error,
	smnt_result_pwdlen_error
}joylink_smnt_result_flag_t;

typedef struct{
	joylink_smnt_result_flag_t smnt_result_status;
	uint8 jd_ssid_len;
	uint8 jd_password_len;
	uint8 jd_ssid[33];
	uint8 jd_password[65];
}joylink_smnt_result_t;


//typedef void (*joylink_smnt_channel_cb)(unsigned char ch);
typedef void (*joylink_smnt_result_cb)(joylink_smnt_result_t result_info);

typedef struct{
	unsigned char secretkey[16+1];
	//void (*switch_channel_callback)(unsigned char);
	void (*get_result_callback)(joylink_smnt_result_t);
} joylink_smnt_param_t;


typedef struct{
	smnt_status_t state;	
	uint16 syncFirst;				
	uint8 syncCount;				
	uint8 syncStepPoint;			
	uint8 syncFirst_downlink;		
	uint8 syncCount_downlink;		
	uint8 syncStepPoint_downlink;	
	uint8 directTimerSkip;			
	uint8 broadcastVersion;			
	uint8 muticastVersion;			
	uint8  broadIndex;				
	uint8  broadBuffer[5];			
	uint16 lastLength;			
	uint16 lastUploadSeq;		
	uint16 lastDownSeq;		
	uint8  syncAppMac[6];		
	uint8  syncBssid[6];		
	uint16 syncIsUplink;		
	uint8 chCurrentIndex;		
	uint8 chCurrentProbability;	
	uint8 isProbeReceived;	
	uint8 payload_multicast[128];	
	uint8 payload_broadcast[128];
	smnt_encrypt_data_t result;	
}joylinkSmnt_t;

void joylink_smnt_init(joylink_smnt_param_t param);                                  
int  joylink_smnt_cyclecall(void);                                               
void joylink_smnt_datahandler(PHEADER_802_11 pHeader, int length);               
void joylink_smnt_release(void);
void joylink_smnt_reset(void);


#endif
