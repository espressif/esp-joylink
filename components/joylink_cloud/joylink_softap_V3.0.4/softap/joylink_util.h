/*************************************

Copyright (c) 2015-2050, JD Smart All rights reserved. 

*************************************/


#ifndef _JOYLINK_UTIL_H_
#define _JOYLINK_UTIL_H_
#include "joylink_head.h"
#include "../auth/joylink_auth_uECC.h"

typedef struct _guid_t{
	uint32  time_low;
	uint16  time_mid;
	uint16  time_hi_and_version;
	uint8   clock_seq_hi_and_reserved;
	uint8   clock_seq_low;
	uint8  node[6];
}guid_t;

uint8 getCrc(uint8 *ptr, uint8 len);
void dump8(uint8* p, int split, int len);
uint16 joylink_auth_crc16(const uint8 *buffer, uint32 size);
int joylink_util_make_guid(uint8 *guid_str, uint8 max_len);
int joylink_AesCBC256Decrypt(uint8 *pEncIn, uint32 encLength,uint8 *key_32bytes, uint8 *pPlainOut, uint32 maxPlainLength);
int joylink_AesCBC256Encrypt(uint8 *pEncIn, int32 encLength,uint8 *key_32bytes, uint8 *pPlainOut, uint32 maxPlainLength);
uECC_RNG_Function	uECC_generate_rng(uint8 *buf,unsigned size);

#endif

