/*************************************

Copyright (c) 2015-2050, JD Smart All rights reserved. 

*************************************/


#ifndef _JOYLINK_SOFTAP_H_
#define _JOYLINK_SOFTAP_H_
#include "joylink_head.h"
#define SIZE_SSID		32
#define SIZE_PASSWORD	64

typedef struct _joylinkSoftAP_Result
{
	unsigned char type;						//reserved
	unsigned char ssid[SIZE_SSID+1];
	unsigned char pass[SIZE_PASSWORD+1];
	unsigned char bssid[6];					//reserved
}joylinkSoftAP_Result_t;

typedef struct _joylinkSoftAPRam
{
	uint8 is_generate_key;
	uint8 is_broadcast_pubkey;
	uint8 ecc_publickey[LEN_PUBLICKEY_ECC];
	uint8 ecc_privatekey[LEN_PRIVIATEKEY_ECC];
	uint8 shared_key[LEN_SHAREDKEY_LEN];
	uint8 ecc_r1[LEN_R1R2_ECC];
	uint8 ecc_r2[LEN_R1R2_ECC];
	uint8 ecc_publickey_remote[LEN_PUBLICKEY_ECC];
	const struct uECC_Curve_t * ecc_curves;
	uint8 sessionkey[0x20];
	joylink_softap_status	status;
	joylinkSoftAP_Result_t 		softap_result;
}joylinkSoftAPRam;

/*
Description:
    The ssid.
Note:
    In softap mode,system is expected to use this as the ssid.And the password is default "12345678"
*/
extern uint8				softap_ssid[MAX_LEN_OF_SSID+1];

/*
Routine Description:
    init function
Arguments:
    none
Return Value:
    dont take too much care
Note:
    System is expected to call this function when system init.
*/
int joylink_softap_init(void);
/*
Routine Description:
    TCP message handle function.
Arguments:
    msg       message address
    count  	  message length
Return Value:
    dont take too much care
Note:
    Use this function to handle the TCP message.
*/
int joylink_softap_tcppacket(uint8 *msg, int16 count);	

/*
Routine Description:
    UDP public key broad function
Arguments:
    none
Return Value:
    dont take too much care
Note:
	Broadcast local public key using UDP.
*/
int joylink_softap_udpbroad(void);			

/*
Routine Description:
    return the current softap handle result.If the device have received the ssid and password,
    It will return TRUE and return the result(ssid and password)
In:
    none
Return Value:
    TRUE  ---the result got
    FALSE ---no result got
Note:
	System is expected to call this function in inquiry method.
	If the function return the right result,System shoud enter to STA mode and try to connet to the ssid
*/
int joylink_softap_result(joylinkSoftAP_Result_t* pRet);

#endif
