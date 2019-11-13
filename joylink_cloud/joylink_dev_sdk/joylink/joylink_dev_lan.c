#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if defined(__MTK_7687__)
#include <stdint.h>
#include "lwip/sockets.h"
#else
#include <stdarg.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#ifndef ESP_PLATFORM
#include <net/if.h>
#endif
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <ifaddrs.h>
#include <netdb.h>
#endif

#include "joylink.h"
#include "joylink_utils.h"
#include "joylink_packets.h"
#include "joylink_crypt.h"
#include "joylink_json.h"
#include "joylink_sub_dev.h"
#include "joylink_join_packet.h"
#include "auth/joylink_auth_crc.h"
#include "joylink_dev.h"
#include "joylink_extern.h"
#include "auth/joylink3_auth_uECC.h"
#ifdef _IS_DEV_REQUEST_ACTIVE_SUPPORTED_
#include "joylink_cloud_log.h"
#endif
#include "joylink_auth_md5.h"

#define USR_TIMESTAMP_MAX      (4)

typedef struct _u_t{
    int id;
    uint32_t timestamp;
}usr_ts_t;

usr_ts_t _g_UT[USR_TIMESTAMP_MAX];

extern int joylink_server_upload_req();

extern int joylink_server_st_close();

/**
 * brief: 
 *
 * @Param: str
 *
 * @Returns: 
 */
static int32_t  
joylink_util_RSHash(const char * str)
{
    if(NULL == str){
        return E_RET_ERROR;
    }
    int32_t  b  =   378551 ;
    int32_t  a  =   63689 ;
    int32_t  hash  =   0 ;

    while(* str){
        hash = hash * a + (*str++);
        a *= b;
    }

    return (hash  & 0x7FFFFFFF );
}

/**
 * brief: 
 *
 * @Returns: 
 */
int
joylink_cloud_timestamp_sync_check_reset_timecache()
{
    static uint32_t refresh_time = 0;
#ifdef ESP_PLATFORM
    if(_g_pdev->cloud_timestamp - refresh_time > 300){
        if((_g_pdev->cloud_timestamp & 0xFFFFF)  < 300){
            refresh_time = _g_pdev->cloud_timestamp;
            memset(_g_UT, 0, sizeof(_g_UT));
        }
    }
    return 0;
#else
    if(_g_pdev->cloud_timestamp - refresh_time > 300){
        if(_g_pdev->cloud_timestamp & 0xFFFFF  < 300){
            refresh_time = _g_pdev->cloud_timestamp;
            memset(_g_UT, 0, sizeof(_g_UT));
        }
    }
#endif
}

/**
 * brief: 
 *
 * @Param: id
 * @Param: timestamp
 *
 * @Returns: 
 */
int
joylink_is_usr_timestamp_ok(char *usr, uint32_t org_timestamp)
{
    char ip[64] = {0};
    int port;
    joylink_util_cut_ip_port(usr, ip, &port);

    int id = joylink_util_RSHash(ip);
    int i;

    /*1 */
    uint32_t timestamp = 0;
    timestamp = org_timestamp & 0xFFFFFFFF;

    log_info("timestamp info:->%s:%u\n", usr, timestamp);


    for(i=0; i < USR_TIMESTAMP_MAX; i++){
        /*update the timestamp*/
        if((_g_UT[i].id & 0x7FFFFFFF) == (id & 0x7FFFFFFF)){
            if(_g_UT[i].timestamp < timestamp){
                _g_UT[i].timestamp = timestamp;
                return 1;
            }else{
                log_error("timstamp error:->%s\n", usr);
                log_error("usr timestamp:%u, cache timestamp:%u\n", timestamp, _g_UT[i].timestamp);
                return 0;
            }
        }
    }
    if(i == USR_TIMESTAMP_MAX){
        /*no find usr add a usr to empty sapce*/
        for(i=0; i < USR_TIMESTAMP_MAX; i++){
            if((_g_UT[i].id & 0x80000000) == 0){
                _g_UT[i].timestamp = timestamp;
                _g_UT[i].id = id | 0x80000000; 
                return 1;
            }
        }
    }
    if(i == USR_TIMESTAMP_MAX){
        /*no find no empty , add a usr to timeout space*/
        for(i=0; i < USR_TIMESTAMP_MAX; i++){
            if(_g_UT[i].timestamp < (_g_pdev->cloud_timestamp - 60*60)){
                _g_UT[i].timestamp = timestamp;
                _g_UT[i].id = id | 0x80000000; 

                return 1;
            }
        }
    }
	if(i == USR_TIMESTAMP_MAX){
		return 0;
	}
    log_error("JSon Control timstamp error: no space->%s\n", usr);
    log_error("usr timestamp:%u, cache timestamp:%u\n", timestamp, _g_UT[i].timestamp);
    /*no space to add*/
    return 0;    
}

