#ifndef _ESP_JOYLINK_SOTEAP_EXTERN_H_
#define _ESP_JOYLINK_SOTEAP_EXTERN_H_

#include "esp_common.h"

/**/
#define SOFTAP_PACKET_HEAD				"JYAP"					//packet head
#define	SOFTAP_PUID						"GNQIYS"				//puid
#define SOFTAP_BRAND					"38C4"					//brand
#define SOFTAP_CID						"09A5"					//cid

//#define printf_high		printf		//the printf function

#define SOFTAP_GATEWAY_IP         "192.168.1.1"
#define SOFTAP_TCP_SERVER_PORT    (3000)
#define UDP_LOCAL_PORT            (4320)
#define UDP_REMOTE_PORT           (9999)

#define joylink_assert(val) do{\
    if(!(val)){os_printf("[ERROR][%s##%u]\n",__FILE__,__LINE__);}\
}while(0)

/*
Routine Description:
    udp send function which is provided by the system.
Arguments:
    data ----data address of tx
    len	 ----data len of tx
Return Value:
    none
Note:
    this function is used to send udp message used by joylink files.
*/
int joylink_udp_send(const void *data, uint16 len);
/*
Routine Description:
    tcp send function which is provided by the system.
Arguments:
    data ----data address of tx
    len	 ----data len of tx
Return Value:
    none
Note:
    this function is used to send tcp message used by joylink files.
*/
void joylink_tcp_send(const void *data, uint16 len);

void tcp_send(const void *data, uint16 len);

void print_buf(uint8 *buf,uint8 size);
/*
Routine Description:
    AES-CBC encryption

Arguments:
    PlainText        Plain text
    PlainTextLength  The length of plain text in bytes
    Key              Cipher key, it may be 16, 24, or 32 bytes (128, 192, or 256 bits)
    KeyLength        The length of cipher key in bytes
    IV               Initialization vector, it may be 16 bytes (128 bits)
    IVLength         The length of initialization vector in bytes
    CipherTextLength The length of allocated cipher text in bytes

Return Value:
    CipherText           Return cipher text
    CipherTextLength Return the length of real used cipher text in bytes   (including padding)
                               if PlainTextLength=16*n,   CipherTextLength=16*(n+1)
                               if PlainTextLength=16*(n-1) + x (0<x<16),    CipherTextLength = 16*n

Note:
*/
void aes_cbc_encrypt (uint8 PlainText[],uint32 PlainTextLength,uint8 Key[],uint32 KeyLength,uint8 IV[],uint32 IVLength,uint8 CipherText[],uint32 *CipherTextLength);

/*
Routine Description:
    AES-CBC decryption

Arguments:
    CipherText       Cipher text
    CipherTextLength The length of cipher text in bytes
    Key              Cipher key, it may be 16, 24, or 32 bytes (128, 192, or 256 bits)
    KeyLength        The length of cipher key in bytes
    IV               Initialization vector, it may be 16 bytes (128 bits)
    IVLength         The length of initialization vector in bytes
    PlainTextLength  The length of allocated plain text in bytes

Return Value:
    PlainText        Return plain text
    PlainTextLength  Return the length of real used plain text in bytes

Note:
*/
void aes_cbc_decrypt (uint8 CipherText[],uint32 CipherTextLength,uint8 Key[],uint32 KeyLength,uint8 IV[],uint32 IVLength,uint8 PlainText[],uint32 *PlainTextLength);

/*
Routine Description:
    get the device mac address

Arguments:
	address---the address of the back mac address
	len	   ---buffer len

Return Value:
	address--the mac address

Note:
*/
void get_mac_address(uint8 *address,uint8 len);

/*
Routine Description:
    get a random data

Arguments:
	none

Return Value:
	a random data
Note:
*/
uint32 apiRand (void);

/*
Routine Description:
    start softap config

Arguments:
	none

Return Value:
	none
Note:
*/
void joylink_softap_innet(void);

#endif
