/*************************************

Copyright (c) 2015-2050, JD Smart All rights reserved. 

*************************************/

#include "joylink_util.h"
#include "joylink_softap_extern.h"

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

uint16 joylink_auth_crc16(const uint8 *buffer, uint32 size)
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


void dump8(uint8* p, int split, int len)
{
	int i;
	char buf[512];
	int index = 0;
	for (i = 0; i < len; i++)
	{
		if (split != 0 && ((i + 1) % split) == 0)
		{
			index += sprintf(buf + index, "%02x,", p[i]);
		}
		else
			index += sprintf(buf + index, "%02x ", p[i]);
	}
	printf_high("Addr=%d,Len=%d:%s\n", p, len, buf);
}

int joylink_util_byte2hexstr(const uint8 *pbytes, int blen, uint8 *o_phex, int hlen)
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


int joylink_util_make_guid(uint8 *guid_str, uint8 max_len)
{
	guid_t guid;	
	int	guid_len = -1;
	uint64 timestamp = 0UL;
	uint16 clock_seq = 0;
	uint8 mac_addr[6] = {0x90,0x65,0xF9,0x16,0xB6,0x25};
	
	uint8 guid_hex[16] = { 0 };


	get_mac_address(mac_addr,sizeof(mac_addr));//get the mac address
	
	if(guid_str && max_len){
		/*not used here for temp delete*/
		#if 0
		timestamp =	joylink_util_get_utc_ns();
		guid.time_low = (unsigned int)(timestamp & 0xFFFFFFFF);
		guid.time_mid = (unsigned short)((timestamp >> 32) & 0xFFFF);
		guid.time_hi_and_version = (unsigned short)((timestamp >> 48) & 0x0FFF);
		guid.time_hi_and_version |= (0x4A << 12);
		#endif
		
		//joylink_util_get_host_mac(mac_addr, sizeof(mac_addr));
		
		clock_seq = (uint16)apiRand();
		guid.clock_seq_low = clock_seq & 0xFF;
		guid.clock_seq_hi_and_reserved = (clock_seq & 0x3F00) >> 8;
		guid.clock_seq_hi_and_reserved |= 0x80;
		memcpy(&guid.node, &mac_addr, sizeof(guid.node));		
		
		memcpy(guid_hex, &guid, sizeof(guid_hex));				
		guid_len = joylink_util_byte2hexstr(guid_hex, 16, guid_str, max_len);
	}
	
	return guid_len;
}

int joylink_AesCBC256Decrypt(uint8 *pEncIn, uint32 encLength,uint8 *key_32bytes, uint8 *pPlainOut, uint32 maxPlainLength)
{
	uint8 key[16] = { 0 };
	uint8 iv[16] = { 0 };

	printf_high("aes encrypt_enclen = %x:\n",encLength);
	print_buf(pEncIn,encLength);

	
	memcpy(iv,key_32bytes,16);
	memcpy(key,key_32bytes+16, 16);
	//uint8 Out[128] = { 0 };
	uint8 Out[128] = { 0 };

	uint32 ret = 128;
	aes_cbc_decrypt(pEncIn, encLength, key, 16, iv, 16, Out, &ret);
	if(ret > maxPlainLength)
	{
		printf_high("decrypt data max than expect:%d,%d\n",ret,maxPlainLength);
		return 0;
	}

	printf_high("decrypt data len :%x",ret);
	memcpy(pPlainOut, Out, ret);
	return ret;
	
}

int joylink_AesCBC256Encrypt(uint8 *pEncIn, int32 encLength,uint8 *key_32bytes, uint8 *pPlainOut, uint32 maxPlainLength)
{
	uint8 key[16] = { 0 };
	uint8 iv[16] = { 0 };
	memcpy(iv,key_32bytes,16);
	memcpy(key,key_32bytes+16, 16);
	uint32 ret = maxPlainLength;
	uint8 Out[128] = { 0 };

	aes_cbc_encrypt(pEncIn,encLength,key,16,iv,16,Out,&ret);
	
	if(ret > maxPlainLength)
	{
		printf_high("encrypt Error\n");
		return -1;
	}
	
	memcpy(pPlainOut, Out, ret);
	
	return ret;
}

uECC_RNG_Function	uECC_generate_rng(uint8 *buf,unsigned size)
{
	unsigned i;
	
	for(i = 0;i<size;i++)
		*(buf+i)=(uint8_t)apiRand();
	
	return 1;
}


