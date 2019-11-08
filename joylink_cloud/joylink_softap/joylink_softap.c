/*************************************

Copyright (c) 2015-2050, JD Smart All rights reserved. 

*************************************/
#include <string.h>
#include "joylink_softap.h"
#include "joylink3_auth_uECC.h"
#include "joylink_softap_util.h"
#include "joylink_aes.h"
// #include "joylink_softap_start.h"
#include "joylink_log.h"
#include "joylink_utils.h"
#include "joylink_auth_crc.h"
#include "joylink_dev_active.h"
#include "joylink_cloud_log.h"

static uint8 joylink_softap_getCrc8(uint8 *ptr, uint8 len);
static int joylink_softap_byte2hexstr(const uint8 *pbytes, int blen, uint8 *o_phex, int hlen);
static int joylink_softap_aes_decrypt(uint8 *pEncIn, uint32 encLength,uint8 *key_32bytes, uint8 *pPlainOut, uint32 maxPlainLength);
static int joylink_softap_aes_encrypt(uint8 *pEncIn, int32 encLength,uint8 *key_32bytes, uint8 *pPlainOut, uint32 maxPlainLength);
int joylink_softap_generate_rng(uint8 *buf,unsigned size);
static void joylink_softap_ssid_generate(void);
static uint16 joylink_softap_crc16(const uint8 *buffer, uint32 size);

joylinkSoftAPRam joy_softap_ram = {0};					
uint8 softap_ssid[MAX_LEN_OF_SSID+1]  = {0};			//beacon ssid

/**
 * @name:get_random 
 *
 * @returns:   
 */
int get_random(void)
{
    static unsigned long int next = 1;
    next = next *1103515245 + 12345;
    return (int)(next/65536) % (1134);
}

/**
 * @name:get_mac_address 
 *
 * @param: address
 * @param: len
 */
void get_mac_address(uint8 *address,uint8 len)
{
	memset(address, 0, len);
	address[0] = 0xaa;
	address[1] = 0xbb;
	address[2] = 0xcc;
	address[3] = 0xdd;
	address[4] = 0xee;
	address[5] = 0xff;
}

/**
 * @name:joylink_softap_init 
 *
 * @returns:   
 */
int joylink_softap_init(void)
{
	joylink_softap_ssid_generate();
	return TRUE;
}
#ifdef _IS_DEV_REQUEST_ACTIVE_SUPPORTED_
/**
 * @name:joylink_softap_is_need_active 
 *
 * @returns:   
 */
int joylink_softap_is_need_active(void)
{
	return joy_softap_ram.is_need_active;
}


/**
 * @name:joylink_softap_active_clear 
 *
 * @returns:   
 */
int joylink_softap_active_clear(void)
{
	joy_softap_ram.is_need_active = 0;
#ifdef ESP_PLATFORM
	return 0;
#endif
}

#endif

/**
 * @name:joylink_softap_result 
 *
 * @param: pRet
 *
 * @returns:   
 */
int  joylink_softap_result(joylinkSoftAP_Result_t* pRet)
{
	if(joy_softap_ram.status == SOFTAP_SUCCESS)
	{
        	memcpy(pRet,&(joy_softap_ram.softap_result),sizeof(joy_softap_ram.softap_result));
		return 0;
	}else{
		return -1;
	}
}


/**
 * @name:joylink_softap_udpbroad 
 *
 * @param: socket_fd
 *
 * @returns:   
 */
