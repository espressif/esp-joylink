/*************************************

Copyright (c) 2015-2050, JD Smart All rights reserved. 

*************************************/
#include "joylink_softap.h"
#include "joylink3_auth_uECC.h"
#include "joylink_softap_util.h"
#include "esp_common.h"
#include "joylink_config.h"
#include "joylink_log.h"

static void joylink_ge_ssid(void);
void print_buf(uint8 *buf,uint8 size);

joylinkSoftAPRam 	joy_softap_ram 			= {0};					
uint8				softap_ssid[MAX_LEN_OF_SSID+1]  = {0};			//beacon ssid

int joylink_softap_init(void)
{
	joylink_ge_ssid();
	return TRUE;
}

int  joylink_softap_result(joylinkSoftAP_Result_t* pRet)
{
	if(joy_softap_ram.status == SOFTAP_SUCCESS)
	{
        memcpy(pRet,&(joy_softap_ram.softap_result),sizeof(joy_softap_ram.softap_result));
		return 0;
	}
	else 
		return 1;
}

int joylink_softap_udpbroad(void)
{
	static uint8 publickey_packet_buf[SOFTAP_BROAD_PUBKEY_PACKETLEN];
	int status = 0;
	uint8 *pdata = publickey_packet_buf;

	jl3_uECC_set_rng((uECC_RNG_Function)joylink_softap_uECC_generate_rng);			//register random function

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
			
			//memcpy(pdata + SOFTAP_OFFSET_DATA,joy_softap_ram.ecc_publickey,LEN_PUBLICKEY_ECC);
		#if IS_PACKEY_WITH_CRC
			crc16 = joylink_softap_auth_crc16(pdata,SOFTAP_BROAD_PUBKEY_PACKETLEN -2);
			memcpy(pdata+SOFTAP_BROAD_PUBKEY_PACKETLEN -2,&crc16,2);
		#else
			//crc16 = joylink_softap_auth_crc16(pdata,SOFTAP_BROAD_PUBKEY_PACKETLEN -2);
			crc16 = 0xA55A;
			memcpy(pdata+SOFTAP_BROAD_PUBKEY_PACKETLEN -2,&crc16,2);
		#endif
			joy_softap_ram.is_generate_key = 1;
			
		}
		else
		{
			joy_softap_ram.is_generate_key = 0;
			log_error("generate public key and priviate key fail");
			return FALSE;
		}
		log_debug("send pulickey packet to APP:");
		print_buf(publickey_packet_buf,SOFTAP_BROAD_PUBKEY_PACKETLEN);
	}
	joylink_udp_send(publickey_packet_buf, SOFTAP_BROAD_PUBKEY_PACKETLEN);

	return TRUE;
}

static void joylink_ge_ssid(void)
{
	uint8 crc8;
	uint8 data_offset = 0,size=0;
	uint8 buf[6];
	
	/*packet head*/
	data_offset = 0;
	size		= 4;
	memcpy(softap_ssid,SOFTAP_PACKET_HEAD,size);
	data_offset += size;
	/*BRAND*/
	size		= 4;
	memcpy(softap_ssid+data_offset,JOYLINK_BRAND,size);
	data_offset += size;
	/*cid*/
	size		= 4;
	memcpy(softap_ssid+data_offset,JOYLINK_CID,size);
	data_offset += size;

	/*PUID*/
	size = 6;
	memcpy(softap_ssid+data_offset,JOYLINK_UUID,size);
	data_offset += size;

	/*mac address*/
	size = 12;
	get_mac_address(buf,sizeof(buf));
	joylink_softap_util_byte2hexstr((const uint8 *)buf,6,softap_ssid+data_offset,size);
	data_offset +=size;

#if IS_PACKEY_WITH_CRC
	crc8 = joylink_softap_getCrc(softap_ssid,30);
#else
	crc8 = 0x5A;
#endif
	joylink_softap_util_byte2hexstr((const uint8 *)(&crc8),1,softap_ssid+data_offset,2);
}

