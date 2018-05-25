#ifndef _JOYLINK_EXTERN_H_
#define _JOYLINK_EXTERN_H_

/**/
#define SOFTAP_PACKET_HEAD				"JYAP"					//packet head
#define	SOFTAP_PUID						"UJKK5C"				//puid
#define SOFTAP_BRAND					"38C4"					//brand 
#define SOFTAP_CID						"09A5"					//cid

#define printf_high		printf		//the printf function
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
extern void udp_send(const void *data, uint16 len);
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
extern void tcp_send(const void *data, uint16 len);

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
extern void aes_cbc_encrypt (uint8 PlainText[],uint32 PlainTextLength,uint8 Key[],uint32 KeyLength,uint8 IV[],uint32 IVLength,uint8 CipherText[],uint32 *CipherTextLength);

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
extern void aes_cbc_decrypt (uint8 CipherText[],uint32 CipherTextLength,uint8 Key[],uint32 KeyLength,uint8 IV[],uint32 IVLength,uint8 PlainText[],uint32 *PlainTextLength);

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
extern void get_mac_address(uint8 *address,uint8 len);

/*
Routine Description:
    get a random data

Arguments:
	none

Return Value:
	a random data
Note:
*/
extern uint32 apiRand (void);
#endif