int joylink_softap_udpbroad(int socket_fd)
{
	static uint8 publickey_packet_buf[SOFTAP_BROAD_PUBKEY_PACKETLEN];
	int status = 0;
	uint8 *pdata = publickey_packet_buf;
	
	jl3_uECC_set_rng(joylink_softap_generate_rng);			//register random function

	/*if we have not generate public and priviate key,generate key*/
	if(joy_softap_ram.is_generate_key == 0)
	{
		joy_softap_ram.ecc_curves = uECC_secp256r1();
		status = jl3_uECC_make_key(joy_softap_ram.ecc_publickey, joy_softap_ram.ecc_privatekey, joy_softap_ram.ecc_curves);
		if(status)
		{
			uint16 crc16;
			
			memcpy(pdata,SOFTAP_PACKET_HEAD,strlen(SOFTAP_PACKET_HEAD));
			*(pdata + SOFTAP_OFFSET_TYPE) 			= SOFTAP_TYPE_PUBKEY;
			*(pdata + SOFTAP_OFFSET_DATALEN) 		= LEN_PUBLICKEY_ECC_ZIP;
			
			jl3_uECC_compress(joy_softap_ram.ecc_publickey, pdata + SOFTAP_OFFSET_DATA, joy_softap_ram.ecc_curves);
			*(pdata + SOFTAP_OFFSET_DATA + LEN_PUBLICKEY_ECC_ZIP) = SOFTAP_FLAG_TCP_UDP;
			
			crc16 = CRC16(pdata,SOFTAP_BROAD_PUBKEY_PACKETLEN -2);
			memcpy(pdata+SOFTAP_BROAD_PUBKEY_PACKETLEN -2,&crc16,2);

			joy_softap_ram.is_generate_key = 1;
			
		}
		else
		{
			joy_softap_ram.is_generate_key = 0;
			log_error("generate public key and priviate key fail\n");
			return FALSE;
		}
		log_info("--->send pulickey packet to APP:\n");
		joylink_util_fmt_p("packet->",publickey_packet_buf,SOFTAP_BROAD_PUBKEY_PACKETLEN);
	}
	joylink_udp_broad_send(socket_fd, publickey_packet_buf, SOFTAP_BROAD_PUBKEY_PACKETLEN);

	return TRUE;
}

#ifdef DEV_SOFTAP_SSID
/**
 * @name:joylink_softap_ssid_generate 
 */
static void joylink_softap_ssid_generate(void)
{
	memset(softap_ssid, 0, MAX_LEN_OF_SSID+1);
	memcpy(softap_ssid, DEV_SOFTAP_SSID, strlen(DEV_SOFTAP_SSID));
}
#else

uint8 getCrc(uint8 *ptr, uint8 len)
{
	unsigned char crc;
	unsigned char i;
	crc = 0;
	while (len--)
	{
		crc ^= *ptr++;
		for (i = 0; i < 8; i++)
		{
			if (crc & 0x01)
			{
				crc = (crc >> 1) ^ 0x8C;
			}
			else
				crc >>= 1;
		}
	}
	return crc;
}

/**
 * @name:joylink_softap_ssid_generate 
 */
static void joylink_softap_ssid_generate(void)
{
	uint8 crc8;
	uint8 data_offset = 0,size=0;
	uint8 buf[6];
	
	/*packet head*/
	data_offset = 0;
	size = 4;
	memcpy(softap_ssid,SOFTAP_PACKET_HEAD,size);
	data_offset += size;
	/*BRAND*/
	size = 4;
	memcpy(softap_ssid+data_offset,SOFTAP_BRAND,size);
	data_offset += size;
	/*cid*/
	size = 4;
	memcpy(softap_ssid+data_offset,SOFTAP_CID,size);
	data_offset += size;

	/*PUID*/
	size = 6;
	memcpy(softap_ssid+data_offset,SOFTAP_UUID,size);
	data_offset += size;

	/*mac address*/
	size = 12;
	get_mac_address(buf,sizeof(buf));
	joylink_softap_byte2hexstr((const uint8 *)buf,6,softap_ssid+data_offset,size);
	data_offset +=size;

	crc8 = getCrc(softap_ssid,30);
	joylink_softap_byte2hexstr((const uint8 *)(&crc8),1,softap_ssid+data_offset,2);
}
#endif

/**
 * @name:joylink_GeandTx_r2packet 
 *
 * @param: socket_fd
 */
