#ifndef _JOYLINK_THUNEDER_H_
#define _JOYLINK_THUNEDER_H_

#define THUNDER_SLAVE_SUPPORT 	1
#define THUNDER_MASTER_SUPPORT	1

#include "joylink_syshdr.h"

#define JOY_THUNDER_VERSION		"JOYL_V1.0"

#define JOY_MEGIC_HEADER		"JOY"
#define JOY_OUI_TYPE			0
#define JOY_PACKET_HEADER_LEN	8
#define JOY_MAC_ADDRESS_LEN		6

#define JOY_CLA_ENCRYPT			1<<7			//
#define JOY_CLA_DECRYPT			0
#define JOY_CLA_UPLINK			0
#define JOY_CLA_DOWNLINK		1<<6
#define JOY_CLA_FIRSTPACKET		0
#define JOY_CLA_RESENDPACKET	1<<5

typedef enum
{
    TLV_TAG_MSG = 0x00,
	TLV_TAG_THUNDERCONFIG_VERSION,
	TLV_TAG_CID,
	TLV_TAG_VENDOR,
	TLV_TAG_UUID,
	TLV_TAG_MAC,
	TLV_TAG_SN,
	TLV_TAG_WIFI_CHANNEL,
	TLV_TAG_RANDOM,
	TLV_TAG_SIGNATURE,
	TLV_TAG_PUBKEY,
	TLV_TAG_PRIKEY,
	TLV_TAG_SSID,
	TLV_TAG_PASSWD,
	TLV_TAG_FEEDID,
	TLV_TAG_ACKEY,
	TLV_TAG_DEVICE_TOKEN,
	
	TLV_TAG_DEVICE_ID,
	TLV_TAG_DEVICE_INFO,
	TLV_TAG_CLOUD_SERVER
} tc_tlv_tag_t;

typedef enum
{
    INS_V1_BASE = 0x40,
    INS_CONFIG_REQ,//1.d2g.0
    INS_LOCK_CHANNEL,//1.g2d.1
    INS_LOCK_CHANNEL_ACK,//1.d2g.2
    INS_AUTH_ALLOWED,//1.g2d.7
    INS_CHALLENGE_CLOUD,//2.d2g.0
	INS_CHALLENGE_CLOUD_ACK,//2.g2d.5
	INS_CHALLENGE_DEVICE_ACK,//3.d2g.0
	INS_AUTH_INFO_A,//4.g2d.a.5
	INS_AUTH_INFO_B,//4.g2d.b.5
	INS_CONNECT_AP_ACK,//x.d2g.0
	INS_DEVICE_BIND_REQ,//5.d2g.0
	INS_DEVICE_BIND_ACK,//5.g2d.3
} tc_ins_t;


typedef enum
{
	MSG_OK = 0x00,
	MSG_ERROR_REQ_DENIED,     //ÇëÇó¾Ü¾ø
	MSG_ERROR_TIMEOUT,        //³¬Ê±
	MSG_ERROR_VERIFY_FAILED, //ÈÏÖ¤Ê§°Ü
	MSG_ERROR_UNKNOWN = 0Xff//Î´Öª´íÎó
} tc_msg_value_t;

typedef enum{
	joy_cla_firstpacket,
	joy_cla_resendpacket
}tc_cla_type_t;


typedef enum{
	packet_init = 0,
	packet_resend
}tc_packet_type_t;

typedef struct tc_package {
	uint8_t oui_type;
	uint8_t magic[3];
	uint8_t cla;
	uint8_t ins;
	uint8_t p1;
	uint8_t p2;
	uint8_t lc;
	uint8_t *data;
	uint16_t crc;
}tc_package_t;

typedef struct{
	uint8_t *value;
	uint8_t length;
}deviceid_t;

typedef struct{
	uint8_t *value;
	uint8_t length;
}deviceinfo_t;

typedef struct{
	uint8_t *value;
	uint8_t length;
}tc_vl_t;


typedef struct{
	uint8_t *value;
	uint8_t length;
}tc_random_t;

typedef struct{
	uint8_t *value;
	uint8_t length;
}tc_sig_t;

typedef struct{
	uint8_t *value;
	uint8_t length;
}tc_pubkey_t;

#endif


