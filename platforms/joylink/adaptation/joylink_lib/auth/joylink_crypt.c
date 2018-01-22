#include <stdio.h>
#include <stdlib.h>
#include "joylink_crypt.h"
#include "joylink_utils.h"
#ifdef ESP_8266
#include "joylink_auth_uECC.h"
#include "joylink_aes.h"
#else
#include "auth/uECC.h"
#include "auth/aes.h"
#endif
#include "joylink.h"

JLEccContex_t __g_ekey = {0};

int 
joylink_ecc_contex_init(void)
{
	if (!__g_ekey.isInited){
		__g_ekey.isInited = 1;
		if (!uECC_make_key_joylink(__g_ekey.devPubKey, __g_ekey.priKey)) {
			os_printf("uECC_make_key() failed\n");
			return 1;
		}
		uECC_compress_joylink(__g_ekey.devPubKey, __g_ekey.devPubKeyC);
	}

	memcpy(_g_pdev->jlp.pubkeyC, __g_ekey.devPubKeyC, uECC_BYTES + 1);
	joylink_util_byte2hexstr(__g_ekey.devPubKeyC, uECC_BYTES + 1, 
            (uint8_t*)_g_pdev->jlp.pubkeyS, uECC_BYTES * 2 + 3);
	os_printf("DevicePubKey:\t%s\n", _g_pdev->jlp.pubkeyS);
    return 0;
}