static void joylink_GeandTx_r2packet(void)
{
	uint8 tx_buf[256+SOFTAP_LEN_PACKET_WITHOUTDATA+LEN_PUBLICKEY_ECC];
	uint16 crc16;
	int len_aesencrypt;
	uint8 *pdata = tx_buf,datalen;

	/*generate R2*/
	joylink_softap_uECC_generate_rng(joy_softap_ram.ecc_r2,LEN_R1R2_ECC);
	//packet head
	memcpy(pdata,SOFTAP_PACKET_HEAD,strlen(SOFTAP_PACKET_HEAD));

	//data type
	*(pdata + SOFTAP_OFFSET_TYPE) 			= SOFTAP_TYPE_DEVICEOUT_R2;
					
#if IS_PACKET_WITH_ENDECRIPT
	//encrypt R2 using the r1 as the key.
	len_aesencrypt = joylink_softap_AesCBC256Encrypt(joy_softap_ram.ecc_r2,LEN_R1R2_ECC,joy_softap_ram.ecc_r1,pdata+SOFTAP_OFFSET_DATA,128);
	
	if(len_aesencrypt == -1)
	{
		log_debug("Error:data length error when encrypt R2");
		return;
	}
	//Add publick key to the packet 
	datalen = len_aesencrypt;
	*(pdata + SOFTAP_OFFSET_DATALEN)		= datalen;

#else
	datalen 								= LEN_R1R2_ECC+LEN_PUBLICKEY_ECC;
	*(pdata + SOFTAP_OFFSET_DATALEN) 		= datalen;
					
	memcpy(pdata + SOFTAP_OFFSET_DATA,joy_softap_ram.ecc_r2,LEN_R1R2_ECC);
	memcpy(pdata + SOFTAP_OFFSET_DATA + LEN_R1R2_ECC,joy_softap_ram.ecc_publickey,LEN_PUBLICKEY_ECC);

#endif

#if IS_PACKEY_WITH_CRC
	crc16 = joylink_softap_auth_crc16(pdata,datalen + SOFTAP_LEN_PACKET_WITHOUTDATA -2);
#else
	//crc16 = joylink_softap_auth_crc16(pdata,SOFTAP_BROAD_PUBKEY_PACKETLEN -2);
	crc16 = 0xA55A;
#endif
	memcpy(pdata+datalen + SOFTAP_LEN_PACKET_WITHOUTDATA -2,&crc16,2);
	joylink_tcp_send(tx_buf,datalen + SOFTAP_LEN_PACKET_WITHOUTDATA);
	log_debug("send R2 and pulickey packet to APP:");
	print_buf(tx_buf,datalen + SOFTAP_LEN_PACKET_WITHOUTDATA);
}

static int handle_rx_r1msg(uint8 *msg, int16 connt)
{
#if IS_PACKET_WITH_ENDECRIPT
	int ecc_status,len_aes_decrypt;
	uint8 datalen;
	datalen = *(msg + SOFTAP_OFFSET_DATALEN);

	log_debug("Receive R1 and remote public key:%d",connt);

	//get adn de compress remote public key
	joy_softap_ram.ecc_curves = uECC_secp256r1();
	jl3_uECC_decompress(msg+SOFTAP_OFFSET_DATA + datalen - LEN_PUBLICKEY_ECC_ZIP,joy_softap_ram.ecc_publickey_remote,joy_softap_ram.ecc_curves);


	joy_softap_ram.ecc_curves = uECC_secp256r1();

	//generate shared key
	ecc_status = jl3_uECC_shared_secret(joy_softap_ram.ecc_publickey_remote, joy_softap_ram.ecc_privatekey, joy_softap_ram.shared_key, joy_softap_ram.ecc_curves);

	/*decrypt R1*/
	len_aes_decrypt = joylink_softap_AesCBC256Decrypt(msg + SOFTAP_OFFSET_DATA,datalen - LEN_PUBLICKEY_ECC_ZIP,joy_softap_ram.shared_key,joy_softap_ram.ecc_r1,LEN_R1R2_ECC);

	if(len_aes_decrypt != LEN_R1R2_ECC)
	{
		log_error("Error:decrypt R1 data length error");
		return FALSE;
	}
	log_debug("The decrypt R1 data:");
	print_buf(joy_softap_ram.ecc_r1,LEN_R1R2_ECC);

#else 
	/*set the r1 and public key*/
	memcpy(joy_softap_ram.ecc_r1,msg+SOFTAP_OFFSET_DATA,LEN_R1R2_ECC);
	memcpy(joy_softap_ram.ecc_publickey_remote,msg+SOFTAP_OFFSET_DATA+LEN_R1R2_ECC,LEN_PUBLICKEY_ECC);
	log_debug("Receive R1 and remote public key");
	print_buf(joy_softap_ram.ecc_r1,LEN_R1R2_ECC);
	print_buf(joy_softap_ram.ecc_publickey_remote,LEN_PUBLICKEY_ECC);
#endif
	return TRUE;
}