/**
 * brief: 1 get packet head and opt
 *        2 cut big packet as JL_MAX_CUT_PACKET_LEN 
 *        3 join phead, opt and paload
 *        4 crc
 *        5 send
 */
void
joylink_send_big_pkg(struct sockaddr_in *sin_recv, socklen_t addrlen)
{
    int ret = -1;
    int len;
    int i;
    int offset = 0;

	JLPacketHead_t *org_pt = (JLPacketHead_t*)_g_pdev->send_p;
    short res = rand()&0xFF;

    int total = (int)(org_pt->payloadlen/JL_MAX_CUT_PACKET_LEN) 
        + (org_pt->payloadlen%JL_MAX_CUT_PACKET_LEN? 1:0);

    for(i = 1; i <= total; i ++){
        if(i == total){
            len = org_pt->payloadlen%JL_MAX_CUT_PACKET_LEN;
            log_info("payloadlen::%d :len:%d",org_pt->payloadlen, len);
        }else{
            len = JL_MAX_CUT_PACKET_LEN;
        }
        
        offset = 0;
        bzero(_g_pdev->send_buff, sizeof(_g_pdev->send_buff));

        memcpy(_g_pdev->send_buff, _g_pdev->send_p, sizeof(JLPacketHead_t) + org_pt->optlen);
        offset  = sizeof(JLPacketHead_t) + org_pt->optlen;

        memcpy(_g_pdev->send_buff + offset, 
                _g_pdev->send_p + offset + JL_MAX_CUT_PACKET_LEN * (i -1), len);

	    JLPacketHead_t *pt; 
        pt = (JLPacketHead_t*)_g_pdev->send_buff; 
        pt->total = total;
        pt->index = i;
        pt->payloadlen = len;
        pt->crc = CRC16(_g_pdev->send_buff + sizeof(JLPacketHead_t), pt->optlen + pt->payloadlen);
        pt->reserved = (unsigned char)res;

        ret = sendto(_g_pdev->lan_socket, 
                _g_pdev->send_buff,
                len + sizeof(JLPacketHead_t) + pt->optlen, 0,
                (SOCKADDR*)sin_recv, addrlen);

        if(ret < 0){
            log_error("send error");
        }
        log_info("send to:%s:ret:%d:total:%d", _g_pdev->jlp.ip, ret, total);
    }

    if(NULL != _g_pdev->send_p){
        free(_g_pdev->send_p);
        _g_pdev->send_p = NULL;
    }
}

/**
 * brief: 
 *
 * @Param: src
 * @Param: sin_recv
 * @Param: addrlen
 */

static void
joylink_proc_lan_scan(uint8_t *src, struct sockaddr_in *sin_recv, socklen_t addrlen)
{
    int ret = -1;
    int len = -1;
    DevScan_t scan;
    int ret_sign = 0;


    bzero(&scan, sizeof(scan));

    if(E_RET_OK != joylink_parse_scan(&scan, (const char *)src)){
    }else{
#ifdef JOYLINK_CLOUD_AUTH
	if(strlen(scan.app_rand) != 0){
		uint8_t prikey_buf[65] = {0};
		uint8_t sign_buf[65] = {0};
		uint8_t rand_buf[33] = {0};
		int i = 0;

		log_info("cloud rand: %s", scan.app_rand);
        	joylink_util_hexStr2bytes(_g_pdev->jlp.prikey, prikey_buf, 32);

		ret_sign = jl3_uECC_sign(prikey_buf, scan.app_rand, strlen(scan.app_rand), sign_buf, uECC_secp256r1());
		if(ret_sign == 1){
			log_info("gen devsigature to cloud random success");
		}else{
			log_error("gen dev sigature to cloud random error");
			return;
		}

		joylink_util_byte2hexstr(sign_buf, 64, scan.dev_sign, 64*2);
	}
#endif
        len = joylink_packet_lan_scan_rsp(&scan);
    }

    if(len > 0){ 
        if(len < JL_MAX_CUT_PACKET_LEN){
            ret = sendto(_g_pdev->lan_socket, 
                    _g_pdev->send_buff,
                    len, 0,
                    (SOCKADDR*)sin_recv, addrlen);
            if(ret < 0){
                perror("send error");
            }
            log_info("send to:%s:ret:%d", _g_pdev->jlp.ip, ret);
        }else{
            joylink_send_big_pkg(sin_recv, addrlen);
            log_info("SEND PKG IS TOO BIG:ip:%s", _g_pdev->jlp.ip);
        }
    }else{
        log_error("packet error ret:%d", ret);
    }
}

