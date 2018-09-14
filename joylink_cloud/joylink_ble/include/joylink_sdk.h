#ifndef JoyLink_SDK_h
#define JoyLink_SDK_h
#include "joylink_syshdr.h"

/******************************************************
 *                     Structures
 ******************************************************/
#pragma pack(1)

typedef struct{
	uint8_t count;
	uint8_t num;
	uint8_t keytype;
	uint8_t seq;
}frame_hdr_t;

typedef struct{
	frame_hdr_t hdr;
	uint8_t payload[16];
}jl_frame_t;

typedef  struct 
{
    uint8_t seq;
    uint8_t operation;
    uint16_t len;
    uint8_t content[1];
    //uint16_t crc;   //because content's length is not solid
}jl_packet_t;

typedef struct{
	uint16_t tag;
	uint8_t len;
	uint8_t value[1];
}jl_property_t;    //tlv

#pragma pack()

//packet offset
#define JOY_OFFSET_FULLPACKET_SEQ			0
#define JOY_OFFSET_FULLPACKET_OPERATE		(JOY_OFFSET_FULLPACKET_SEQ + 1)
#define JOY_OFFSET_FULLPACKET_LENGTH		(JOY_OFFSET_FULLPACKET_OPERATE + 1)
#define JOY_OFFSET_FULLPACKET_CONTENT		(JOY_OFFSET_FULLPACKET_LENGTH + 2)

//operate
#define JOY_OPERATE_ID_PRD					0x01	//phone read device
#define JOY_OPERATE_ID_DRP					0x11	//data return data to phone
#define JOY_OPERATE_ID_PWD_WITHNORES		0x02	//phone write device without response
#define JOY_OPERATE_ID_PWD_WITHRES			0x03	//phone write device with response
#define JOY_OPERATE_ID_DRWRES				0x13    //device return response of write cmd

#define JOY_OPERATE_ID_DTP_WITHNORES		0x16	//device send data to phone without response
#define JOY_OPERATE_ID_DTP_WITHRES			0x17	//device send data to phone with response
#define JOY_OPERATE_ID_PTD_RES				0x07	//phone return response to device

//property
#define JOY_PROPERTY_TAG_PUID				0xFFFF
#define JOY_PROPERTY_TAG_GUID				0xFFFC
#define JOY_PROPERTY_TAG_LOCALKEY			0xFFFB
#define JOY_PROPERTY_TAG_DEV_SNAPSHOT		0xFEFF
#define JOY_PROPERTY_TAG_PUBKEY_APP			0xFEF9
#define JOY_PROPERTY_TAG_PUBKEY_DEV			0xFEF8
#define JOY_PROPERTY_TAG_SSID				0xFEF7
#define JOY_PROPERTY_TAG_PASSWORD			0xFEF6
#define JOY_PROPERTY_TAG_SECLEVEL			0xFEF5
#define JOY_PROPERTY_TAG_BRAND				0xFEF4
#define JOY_PROPERTY_TAG_CID				0xFEF3
#define JOY_PROPERTY_TAG_BLE_DEV_CTL		0xFEF2
#define JOY_PROPERTY_TAG_WIFI_STATUS		0xFEF1

//BLE service adn charicteristic UUID
#define GATTS_SERVICE_UUID_JD_PROFILE   		0xFE70
#define GATTS_CHAR_UUID_WRITE_JD_PROFILE    	0xFE71
#define GATTS_CHAR_UUID_INDICATIE_JD_PROFILE    0xFE72
#define GATTS_CHAR_UUID_READ_JD_PROFILE      	0xFE73

enum
{
	OP_APP_READ = 0x01,
	OP_DEV_RESP_READ = 0x11,
	OP_APP_WRITE_WITHOUT_RESP = 0x02,
	OP_APP_WRITE_WITH_RESP = 0x03,
	OP_DEV_RESP_WRITE = 0x13,
	OP_DEV_WRITE_WITHOUT_RESP = 0x16,
	OP_DEV_WRITE_WITH_RESP = 0x17,
	OP_APP_RESP_WRITE = 0x07,
	OP_ERROR_ACK = 0XFF
};

enum
{
	KEY_TYPE_PLAIN = 0x00,
	KEY_TYPE_PSK = 0x01,
	KEY_TYPE_ECDH = 0x2
};

int jl_init(uint8_t* zoneSend, uint16_t send_size, uint8_t* zoneRcv, uint16_t rcv_size);
int jl_rcv_reset(void);
int jl_send_reset(void);
int jl_receive(uint8_t* frame);
int jl_send_seclevel_0(uint16_t length, uint8_t* privData);
int jl_send_seclevel_1(uint16_t length, uint8_t* privData);
int jl_send_seclevel_2(uint16_t length, uint8_t* privData);

int jl_indication_confirm_cb(void);

int jl_save_guid(unsigned char *guid);
int jl_get_guid(unsigned char *guid);
int jl_save_localkey(unsigned char *localkey);
int jl_secure_prepare(void);
uint16_t jl_calc_crc16(const uint8_t *buffer, uint32_t size);

/*test code*/
void test_jl_get_secretkey(unsigned char *l_secretkey);
void test_jl_get_pid(unsigned char *l_pid);
void test_jl_get_mac(unsigned char *l_mac);


#define ERR_NUM_UNORDER  (-201)
#define ERR_NUM_OVERFLOW (-202)
#define ERR_UNSUPPORT_KEY (-203)

#define RECEIVE_WAIT 0
#define RECEIVE_END   1
#endif