static int handle_rx_ssidmsg(uint8 *msg, int16 connt,joylinkSoftAP_Result_t *result_return)
{
	joylinkSoftAP_Result_t result = {0};
	uint8 size_ssid,size_pass;
#if IS_PACKET_WITH_ENDECRIPT
	uint8 sessionkey[LEN_R1R2_ECC],i,decrypt_data[128];
	int len_aes_decrypt;
	
	/*generate session key*/
	for(i=0;i<LEN_R1R2_ECC;i++)
		sessionkey[i] = joy_softap_ram.ecc_r1[i] ^ joy_softap_ram.ecc_r2[i];

	log_debug("Generate the session key:");
	print_buf(sessionkey,LEN_R1R2_ECC);
	
	/*decrypt the data*/
	len_aes_decrypt = joylink_softap_AesCBC256Decrypt(msg + SOFTAP_OFFSET_DATA,*(msg+SOFTAP_OFFSET_DATALEN),sessionkey,decrypt_data,128);
	log_debug("decrpty_data len:%d :", len_aes_decrypt);
    print_buf(decrypt_data, len_aes_decrypt);  
	if(len_aes_decrypt > (SIZE_SSID + SIZE_PASSWORD + 2))
	{
		log_error("Error:SSID and pass data length error!");
		return FALSE;
	}

	/*Analyze the ssid and pass data*/
	size_ssid = decrypt_data[0];
	size_pass = decrypt_data[1];
	if((size_ssid > (SIZE_SSID+1)) || (size_pass > (SIZE_PASSWORD+1)))
	{
		/*size error*/
		log_error("Error:ssid or pass too long");
		return FALSE;	
	}
	
	memcpy(result.ssid,decrypt_data + 2,size_ssid);
	memcpy(result.pass,decrypt_data + 2 + size_ssid,size_pass);
    log_debug("recieved SSID:%s", result.ssid);
    log_debug("recieved PWD:%s", result.pass);
#else

	size_ssid = *(msg + SOFTAP_OFFSET_DATA);			//the size of ssid
	size_pass = *(msg + SOFTAP_OFFSET_DATA + 1);		//the size of password
	if((size_ssid > (SIZE_SSID+1)) || (size_pass > (SIZE_PASSWORD+1)))
	{
		/*size error*/
		log_error("Error:ssid or pass too long");
		return FALSE;	
	}
	
	memcpy(result.ssid,msg + SOFTAP_OFFSET_DATA + 2,size_ssid);
	memcpy(result.pass,msg + SOFTAP_OFFSET_DATA + 2 + size_ssid,size_pass);
#endif

	/*return the result*/
	*result_return = result;
	return TRUE;
}