/**
 * brief: 
 *
 * @Param: src
 * @Param: sin_recv
 * @Param: addrlen
 */

extern int jl3_uECC_verify_256r1(const uint8_t *public_key,
                const uint8_t *message_hash,
                unsigned hash_size,
                const uint8_t *signature);

static void
joylink_proc_lan_write_key(uint8_t *src, struct sockaddr_in *sin_recv, socklen_t addrlen)
{
    int ret = -1;
    int len = 0;
    DevEnable_t de;
    bzero(&de, sizeof(de));

    uint8_t sig[64] = {0}; 
    uint8_t pubkey[33] = {0}; 

    joylink_parse_lan_write_key(&de, (const char*)src + 4);

    joylink_util_hexStr2bytes(_g_pdev->idt.cloud_pub_key, pubkey, sizeof(pubkey));
    joylink_util_hexStr2bytes(de.cloud_sig, sig, sizeof(sig));

#ifdef JOYLINK_DEVICE_AUTH
    if(1 == jl3_uECC_verify_256r1((uint8_t *)pubkey, 
                (uint8_t *)_g_pdev->idt.rand, 
                strlen(_g_pdev->idt.rand), 
                (uint8_t *)sig)){
#else
    if(1){
#endif
        if(_g_pdev->jlp.is_actived && (strcmp(_g_pdev->jlp.feedid, de.feedid) || strcmp(_g_pdev->jlp.accesskey, de.accesskey) \
            || strcmp(_g_pdev->jlp.localkey, de.localkey) || strcmp(_g_pdev->jlp.javs_server, de.javs_server) \
            || strcmp(_g_pdev->jlp.opengw_server, de.opengw_server) || strcmp(_g_pdev->jlp.router_server, de.router_server))){
            len = joylink_packet_lan_write_key_rsp(E_RET_ERROR_DEV_ACTIVED, "dev is alread actived");
            log_error("dev is alread actived, not response write key");
        }else{
            strcpy(_g_pdev->jlp.feedid, de.feedid);
            strcpy(_g_pdev->jlp.accesskey, de.accesskey);
            strcpy(_g_pdev->jlp.localkey, de.localkey);

            strcpy(_g_pdev->jlp.javs_server, de.javs_server);
            strcpy(_g_pdev->jlp.opengw_server, de.opengw_server);
            strcpy(_g_pdev->jlp.router_server, de.router_server);

            joylink_util_cut_ip_port(de.joylink_server, _g_pdev->jlp.joylink_server, &_g_pdev->jlp.server_port);

            _g_pdev->jlp.is_actived = 1;
            joylink_dev_set_attr_jlp(&_g_pdev->jlp);
            len = joylink_packet_lan_write_key_rsp(0, "write accesskey ok");

            joylink_server_st_close();

            //memset(_g_pdev->idt.rand, 0, sizeof(_g_pdev->idt.rand));
	    log_info("\nWrite accesskey ok!\n");
	}
    }else{
        len = joylink_packet_lan_write_key_rsp(-1, "verify cloud sig error");
        log_error("-->verify cloud sig error:%s\n cloud_public_key:%s", de.cloud_sig, _g_pdev->idt.cloud_pub_key);
    }
    
    if(len > 0 && len < JL_MAX_PACKET_LEN){ 
        ret = sendto(_g_pdev->lan_socket, _g_pdev->send_buff, len, 0, (SOCKADDR*)sin_recv, addrlen);
        if(ret < 0){
            log_error("send error ret:%d", ret);
        }
	ret = sendto(_g_pdev->lan_socket, _g_pdev->send_buff, len, 0, (SOCKADDR*)sin_recv, addrlen);
        if(ret < 0){
            log_error("send error ret:%d", ret);
        }
	ret = sendto(_g_pdev->lan_socket, _g_pdev->send_buff, len, 0, (SOCKADDR*)sin_recv, addrlen);
        if(ret < 0){
            log_error("send error ret:%d", ret);
        }
    }else{
        log_error("packet error ret:%d", ret);
    }
}
#ifdef _IS_DEV_REQUEST_ACTIVE_SUPPORTED_
/**
 * @name:joylink_active_write_key 
 *
 * @param: de
 * @param: randstr
 *
 * @returns:   
 */
int joylink_active_write_key(DevEnable_t *de,char *randstr)
{
    int ret = -1;
    int len = 0;
    uint8_t sig[64] = {0}; 
    uint8_t pubkey[33] = {0}; 
	log_info("-->feedid:%s:accesskey:%s\n", de->feedid, de->accesskey);
	log_info("-->localkey:%s\n", de->localkey);
	log_info("-->joylink_server:%s\n", de->joylink_server);
	log_info("-->cloud sig:%s\n", de->cloud_sig);

	memset(_g_pdev->idt.rand,0,sizeof(_g_pdev->idt.rand));
	strcpy(_g_pdev->idt.rand,randstr);
	
	joylink_util_hexStr2bytes(_g_pdev->idt.cloud_pub_key, pubkey, sizeof(pubkey));
	joylink_util_hexStr2bytes(de->cloud_sig, sig, sizeof(sig));
	if(1 == jl3_uECC_verify_256r1((uint8_t *)pubkey, (uint8_t *)_g_pdev->idt.rand, strlen(_g_pdev->idt.rand), (uint8_t *)sig)){

		uint8_t temp[32] = {0};
		uint8_t localkey[33] = {0};
		MD5_CTX md5buf;
	
		memset(&md5buf, 0, sizeof(MD5_CTX));
		JDMD5Init(&md5buf);
		JDMD5Update(&md5buf, de->accesskey, strlen(de->accesskey));
		JDMD5Final(&md5buf, temp);
		joylink_util_byte2hexstr(temp, 16, localkey, 32);

		strcpy(_g_pdev->jlp.feedid, de->feedid);
		strcpy(_g_pdev->jlp.accesskey, de->accesskey);
		strcpy(_g_pdev->jlp.localkey, localkey);
	
		joylink_util_cut_ip_port(de->joylink_server,
						_g_pdev->jlp.joylink_server,
						&_g_pdev->jlp.server_port);
	
		_g_pdev->jlp.is_actived = 1;
		ret = joylink_dev_set_attr_jlp(&_g_pdev->jlp);
		if(ret != E_RET_OK){
			log_error("write active data fail");
			joylink_cloud_log_post(TAGID_LOG_AP_WRITE_FEEDID_RES, RES_LOG_FAIL,"2018-12-21 17:20:20","[CloudLog]write active data to flash fail","0",de->feedid);
			goto RET;
		}
		memset(_g_pdev->idt.rand, 0, sizeof(_g_pdev->idt.rand));

		joylink_server_st_close();

		log_info("active write success");
		joylink_cloud_log_post(TAGID_LOG_AP_WRITE_FEEDID_RES, RES_LOG_SUCCES,"2018-12-21 17:20:20","[CloudLog]write active data success","0",de->feedid);
		ret = E_RET_OK;
	}else{
		log_error("-->verify cloud sig error:%s\n cloud_public_key:%s", de->cloud_sig, _g_pdev->idt.cloud_pub_key);
		joylink_cloud_log_post(TAGID_LOG_AP_WRITE_FEEDID_RES, RES_LOG_FAIL,"2018-12-21 17:20:20","[CloudLog]verify cloud signature fail","0",de->feedid);
		ret = E_RET_ERROR;
	}
RET:
	return ret;
}
#endif
/**
 * brief: 
 *
 * @Param: json_cmd
 * @Param: sin_recv
 * @Param: addrlen
 */
static void
joylink_proc_lan_json_ctrl(uint8_t *json_cmd, struct sockaddr_in *sin_recv, socklen_t addrlen)
{
    int ret = -1;
    int len = 0;
    char data[JL_MAX_PACKET_LEN] = {0};
    char feedid[JL_MAX_FEEDID_LEN] = {0};
    time_t tt = time(NULL);

    joylink_parse_json_ctrl(feedid, (char*)json_cmd);
    ret = joylink_dev_lan_json_ctrl((char *)json_cmd);
   
    memcpy(data, &tt, 4);
    ret = joylink_dev_get_json_snap_shot(data + 4, sizeof(data) - 4, ret, feedid);

    log_info("rsp data:%s:len:%d", data + 4 , ret);

    len = joylink_encrypt_lan_basic(_g_pdev->send_buff, JL_MAX_PACKET_LEN,
            ET_ACCESSKEYAES, PT_SCRIPTCONTROL,
            (uint8_t*)_g_pdev->jlp.localkey, (const uint8_t*)data, ret + 4);

    if(len > 0 && len < JL_MAX_PACKET_LEN){ 
        ret = sendto(_g_pdev->lan_socket, _g_pdev->send_buff, len, 0,
                (SOCKADDR*)sin_recv, addrlen);

        if(ret < 0){
            log_error(" 1 send error ret:%d", ret);
        }else{
            log_info("rsp to:%s:len:%d", _g_pdev->jlp.ip, ret);
        }
    }else{
        log_error("packet error ret:%d", ret);
    }

    joylink_server_upload_req();
} 

/**
 * brief: 
 *
 * @Param: src
 * @Param: src_len
 * @Param: sin_recv
 * @Param: addrlen
 */
static void
joylink_proc_lan_script_ctrl(uint8_t *src, uint8_t src_len, struct sockaddr_in *sin_recv, socklen_t addrlen)
{
    int ret = -1;
    int len = 0;
    char data[JL_MAX_PACKET_LEN] = {0};
    JLContrl_t ctr;
    bzero(&ctr, sizeof(ctr));

    if(-1 == joylink_dev_script_ctrl((const char *)src, src_len, &ctr, 0)){
        return;
    }
    ret = joylink_packet_script_ctrl_rsp(data, sizeof(data), &ctr);
    log_info("rsp data:%s:len:%d", data + 12, ret);
    len = joylink_encrypt_lan_basic(_g_pdev->send_buff, JL_MAX_PACKET_LEN,
            ET_ACCESSKEYAES, PT_SCRIPTCONTROL,
            (uint8_t*)_g_pdev->jlp.localkey, (const uint8_t*)data, ret);

    if(len > 0 && len < JL_MAX_PACKET_LEN){ 
        ret = sendto(_g_pdev->lan_socket, _g_pdev->send_buff, len, 0,
                (SOCKADDR*)sin_recv, addrlen);

        if(ret < 0){
            log_error(" 1 send error ret:%d", ret);
        }else{
            log_info("rsp to:%s:len:%d", _g_pdev->jlp.ip, ret);
        }
    }else{
        log_error("packet error ret:%d", ret);
    }
    joylink_server_upload_req();
}

/**
 * brief: 
 *
 * @Param: src
 * @Param: src_len
 * @Param: sin_recv
 * @Param: addrlen
 */
static void
joylink_proc_lan_ctrl_timestamp_error_rsp(uint8_t *src, uint8_t src_len, struct sockaddr_in *sin_recv, socklen_t addrlen)
{
    int ret = -1;
    int len = 0;
    char data[JL_MAX_PACKET_LEN] = {0};

    ret = joylink_packet_script_ctrl_error_rsp(data, sizeof(data));

    log_info("rsp data:%s:len:%d", data + 12, ret);
    len = joylink_encrypt_lan_basic(_g_pdev->send_buff, JL_MAX_PACKET_LEN,
            ET_ACCESSKEYAES, PT_SCRIPTCONTROL,
            (uint8_t*)_g_pdev->jlp.localkey, (const uint8_t*)data, ret);

    if(len > 0 && len < JL_MAX_PACKET_LEN){ 
        ret = sendto(_g_pdev->lan_socket, _g_pdev->send_buff, len, 0,
                (SOCKADDR*)sin_recv, addrlen);

        if(ret < 0){
            log_error(" 1 send error ret:%d", ret);
        }else{
            log_info("rsp to:%s:len:%d", _g_pdev->jlp.ip, ret);
        }
    }else{
        log_error("packet error ret:%d", ret);
    }
}
/**
 * brief: 
 *
 * @Param: data
 * @Param: len
 * @Param: sin_recv
 * @Param: addrlen
 *
 * @Returns: 
 */
E_JLRetCode_t 
joylink_proc_lan_rsp_send(uint8_t* data, int len,  struct sockaddr_in *sin_recv, socklen_t addrlen)
{
    int ret = E_RET_ERROR;
    int en_len = 0;
    en_len = joylink_encrypt_lan_basic(_g_pdev->send_buff, JL_MAX_PACKET_LEN,
            ET_ACCESSKEYAES, PT_SCRIPTCONTROL,
            (uint8_t*)_g_pdev->jlp.localkey, (const uint8_t*)data, len);

    if(en_len > 0 && en_len < JL_MAX_PACKET_LEN){ 
        ret = sendto(_g_pdev->lan_socket, _g_pdev->send_buff, en_len, 0,
                (SOCKADDR*)sin_recv, addrlen);

        if(ret < 0){
            log_error("send error ret:%d", ret);
        }else{
            log_info("rsp to:%s:len:%d", _g_pdev->jlp.ip, ret);
        }
    }else{
        log_error("packet error ret:%d", ret);
    }

    joylink_server_upload_req();

    return ret;
}

/**
 * brief: 
 */
void
joylink_proc_lan()
{
    int ret;
    uint8_t recBuffer[JL_MAX_PACKET_LEN] = { 0 };
    uint8_t recPainBuffer[JL_MAX_PACKET_LEN] = { 0 };
    uint8_t *recPainText = NULL;

    struct sockaddr_in sin_recv;
    JLPacketHead_t* pHead = NULL;
    JLPacketParam_t param;

    bzero(&sin_recv, sizeof(sin_recv));
    bzero(&param, sizeof(param));
    socklen_t addrlen = sizeof(SOCKADDR);

    ret = recvfrom(_g_pdev->lan_socket, recBuffer, 
            sizeof(recBuffer), 0, (SOCKADDR*)&sin_recv, &addrlen);

    int len = ret;

    if(ret == -1){
        goto RET;
    }

    joylink_util_get_ipstr(&sin_recv, _g_pdev->jlp.ip);

    ret = joylink_dencypt_lan_req(&param, recBuffer, ret, recPainBuffer, JL_MAX_PACKET_LEN);
    if (ret <= 0){
        goto RET;
    }

    pHead = (JLPacketHead_t*)recBuffer;

    if(pHead->total > 1){
        log_info("PACKET WAIT TO JOIN!!!!!!:IP:%s", _g_pdev->jlp.ip);
    }else{
        recPainText = recPainBuffer;
    }

    if(NULL == recPainText){
        goto RET;
    }

    log_info("------------- lan ctrl --------------");
    switch (param.type){
        case PT_SCAN:
            if(param.version == 1){
                log_info("PT_SCAN:%s (Scan->Type:%d, Version:%d)", 
                        _g_pdev->jlp.ip, param.type, param.version);
                joylink_proc_lan_scan(recPainText, &sin_recv, addrlen);
            }
            break;
        case PT_WRITE_ACCESSKEY:
            if(param.version == 1 &&
                joylink_is_usr_timestamp_ok(_g_pdev->jlp.ip,*((int*)(recPainText)))){
                log_info("PT_WRITE_ACCESSKEY");
                log_info("-->write key org:%s", recPainText + 4);
                joylink_proc_lan_write_key(recPainText, &sin_recv, addrlen);
            }
            break;
        case PT_JSONCONTROL:
            if(param.version == 1 &&
                    joylink_is_usr_timestamp_ok(_g_pdev->jlp.ip,*((int*)(recPainText)))){
                log_info("JSon Control->%s\n", recPainText + 4);
                joylink_proc_lan_json_ctrl(recPainText + 4, &sin_recv, addrlen);
            }
            break;
        case PT_SCRIPTCONTROL:
            log_debug("SCRIPT Control serial->%d", *((int*)(recPainText + 8)));

            int32_t biz_code = (int)(*((int *)(recPainText  + 4)));
            if(biz_code == JL_BZCODE_GET_SNAPSHOT || biz_code == JL_BZCODE_CTRL){
                if(param.version == 1 &&
                        joylink_is_usr_timestamp_ok(_g_pdev->jlp.ip,*((int*)(recPainText)))){
                    log_debug("SCRIPT Control->%s", recPainText + 12);
                    joylink_proc_lan_script_ctrl(recPainText, ret,  &sin_recv, addrlen);
                }else{
                    //joylink_proc_lan_ctrl_timestamp_error_rsp(recPainText, ret,  &sin_recv, addrlen);
                }
            }else{
                log_error("SCRIPT Control biz_code error:%d", biz_code);
            }

            break;
        case PT_SUB_AUTH:
            if(param.version == 1 &&
                    joylink_is_usr_timestamp_ok(_g_pdev->jlp.ip,*((int*)(recPainText)))){
                log_info("PT_SUB_AUTH");
                log_debug("Control->%s", recPainText + 4);
                joylink_proc_lan_sub_auth(recPainText + 4, &sin_recv, addrlen);
            }
            break;
        case PT_SUB_LAN_JSON:
            if(param.version == 1 &&
                    joylink_is_usr_timestamp_ok(_g_pdev->jlp.ip,*((int*)(recPainText)))){
                log_debug("Control->%s", recPainText + 4);
                joylink_proc_lan_json_ctrl(recPainText + 4, &sin_recv, addrlen);
            }
            break;
        case PT_SUB_LAN_SCRIPT:
            if(param.version == 1 &&
                    joylink_is_usr_timestamp_ok(_g_pdev->jlp.ip,*((int*)(recPainText)))){
                log_info("PT_SUB_LAN_SCRIPT");
                log_debug("localkey->%s", _g_pdev->jlp.localkey);
                //log_debug("Control->%s", recPainText + 12 + 32);
                joylink_proc_lan_sub_script_ctrl(recPainText, ret,
                            &sin_recv, addrlen);
            }
            break;
        case PT_SUB_ADD:
            if(param.version == 1 &&
                joylink_is_usr_timestamp_ok(_g_pdev->jlp.ip,*((int*)(recPainText)))){
                log_info("PT_SUB_ADD");
                log_debug("Sub add->%s", recPainText + 4);
		{
			int i = 0;
			printf("test: \n");
			for(i = 0; i < 32; i++)
			{
				printf("%02x ", recPainText[i]);
			}
			printf("\n");
		}
                joylink_proc_lan_sub_add(recPainText + 4, &sin_recv, addrlen);
            }
            break;
#ifdef _AGENT_GW_
        case PT_AGENT_ADD_SUBDEV:
            if(param.version == 1 &&
                joylink_is_usr_timestamp_ok(_g_pdev->jlp.ip,*((int*)(recPainText)))){
                log_info("PT_AGENT_ADD_SUBDEV");
                log_debug("PT_AGENT_ADD_SUBDEV->%s", recPainText + 4);
                joylink_agent_proc_dev_add(recPainText + 4, &sin_recv, addrlen);
            }
        break;
        case PT_AGENT_DEL_SUBDEV:
            if(param.version == 1 &&
                joylink_is_usr_timestamp_ok(_g_pdev->jlp.ip,*((int*)(recPainText)))){
                log_debug("PT_AGENT_DEL_SUBDEV->%s", recPainText + 4);
                joylink_agent_proc_dev_del(recPainText + 4, &sin_recv, addrlen);
            }
        break;
        case PT_AGENT_GET_DEV_LIST:
            if(param.version == 1 &&
                joylink_is_usr_timestamp_ok(_g_pdev->jlp.ip, *((int*)(recPainText)))){
                log_debug("PT_AGENT_GET_LIST->%s", recPainText + 4);
                joylink_agent_proc_get_dev_list(recPainText + 4, 
                        &sin_recv, addrlen);
            }
        break;
#endif

        default:
            break;
    }

RET:
/*
    if(NULL != pHead
            && pHead->total > 1
            && NULL != recPainText){
            free(recPainText);
    }
*/
    return;
}