static void joylink_GeandTx_r2packet(int socket_fd)
{
	uint8 tx_buf[256+SOFTAP_LEN_PACKET_WITHOUTDATA+LEN_PUBLICKEY_ECC];
	uint16 crc16;
	int len_aesencrypt;
	uint8 *pdata = tx_buf,datalen;

	/*generate R2*/
	joylink_softap_generate_rng(joy_softap_ram.ecc_r2,LEN_R1R2_ECC);
	//packet head
	memcpy(pdata,SOFTAP_PACKET_HEAD,strlen(SOFTAP_PACKET_HEAD));

	//data type
	*(pdata + SOFTAP_OFFSET_TYPE) = SOFTAP_TYPE_DEVICEOUT_R2;
					
	//encrypt R2 using the r1 as the key.
	len_aesencrypt = joylink_softap_aes_encrypt(joy_softap_ram.ecc_r2,LEN_R1R2_ECC,joy_softap_ram.ecc_r1,pdata+SOFTAP_OFFSET_DATA,128);
	
	if(len_aesencrypt == -1)
	{
		log_error("Error:data length error when encrypt R2");
		return;
	}
	//Add publick key to the packet 
	datalen = len_aesencrypt;
	*(pdata + SOFTAP_OFFSET_DATALEN) = datalen;
	log_info("r1,r2");
	joylink_util_fmt_p("r1",joy_softap_ram.ecc_r1,LEN_R1R2_ECC);
	joylink_util_fmt_p("r2",joy_softap_ram.ecc_r2,LEN_R1R2_ECC);


	crc16 = CRC16(pdata,datalen + SOFTAP_LEN_PACKET_WITHOUTDATA -2);
	memcpy(pdata+datalen + SOFTAP_LEN_PACKET_WITHOUTDATA -2,&crc16,2);
	joylink_softap_socket_send(socket_fd, tx_buf,datalen + SOFTAP_LEN_PACKET_WITHOUTDATA);
	log_info("--->send R2 and pulickey packet to APP:\n");
	joylink_util_fmt_p("tx buf",tx_buf,datalen + SOFTAP_LEN_PACKET_WITHOUTDATA);
}

/**
 * @name:handle_rx_r1msg 
 *
 * @param: msg
 * @param: connt
 *
 * @returns:   
 */
static int handle_rx_r1msg(uint8 *msg, int16 connt)
{
	int ecc_status,len_aes_decrypt;
	uint8 datalen;
	datalen = *(msg + SOFTAP_OFFSET_DATALEN);

	log_info("Receive R1 and remote public key:%d\n",connt);

	//get adn de compress remote public key
	joy_softap_ram.ecc_curves = uECC_secp256r1();
	jl3_uECC_decompress(msg+SOFTAP_OFFSET_DATA + datalen - LEN_PUBLICKEY_ECC_ZIP,joy_softap_ram.ecc_publickey_remote,joy_softap_ram.ecc_curves);


	joy_softap_ram.ecc_curves = uECC_secp256r1();

	//generate shared key
	ecc_status = jl3_uECC_shared_secret(joy_softap_ram.ecc_publickey_remote, joy_softap_ram.ecc_privatekey, joy_softap_ram.shared_key, joy_softap_ram.ecc_curves);
	log_info(">>>>>>>>ecc_status = %d",ecc_status);

	log_info("generate sharekey,privatekey, remote publicky,sharekey");
	joylink_util_fmt_p("prikey",joy_softap_ram.ecc_privatekey,LEN_PRIVIATEKEY_ECC);
	joylink_util_fmt_p("remote pubkey",joy_softap_ram.ecc_publickey_remote,LEN_PUBLICKEY_ECC);
	joylink_util_fmt_p("sharekey",joy_softap_ram.shared_key,LEN_SHAREDKEY_LEN);
	/*decrypt R1*/
	len_aes_decrypt = joylink_softap_aes_decrypt(msg + SOFTAP_OFFSET_DATA,datalen - LEN_PUBLICKEY_ECC_ZIP,joy_softap_ram.shared_key,joy_softap_ram.ecc_r1,LEN_R1R2_ECC);

	if(len_aes_decrypt != LEN_R1R2_ECC)
	{
		log_error("Error:decrypt R1 data length error\n");
		return FALSE;
	}
	log_info("The decrypt R1 data:\n");
	joylink_util_fmt_p("R1->",joy_softap_ram.ecc_r1,LEN_R1R2_ECC);

	joy_softap_ram.is_stop_broadcast_pubkey = 1;
	return TRUE;
}

/**
 * @name:handle_rx_ssidmsg 
 *
 * @param: msg
 * @param: connt
 * @param: result_return
 *
 * @returns:   
 */