static void joylink_tx_softap_result(uint8 result)
{
	uint8 tx_buf[128+8],*pdata,datalen;
	pdata = tx_buf;
	uint16 crc16;
	int len_aesencrypt;
	
	memcpy(pdata,SOFTAP_PACKET_HEAD,strlen(SOFTAP_PACKET_HEAD));
	*(pdata + SOFTAP_OFFSET_TYPE) 			= SOFTAP_TYPE_DEVICEOUT_RESULT;

	if(result == TRUE)
	{
		uint8 result[] = "0success";
		result[0] = 0x00;
		//encrypt R2 using the r1 as the key.
		len_aesencrypt = joylink_softap_AesCBC256Encrypt(result,sizeof(result),joy_softap_ram.sessionkey,pdata+SOFTAP_OFFSET_DATA,128);
		if(len_aesencrypt == -1)
		{
			log_error("Error:data length error when encrypt R2");
			return;
		}
		datalen = len_aesencrypt;
	}
	else
	{
		uint8 result[] = "1failed";
		result[0] = 0x01;
		//encrypt R2 using the r1 as the key.
		len_aesencrypt = joylink_softap_AesCBC256Encrypt(result,sizeof(result),joy_softap_ram.sessionkey,pdata+SOFTAP_OFFSET_DATA,128);
		if(len_aesencrypt == -1)
		{
			log_debug("Error:data length error when encrypt R2");
			return;
		}
		datalen = len_aesencrypt;
	}
	*(pdata +SOFTAP_OFFSET_DATALEN) = datalen;
	
#if IS_PACKEY_WITH_CRC
	crc16 = joylink_softap_auth_crc16(pdata,datalen + SOFTAP_LEN_PACKET_WITHOUTDATA -2);
#else
	crc16 = 0xA55A;
#endif

	log_debug("send the result to APP");
	memcpy(pdata+datalen + SOFTAP_LEN_PACKET_WITHOUTDATA -2,&crc16,2);
	joylink_tcp_send(tx_buf,datalen + SOFTAP_LEN_PACKET_WITHOUTDATA);
}

int joylink_softap_tcppacket(uint8 *msg, int16 count)
{
	uint8 type,datalen;
	uint16 crc_checksum,crc_packet;
	log_debug("Receive TCP packet:");
	print_buf(msg,count);
	
	if(memcmp(msg,SOFTAP_PACKET_HEAD,strlen(SOFTAP_PACKET_HEAD)) == 0)
	{

	#if IS_PACKEY_WITH_CRC
		datalen = *(msg +SOFTAP_OFFSET_DATALEN);
		memcpy(&crc_packet, msg+datalen + SOFTAP_LEN_PACKET_WITHOUTDATA -2,2);
		crc_checksum = joylink_softap_auth_crc16(msg,datalen + SOFTAP_LEN_PACKET_WITHOUTDATA -2);
		if(crc_checksum != crc_packet)
		{
			log_debug("CRC error:check = %x;receive = %x");
			return FALSE;
		}
	#endif
		type 		= *(msg + SOFTAP_OFFSET_TYPE);
		switch(type)
		{
			case SOFTAP_TYPE_DEVCIEIN_R1:
				if(TRUE == handle_rx_r1msg(msg,count))
					joylink_GeandTx_r2packet();
				break;
			case SOFTAP_TYPE_DEVICEIN_SSIDPASS:
			{
				int handle_result;
				
				handle_result = handle_rx_ssidmsg(msg,count,&joy_softap_ram.softap_result);

				if(handle_result == TRUE)
				{
					joylink_tx_softap_result(TRUE);
					joy_softap_ram.status = SOFTAP_SUCCESS;
					log_info("Receive the ssid and password success");
				}
				else
				{
					joy_softap_ram.status = SOFTAP_FAIL;
					joylink_tx_softap_result(FALSE);
					log_error("Error:receive ssid and password wrong!");
				}
			}
			break;
		default:
			log_error("Error: Type error!");
			break;
	}
	}
	return TRUE;
}

int joylink_softap_success(void)
{
    if (joy_softap_ram.status == SOFTAP_SUCCESS) {
        return TRUE;
    } else {
        return FALSE;
    }
}

void print_buf(uint8 *buf,uint8 size)
{
	uint8 i;
	for(i = 0;i < size; i++)
	{
		printf("%x ",*(buf+i));
	}
	printf("\n");
}


