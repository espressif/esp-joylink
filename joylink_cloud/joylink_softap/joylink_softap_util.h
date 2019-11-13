#ifndef _JOYLINK_SOFTAP_UTIL_H_
#define _JOYLINK_SOFTAP_UTIL_H_
/* Boolean Type */
typedef char                       	int8,   *pint8,  *pchar;
typedef short                      	int16, *pint16;
typedef int                       	int32;
typedef long long                 	int64;
typedef unsigned char         	 		uint8, *puint8,   *puchar;
typedef unsigned short         			uint16, *puint16;
typedef unsigned short         			uint16le;
typedef unsigned int             		uint32,*puint32;
typedef unsigned int             		uint32le;
typedef unsigned long long   				uint64,*puint64;

#define TRUE			1
#define FALSE			0

#define MAX_LEN_OF_SSID                                  (32)
#define MAX_LEN_OF_PASSWORD									64
#define LEN_URL_MAX			32
#define LEN_TOKEN_MAX		16

/*packet type*/
#define SOFTAP_TYPE_PUBKEY							1						//device	->app public key type
#define SOFTAP_TYPE_DEVCIEIN_R1					2						//app	->device R1 type
#define SOFTAP_TYPE_DEVICEIN_SSIDPASS		4						//app	->device	ssid and pass packet type
#define SOFTAP_TYPE_DEVICEOUT_R2        3						//device	->app R2 packet type
#define SOFTAP_TYPE_DEVICEOUT_RESULT		5						//device	->app ssid and pass setting result type

/*packet message*/
#define SOFTAP_OFFSET_TYPE							4
#define SOFTAP_OFFSET_DATALEN						5
#define SOFTAP_OFFSET_DATA							6
#define SOFTAP_LEN_PACKET_WITHOUTDATA		8						//the length of the packet without area

#define LEN_PUBLICKEY_ECC				0x40
#define LEN_SHAREDKEY_LEN				0x20
#define LEN_PRIVIATEKEY_ECC			0x20
#define LEN_R1R2_ECC						0x20

#define LEN_PUBLICKEY_ECC_ZIP		33

#define SOFTAP_FLAG_TCP      0
#define SOFTAP_FLAG_UDP      1
#define SOFTAP_FLAG_TCP_UDP  2

#define SOFTAP_SHORT_VERSION	2
#define SOFTAP_BROAD_PUBKEY_PACKETLEN	(SOFTAP_LEN_PACKET_WITHOUTDATA + LEN_PUBLICKEY_ECC_ZIP + 1)	//the packet length of broad public key packet

#define SIZE_SSID		32
#define SIZE_PASSWORD	64
#define SOFTAP_PACKET_HEAD				"JYAP"					//packet head


typedef enum _joylink_softap_status
{
	SOFTAP_PROCESSING = 0,
	SOFTAP_SUCCESS,
	SOFTAP_FAIL
}softapStatus_t;

typedef struct _joylinkSoftAP_Result
{
	unsigned char type;						//reserved
	unsigned char ssid[SIZE_SSID+1];
	unsigned char pass[SIZE_PASSWORD+1];
#ifdef _IS_DEV_REQUEST_ACTIVE_SUPPORTED_
	unsigned char url[LEN_URL_MAX + 1];
	unsigned char token[LEN_TOKEN_MAX + 1];
#endif
	unsigned char bssid[6];					//reserved
}joylinkSoftAP_Result_t;

typedef struct _joylinkSoftAPRam
{
	uint8 is_generate_key;
	uint8 is_stop_broadcast_pubkey;
	uint8 ecc_publickey[LEN_PUBLICKEY_ECC];
	uint8 ecc_privatekey[LEN_PRIVIATEKEY_ECC];
	uint8 shared_key[LEN_SHAREDKEY_LEN];
	uint8 ecc_r1[LEN_R1R2_ECC];
	uint8 ecc_r2[LEN_R1R2_ECC];
	uint8 ecc_publickey_remote[LEN_PUBLICKEY_ECC];
	const struct uECC_Curve_t * ecc_curves;
	uint8 sessionkey[0x20];
	softapStatus_t				status;
	joylinkSoftAP_Result_t 		softap_result;
#ifdef _IS_DEV_REQUEST_ACTIVE_SUPPORTED_
	uint8 is_need_active;
#endif
}joylinkSoftAPRam;

#endif