static int handle_rx_ssidmsg(uint8 *msg, int16 connt,joylinkSoftAP_Result_t *result_return)
{
//	joylinkSoftAP_Result_t result = {0};
	uint8 size_ssid,size_pass,offset = 0;
#ifdef _IS_DEV_REQUEST_ACTIVE_SUPPORTED_
	uint8 size_url,size_token;
	char url[LEN_URL_MAX+1] = {0},token[LEN_TOKEN_MAX+1] = {0};
#endif
	
	uint8 sessionkey[LEN_R1R2_ECC],i,decrypt_data[128];
	int len_aes_decrypt;
	
	/*generate session key*/
	for(i=0;i<LEN_R1R2_ECC;i++)
		sessionkey[i] = joy_softap_ram.ecc_r1[i] ^ joy_softap_ram.ecc_r2[i];

	log_info("Generate the session key:\n");
	joylink_util_fmt_p("sessionkey",sessionkey,LEN_R1R2_ECC);
	log_info("endata,len = 0x%2X\n",*(msg+SOFTAP_OFFSET_DATALEN));
	joylink_util_fmt_p("endata->",msg + SOFTAP_OFFSET_DATA,*(msg+SOFTAP_OFFSET_DATALEN));
	
	/*decrypt the data*/
	len_aes_decrypt = joylink_softap_aes_decrypt(msg + SOFTAP_OFFSET_DATA,*(msg+SOFTAP_OFFSET_DATALEN),sessionkey,decrypt_data,128);

	if(len_aes_decrypt > (MAX_LEN_OF_SSID + MAX_LEN_OF_PASSWORD + LEN_URL_MAX + LEN_TOKEN_MAX + 4))
	{
		log_error("Error:SSID and pass data length error!\n");
		return FALSE;
	}
	
	if(len_aes_decrypt<0)
		return FALSE;

	/*Analyze the ssid and pass data*/
	size_ssid = decrypt_data[offset++];
	size_pass = decrypt_data[offset++];
	if((size_ssid > (MAX_LEN_OF_SSID + 1)) || (size_pass > (MAX_LEN_OF_PASSWORD + 1)))
	{
		/*size error*/
		log_error("Error:ssid or pass too long\n");
		return FALSE;	
	}
	memset(result_return,0,sizeof(joylinkSoftAP_Result_t));
	memcpy(result_return->ssid,decrypt_data + offset,size_ssid);
	offset+=size_ssid;
	memcpy(result_return->pass,decrypt_data + offset,size_pass);
	offset+=size_pass;
	log_info("ssid %s,key: %s\r\n",result_return->ssid,result_return->pass);

#ifdef _IS_DEV_REQUEST_ACTIVE_SUPPORTED_
	if(offset >= (len_aes_decrypt)){
		log_error("no url and token received");
		return TRUE;
	}
	
	size_url = decrypt_data[offset++];
	if(size_url > (LEN_URL_MAX + 1)){
		log_error("size of url maxer");
		return FALSE;
	}
	memset(url,0,sizeof(url));
	memcpy(url,decrypt_data + offset,size_url);
	log_info("get url->%s",url);
	offset+=size_url;

	memset(token,0,sizeof(token));
	
	size_token = decrypt_data[offset++];
	if(size_token > (LEN_TOKEN_MAX + 1)){
		log_error("size of token maxer,rev len=0x%02X",size_token);
		return FALSE;
	}
	
	memcpy(token,decrypt_data + offset, size_token);
	log_info("get token->%s",token);
	joylink_dev_active_param_set(url,token);
	joylink_cloud_log_param_set(url,token);
	joy_softap_ram.is_need_active = 1;
#endif	

	return TRUE;
}

/**
 * @name:joylink_tx_softap_result 
 *
 * @param: socket_fd
 * @param: result
 */
