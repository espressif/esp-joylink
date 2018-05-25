#ifndef _JOYLINK_HEAD_H_
#define _JOYLINK_HEAD_H_
/* Boolean Type */
typedef unsigned char        				bool;

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

#define IS_PACKET_WITH_ENDECRIPT		1
#define IS_PACKEY_WITH_CRC					1

#define TRUE			1
#define FALSE			0

#define MAX_LEN_OF_SSID                                  (32)


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

#define SOFTAP_BROAD_PUBKEY_PACKETLEN	(SOFTAP_LEN_PACKET_WITHOUTDATA + LEN_PUBLICKEY_ECC_ZIP)	//the packet length of broad public key packet

typedef enum _joylink_softap_status
{
	SOFTAP_PROCESSING = 0,
	SOFTAP_SUCCESS,
	SOFTAP_FAIL
}joylink_softap_status;


#endif


