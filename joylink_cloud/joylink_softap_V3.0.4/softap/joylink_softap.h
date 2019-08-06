/*************************************

Copyright (c) 2015-2050, JD Smart All rights reserved. 

*************************************/


#ifndef _JOYLINK_H_
#define _JOYLINK_H_
#include "joylink_softap_util.h"

#define	SOFTAP_UUID	    CONFIG_JOYLINK_DEVICE_UUID	        //puid
#define SOFTAP_BRAND	    CONFIG_JOYLINK_DEVICE_BRAND		//brand 
#define SOFTAP_CID	    CONFIG_JOYLINK_DEVICE_CID		//cid

// #define DEV_SOFTAP_SSID     "JDDeng9141"

/*
Description:
    The ssid.
Note:
    In softap mode,system is expected to use this as the ssid.And the password is default "12345678"
*/
extern uint8 softap_ssid[MAX_LEN_OF_SSID+1];

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
int joylink_softap_data_packet_handle(int socket_fd, uint8 *msg, int16 count);	

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
int joylink_softap_udpbroad(int socket_fd);			

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


#ifdef _IS_DEV_REQUEST_ACTIVE_SUPPORTED_
/**
 * @name:joylink_softap_is_need_active 
 *
 * @returns:   
 */
int joylink_softap_is_need_active(void);
/**
 * @name:joylink_softap_active_clear 
 *
 * @returns:   
 */
int joylink_softap_active_clear(void);

#endif

#endif