static void joylink_tx_softap_result(int socket_fd, uint8 result)
{
	uint8 tx_buf[128+8],*pdata,datalen;
	pdata = tx_buf;
	uint16 crc16;
	int len_aesencrypt;
	
	memcpy(pdata,SOFTAP_PACKET_HEAD,strlen(SOFTAP_PACKET_HEAD));
	*(pdata + SOFTAP_OFFSET_TYPE) 			= SOFTAP_TYPE_DEVICEOUT_RESULT;

	if(result == TRUE)
	{
		uint8 res[] = "0success";
		res[0] = 0x00;
		//encrypt R2 using the r1 as the key.
		len_aesencrypt = joylink_softap_aes_encrypt(res,sizeof(res),joy_softap_ram.sessionkey,pdata+SOFTAP_OFFSET_DATA,128);
		if(len_aesencrypt == -1)
		{
			log_error("Error:data length error when encrypt R2");
			return;
		}
		datalen = len_aesencrypt;
	}
	else
	{
		uint8 resf[] = "1failed";
		resf[0] = 0x01;
		//encrypt R2 using the r1 as the key.
		len_aesencrypt = joylink_softap_aes_encrypt(resf,sizeof(resf),joy_softap_ram.sessionkey,pdata+SOFTAP_OFFSET_DATA,128);
		if(len_aesencrypt == -1)
		{
			log_error("Error:data length error when encrypt R2");
			return;
		}
		datalen = len_aesencrypt;
	}
	*(pdata +SOFTAP_OFFSET_DATALEN) = datalen;
	
	crc16 = joylink_softap_crc16(pdata,datalen + SOFTAP_LEN_PACKET_WITHOUTDATA -2);

	memcpy(pdata+datalen + SOFTAP_LEN_PACKET_WITHOUTDATA -2,&crc16,2);
	joylink_softap_socket_send(socket_fd, tx_buf,datalen + SOFTAP_LEN_PACKET_WITHOUTDATA);
	log_info("send the result to APP\n");
}

/**
 * @name:joylink_softap_data_packet_handle 
 *
 * @param: socket_fd
 * @param: msg
 * @param: count
 *
 * @returns:   
 */
int joylink_softap_data_packet_handle(int socket_fd, uint8 *msg, int16 count)
{
	int ret = FALSE;
	uint8 type,datalen;
	uint16 crc_checksum,crc_packet;
	joylink_util_fmt_p("Receive data packet:\n",msg,count);
	
	if(memcmp(msg,SOFTAP_PACKET_HEAD,strlen(SOFTAP_PACKET_HEAD)) == 0)
	{
		datalen = *(msg +SOFTAP_OFFSET_DATALEN);
		memcpy(&crc_packet, msg+datalen + SOFTAP_LEN_PACKET_WITHOUTDATA -2,2);
		crc_checksum = joylink_softap_crc16(msg,datalen + SOFTAP_LEN_PACKET_WITHOUTDATA -2);
		if(crc_checksum != crc_packet)
		{
			log_error("CRC error:check = %d;receive = %d\n", crc_checksum, crc_packet);
			return FALSE;
		}
		type 		= *(msg + SOFTAP_OFFSET_TYPE);
		switch(type)
		{
			case SOFTAP_TYPE_DEVCIEIN_R1:
				if(TRUE == handle_rx_r1msg(msg,count)){
					joylink_GeandTx_r2packet(socket_fd);
					return TRUE;
				}else{
					joylink_tx_softap_result(socket_fd,FALSE);
					return FALSE;
				}
				break;
			case SOFTAP_TYPE_DEVICEIN_SSIDPASS:
			{
				int handle_result;
				
				handle_result = handle_rx_ssidmsg(msg,count,&joy_softap_ram.softap_result);

				if(handle_result == TRUE)
				{
					joylink_tx_softap_result(socket_fd, TRUE);
					sleep(1);
					joy_softap_ram.status = SOFTAP_SUCCESS;
					log_info("Receive the ssid and pass success\n");
					return TRUE;
				}
				else
				{
					joy_softap_ram.status = SOFTAP_FAIL;
					joylink_tx_softap_result(socket_fd, FALSE);
					sleep(1);
					log_error("Error:receive ssid and password wrong!\n");
					return FALSE;
				}
			}
			break;
		default:
			log_error("Error: Type error!\n");
			return FALSE;
			break;
	}
	}
	return TRUE;
}

/**
 * @name:joylink_softap_getCrc8 
 *
 * @param: ptr
 * @param: len
 *
 * @returns:   
 */
static uint8 joylink_softap_getCrc8(uint8 *ptr, uint8 len)
{
	unsigned char crc;
	unsigned char i;
	crc = 0;
	while (len--)
	{
		crc ^= *ptr++;
		for (i = 0; i < 8; i++)
		{
			if (crc & 0x01)
			{
				crc = (crc >> 1) ^ 0x8C;
			}
			else
				crc >>= 1;
		}
	}
	return crc;
}



/**
 * @name:joylink_softap_crc16 
 *
 * @param: buffer
 * @param: size
 *
 * @returns:   
 */
static uint16 joylink_softap_crc16(const uint8 *buffer, uint32 size)
{
	uint16 checksum = 0xFFFF;  

	if (buffer && size){
		while (size--) {
			checksum = (checksum >> 8) | (checksum << 8);
			checksum ^= *buffer++;
			checksum ^= ((unsigned char) checksum) >> 4;
			checksum ^= checksum << 12;
			checksum ^= (checksum & 0xFF) << 5;
		}
	}
	
	return checksum;
}


/**
 * @name:joylink_softap_byte2hexstr 
 *
 * @param: pbytes
 * @param: blen
 * @param: o_phex
 * @param: hlen
 *
 * @returns:   
 */
static int joylink_softap_byte2hexstr(const uint8 *pbytes, int blen, uint8 *o_phex, int hlen)
{
	const char tab[] = "0123456789abcdef";
	int i = 0;

	memset(o_phex, 0, hlen);
	if(hlen < blen * 2){
		blen = (hlen - 1) / 2;
    }

	for(i = 0; i < blen; i++){
		*o_phex++ = tab[*pbytes >> 4];
		*o_phex++ = tab[*pbytes & 0x0f];
		pbytes++;
	}
	*o_phex++ = 0;

	return blen * 2;
}

/**
 * @name:joylink_softap_aes_decrypt 
 *
 * @param: pEncIn
 * @param: encLength
 * @param: key_32bytes
 * @param: pPlainOut
 * @param: maxPlainLength
 *
 * @returns:   
 */
static int joylink_softap_aes_decrypt(uint8 *pEncIn, uint32 encLength,uint8 *key_32bytes, uint8 *pPlainOut, uint32 maxPlainLength)
{
	uint8 key[16] = { 0 };
	uint8 iv[16] = { 0 };
	
	memcpy(iv,key_32bytes,16);
	memcpy(key,key_32bytes+16, 16);
	uint8 Out[128] = { 0 };

	uint32 ret = 128;
	ret = device_aes_decrypt_entire_iv(key, 16, iv, pEncIn, encLength, Out, sizeof(Out));
	if(ret > maxPlainLength)
	{
		log_error("decrypt data max than expect:%d,%d\n",ret,maxPlainLength);
		return 0;
	}

	log_info("decrypt data len :%x",ret);
	memcpy(pPlainOut, Out, ret);
	return ret;
	
}

/**
 * @name:joylink_softap_aes_encrypt 
 *
 * @param: pEncIn
 * @param: encLength
 * @param: key_32bytes
 * @param: pPlainOut
 * @param: maxPlainLength
 *
 * @returns:   
 */
static int joylink_softap_aes_encrypt(uint8 *pEncIn, int32 encLength,uint8 *key_32bytes, uint8 *pPlainOut, uint32 maxPlainLength)
{
	uint8 key[16] = { 0 };
	uint8 iv[16] = { 0 };
	memcpy(iv,key_32bytes,16);
	memcpy(key,key_32bytes+16, 16);
	uint32 ret = maxPlainLength;
	uint8 Out[128] = { 0 };

	ret = device_aes_encrypt_entire_iv(key, 16,iv, pEncIn,encLength, Out, sizeof(Out));
	if(ret > maxPlainLength)
	{
		log_error("encrypt Error\n");
		return -1;
	}
	
	memcpy(pPlainOut, Out, ret);
	
	return ret;
}

/**
 * @name:joylink_softap_generate_rng 
 *
 * @param: buf
 * @param: size
 *
 * @returns:   
 */
int joylink_softap_generate_rng(uint8 *buf,unsigned size)
{
	unsigned i;
	
	for(i = 0;i < size; i++){
		*(buf+i) = (uint8_t)get_random();
	}
	
	return 1;
}

