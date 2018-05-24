/* --------------------------------------------------
 * @brief: 
 *
 * @version: 1.0
 *
 *
 * @author: 
 * --------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>

#include <arpa/inet.h>
#include "joylink_agent.h"
#include "joylink_agent_devs.h"
#include "joylink_log.h"
#include "joylink_agent_json.h"

typedef struct {
    int32_t fd;
    int32_t st;
    int32_t cloud_thread_run;
    char eabled;
    pthread_t ntid;
}jl_agent_cloud_mg_t;

static jl_agent_cloud_mg_t _g_c_mg, * _g_p_cmg = &_g_c_mg;

extern int 
joylink_dencypt_server_req(
        JLPacketParam_t *pParam, 
        const uint8_t *pIn, 
        int length, 
        uint8_t* pOut, int maxlen, char*  feedid);

#ifdef _AGENT_GW_ 
pthread_mutex_t    agent_devs_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

/**
 * brief: 
 */
void joylink_agent_devs_lock() 		
{
#ifdef _AGENT_GW_ 
    pthread_mutex_lock(&agent_devs_lock);
#endif
}

/**
 * brief: 
 */
void joylink_agent_devs_unlock() 		
{
#ifdef _AGENT_GW_ 
    pthread_mutex_unlock(&agent_devs_lock);
#endif
}

/**
 * brief: 
 *
 * @Returns: 
 */
static jl_agent_cloud_mg_t*
joylink_agent_cloud_mg(void)
{
    return &_g_c_mg;
}

/**
 * [joylink_agent_cloud_init]
 */
E_JLRetCode_t
joylink_agent_cloud_init(void)
{
    jl_agent_cloud_mg_t init_c_mg = { -1, 0, 1, 1};
    memcpy(_g_p_cmg, &init_c_mg, sizeof(jl_agent_cloud_mg_t));

    char err[512] = {0};
    int socket = joylink_adapter_net_tcp_server(err, JL_AGENT_GW_PORT, 
            NULL, JL_AGENT_GW_CLIENT_MAX);

    if(socket < 0){
        log_error("agent cloud tcp socket create error.");
        return E_RET_ERROR;
    }

    _g_p_cmg->fd = socket;

    return E_RET_OK;
}

/**
 * [joylink_agent_cloud_finit]
 */
void 
joylink_agent_cloud_finit(void)
{
    log_debug("=====finit cloud data: init st; close fd, set it to -1=====");
    _g_c_mg.st = 0;
    if (_g_c_mg.fd > 0) {
        joylink_adapter_net_close(_g_c_mg.fd);
    }
    _g_c_mg.fd = -1;
    log_debug("=====finit cloud data end=====");
    
    return;
}

E_JLRetCode_t
joylink_agent_make_session_key(char *skey)
{
    char sk[32] = "01234567890123456789012345678901";
    memcpy(skey, sk, 32);
    return E_RET_OK;
}

int
joylink_agent_dev_rsp()
{
    int ret = -1;
    int time_v;
    int len;
    char data[JL_MAX_PACKET_LEN] = {0};

    ret = joylink_dev_get_snap_shot(data + 4, JL_MAX_PACKET_LEN - 4);

    if(ret > 0){
        time_v = time(NULL);
        memcpy(data, &time_v, 4);

        len = joylink_encypt_server_rsp(
                _g_pdev->send_buff,
                JL_MAX_PACKET_LEN, PT_UPLOAD,
                _g_pdev->jlp.sessionKey, 
                (uint8_t*)&data,
                ret + 4);

        if(len > 0 && len < JL_MAX_PACKET_LEN){
            ret = send(_g_pdev->server_socket, _g_pdev->send_buff, len, 0);
            if(ret < 0){
                log_error("send error ret:%d", ret);
            }
            log_debug("send to server len:%d:ret:%d\n", len, ret);
        }else{
            log_error("send data too big or packe error:ret:%d", ret);
        }
    }

    return ret;
}

int 
joylink_agent_cloud_rsp_packet(
        uint8_t* pBuf, 
        int buflen, 
        E_PacketType cmd, 
        uint8_t* pOpt, 
        int optLen, 
        uint8_t* key, 
        const uint8_t* payload, 
        int payloadLen)
{
	JLPacketHead_t head = {
		.magic = 0x123455CC,
		.optlen = 0,
		.payloadlen = 0,
		.version = 1,
		.type = (char)cmd,
		.total = 0,
		.index = 0,
		.enctype = (char)1,
		.reserved = 0,
		.crc = 0		// Todo:У??CRC
	};

	int len = 0;
	uint8_t* pOut = pBuf + sizeof(JLPacketHead_t) + optLen;
    
    /*1 packet opt*/
    if((NULL != pOpt) && (optLen > 0)){
        memcpy(pOut, pOpt, optLen);
        head.optlen = optLen;
    }
    /*2 enctype type*/
    switch (cmd) {
        case PT_AUTH:
            head.enctype = ET_ACCESSKEYAES;
            break;
        default:
            head.enctype = ET_SESSIONKEYAES;
            break;
    }

    /*3 encrypt payload*/
    /*4 packet payload*/
    len = device_aes_encrypt(
            key, 16,
            key + 16,
            (uint8_t*)payload,
            payloadLen,
            pOut, buflen);

    head.payloadlen = len;
    pOut += len;

    /*5 crc*/
	len = pOut - pBuf;
	head.crc = CRC16(pBuf + sizeof(JLPacketHead_t), 
            len - sizeof(JLPacketHead_t));

    /*6 packet head*/
	memcpy(pBuf, &head, sizeof(head));
    
    /*7 return all packet len*/
	return len;
}

/*
 * agent dev >> agent GW
 * tcp
 * 
 */
E_JLRetCode_t
joylink_agent_dev_cloud_auth_hand(int32_t fd, char* feedid, uint32_t d_rand, 
        uint8_t *payload, int32_t s_len, char* auth_org)
{
    /**
     *1 check random  
     *2 create session key
     *3 packet rsp
     *4 send to rsp
     *5 update session key and fd to agent dev
     */
    int ret = E_RET_ERROR;
    int len = 0;
    char auth_rsp[4 + 4 + 32] = {0};
    char buff[JL_MAX_PACKET_LEN] = {0};

    JLAuth_t *pAuth = (JLAuth_t *)payload;
    JLAuthRsp_t *pRsp = (JLAuthRsp_t*)auth_rsp;

    if(d_rand != pAuth->random_unm){
        log_error("agent dev auth random is error");
        goto ERROR; 
    }

    pRsp->timestamp = time(NULL); 
    pRsp->random_unm = 3;
    joylink_agent_make_session_key((char *)pRsp->session_key);

    AgentDev_t *p = joylink_agent_dev_get(feedid);
    if(p == NULL){
        log_error("agent dev get is error");
        goto ERROR;
    }
    len = joylink_agent_cloud_rsp_packet(
                (uint8_t*)buff, 
                sizeof(buff), 
                PT_AUTH, 
                (uint8_t*) &pRsp->random_unm, 
                4, 
                (uint8_t*)p->accesskey, 
                (const uint8_t*)auth_rsp, 
                sizeof(auth_rsp));
    if(len < 0){
        log_error("packet error");
        goto ERROR;
    }

    ret = send(fd, buff, len, 0);
    if(ret < 0){
        log_error("send error ret:%d", ret);
        goto ERROR;
    }

    memcpy(p->sessionkey, pRsp->session_key, 32);
    memcpy(p->auth, auth_org, 16);
    p->random = pAuth->random_unm;

    p->fd = fd;
    ST_AGENT_AUTH_SET(p->agent_st);
    log_debug("send to server len:%d:ret:%d:%d\n", 
            len, ret, p->agent_st);
ERROR:
    return ret;
}

/*
 * agent dev >> agent GW
 * tcp
 * 
 */
E_JLRetCode_t
joylink_agent_dev_cloud_HB_hand(int32_t fd, uint8_t *payload, 
        int32_t s_len)
{
    /**
     *1 get HB
     *2 packet rsp
     *3 send to rsp
     *4 update HB agent dev
     */
    int ret = E_RET_ERROR;
    int len = 0;
    char buff[JL_MAX_PACKET_LEN] = {0};

    JLHearBeat_t *hb = (JLHearBeat_t*)payload;
    JLHearBeatRst_t hbrsp;

    hbrsp.timestamp = time(NULL);
    hbrsp.code = 0;

    AgentDev_t * p = joylink_agent_get_dev_by_fd(fd);
    if(p == NULL){
        log_error("agent dev get is error");
        goto ERROR;
    }
    len = joylink_agent_cloud_rsp_packet(
                (uint8_t*)buff, 
                sizeof(buff), 
                PT_BEAT, 
                NULL, 
                0, 
                (uint8_t*)p->sessionkey, 
                (const uint8_t*)&hbrsp, 
                sizeof(hbrsp));
    if(len < 0){
        log_error("packet error");
        goto ERROR;
    }

    ret = send(fd, buff, len, 0);
    if(ret < 0){
        log_error("send error ret:%d", ret);
        goto ERROR;
    }
    memcpy(&p->hb, hb, sizeof(JLHearBeat_t));
    log_debug("rssi:%d verion:%d\n", hb->rssi, hb->verion);
    ST_AGENT_HB_SET(p->agent_st);
    log_debug("send to server len:%d:ret:%d\n", len, ret);
ERROR:
    return ret;
}

/**
 * brief: 
 *
 * @Param: fd
 * @Param: payload
 * @Param: s_len
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_agent_dev_cloud_upload_hand(int32_t fd, uint8_t *payload, 
        int32_t s_len)
{
    /**
     *1 get UPLOAD 
     *2 packet rsp
     *3 send to rsp
     *4 update UPLOAD to agent dev
     */
    int ret = E_RET_ERROR;
    int len = 0;
    char buff[JL_MAX_PACKET_LEN] = {0};

    JLHearBeatRst_t hbrsp;

    hbrsp.timestamp = time(NULL);
    hbrsp.code = 0;

    AgentDev_t * p = joylink_agent_get_dev_by_fd(fd);
    if(p == NULL){
        log_error("agent dev get is error");
        goto ERROR;
    }
    len = joylink_agent_cloud_rsp_packet(
                (uint8_t*)buff, 
                sizeof(buff), 
                PT_UPLOAD, 
                NULL, 
                0, 
                (uint8_t*)p->sessionkey, 
                (const uint8_t*)&hbrsp, 
                sizeof(hbrsp));
    if(len < 0){
        log_error("packet error");
        goto ERROR;
    }

    ret = send(fd, buff, len, 0);
    if(ret < 0){
        log_error("send error ret:%d", ret);
        goto ERROR;
    }
    if(p->snap != NULL){
        free(p->snap);
    }
    if(NULL == (p->snap = (char*)malloc(s_len * 2 + 1))){
        log_error("malloc error");
    }else{
        memset(p->snap, 0, s_len * 2 + 1);
        joylink_util_byte2hexstr(payload, s_len, p->snap, s_len * 2);
    }
    ST_AGENT_SNAP_SET(p->agent_st);
    log_debug("snap:%s\n", payload);
    log_debug("send to server len:%d:ret:%d\n", len, ret);
ERROR:
    return ret;
}

/**
 * brief: 
 *
 * @Param: fd
 * @Param: payload
 * @Param: s_len
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_agent_dev_cloud_ota_up_hand(int32_t fd, 
        uint8_t *payload, 
        int32_t s_len)
{
    /**
     *1 packet rsp
     *2 send to rsp
     */
    int ret = E_RET_ERROR;
    int len = 0;
    char buff[JL_MAX_PACKET_LEN] = {0};
    char data[JL_MAX_PACKET_LEN] = {0};

    *((int32_t*)(data)) = (int)time(NULL);
    sprintf(data + 4, "{\"code\":%d, \"msg\":\"%s\"}", 0, "sucess");

    AgentDev_t * p = joylink_agent_get_dev_by_fd(fd);
    if(p == NULL){
        log_error("agent dev get is error");
        goto ERROR;
    }
    len = joylink_agent_cloud_rsp_packet(
                (uint8_t*)buff, 
                sizeof(buff), 
                PT_UPLOAD, 
                NULL, 
                0, 
                (uint8_t*)p->sessionkey, 
                (const uint8_t*)data, 
                strlen(data + 4) + 4);
    if(len < 0){
        log_error("packet error");
        goto ERROR;
    }

    ret = send(fd, buff, len, 0);
    if(ret < 0){
        log_error("send error ret:%d", ret);
        goto ERROR;
    }
    log_debug("send to server len:%d:ret:%d\n", len, ret);

ERROR:
    return ret;
}

/************ TCP SERVER *************/
/**
 * [joylink_agent_cloud_fd] 
 *
 * @returns: 
 */
int32_t
joylink_agent_cloud_fd()
{
    return _g_p_cmg->fd; 
}

/**
 * brief: 
 *
 * @Param: fd
 * @Param: rec_buff
 * @Param: max
 *
 * @Returns: 
 */
int
joylink_agent_tcp_recv(char fd, char *rec_buff, int max)
{
    JLPacketHead_t head;
    bzero(&head, sizeof(head));
    E_PT_REV_ST_t st = E_PT_REV_ST_MAGIC;
    int ret;
    
    do{
        switch(st){
            case E_PT_REV_ST_MAGIC:
                ret = recv(fd, &head.magic, sizeof(head.magic), 0);
                if(head.magic == 0x123455CC){
                    st = E_PT_REV_ST_HEAD;
                }
                break;
            case E_PT_REV_ST_HEAD:
                ret = recv(fd, &head.optlen, sizeof(head) - sizeof(head.magic), 0);
                if(ret > 0){
                    st = E_PT_REV_ST_DATA;
                }
                break;
            case E_PT_REV_ST_DATA:
                if(head.optlen + head.payloadlen < max - sizeof(head)){
                    ret = recv(fd, rec_buff + sizeof(head), head.optlen + head.payloadlen , 0);
                    if(ret > 0){
                        memcpy(rec_buff, &head, sizeof(head));
                        ret = ret + sizeof(head);
                        st = E_PT_REV_ST_END;
                    }

                }else{
                    ret = -1;
                    log_fatal("recev buff is too small");
                }
                break;
            default:
                break;
        }
    }while(ret>0 && st != E_PT_REV_ST_END);

    if (ret == -1 || ret == 0){
        return -1;
    }

    return ret;
}

/**
 * brief: 
 *
 * @Param: fd
 */
void
joylink_agent_dev_req_proc(int fd)
{
    uint8_t recBuffer[JL_MAX_PACKET_LEN * 8 ] = { 0 };
    uint8_t recPainText[JL_MAX_PACKET_LEN * 8 + 16] = { 0 };
    char feedid[JL_MAX_FEEDID_LEN] = {0};
    char auth_org[16] = {0};
    uint32_t d_rand = 0;
    int ret;
    JLPacketParam_t param;

    memset(&param, 0, sizeof(param));

    ret = joylink_agent_tcp_recv(fd, (char*)recBuffer, sizeof(recBuffer));
    if (ret == -1 || ret == 0){
        close(fd);
        joylink_agent_dev_clear_fd_by_fd(fd);
        log_info("client close!:%d\r\n", ret);
        return;
    }
    
	JLPacketHead_t* pPack = (JLPacketHead_t*)recBuffer;
    log_debug("pack type:%d", pPack->type);

    if(PT_AUTH == pPack->type){
        if(pPack->optlen < 4){
            log_error("pPack->optlen is error optlen:%d", pPack->optlen);
            return;
        }
        memcpy(feedid, recBuffer + sizeof(JLPacketHead_t), pPack->optlen - 4);
        memcpy(&d_rand, recBuffer + sizeof(JLPacketHead_t) + pPack->optlen - 4, 4);
        memcpy(auth_org, recBuffer + sizeof(JLPacketHead_t) + pPack->optlen, 16);

        log_debug("auth feedid:%s", feedid);
        log_debug("auth payload len:%d", pPack->payloadlen);
    }else{
        if(E_RET_OK != joylink_agent_get_feedid_by_fd(fd, feedid)){
            log_error("Find feedid by fd error:%d", fd);
            return;
        }
    }
   
    ret = joylink_dencypt_server_req(&param, recBuffer, ret, 
            recPainText, sizeof(recPainText), feedid);

    if (ret < 1){
        log_debug("dencypt erorr ret");
        return;
    }

    log_info("Server org ctrl type:%d:", param.type);
    switch (param.type){
        case PT_AUTH:
            log_debug("Dev AUTH is get");
            joylink_agent_dev_cloud_auth_hand(fd, feedid, d_rand, recPainText, ret, auth_org);
            /*joylink_agent_auth_req();*/
            break;
        case PT_BEAT:
            log_debug("Dev HB is get");
            joylink_agent_dev_cloud_HB_hand(fd, recPainText, ret);
            /*joylink_agent_HB_req();*/
            break;
        case PT_UPLOAD:
            joylink_agent_dev_cloud_upload_hand(fd, recPainText, ret);
            log_debug("Dev UP_UPLOAD is get");
            /*joylink_agent_snap_req();*/
            break;
        case PT_OTA_UPLOAD:
            log_debug("Dev PT_OTA_UPLOAD is get");
            /*send ack to dev*/
            joylink_agent_dev_cloud_ota_up_hand(fd, recPainText, ret);
            /*send ota st to cloud*/
            joylink_agent_ota_st_to_cloud_req(recPainText, ret);
            break;
        default:
            log_debug("Unknow param type.\r\n");
            break;
    }
}

/**
 * brief: 
 *
 * @Param: gw_fd
 */
static void
joylink_agent_cloud_server_proc(int32_t gw_fd)
{
    int32_t fd;
    char c_ip[128] = {0};
    char err[128] = {0};
    int32_t c_port;
    int32_t ret; 

    /*1 accept client */
    fd = joylink_net_tcp_accept(err, gw_fd, c_ip, sizeof(c_ip), &c_port);

    log_debug("--------------accept fd:%d, ip:%s", fd, c_ip);

    if(fd > 0){
        joylink_agent_dev_req_proc(fd);
    }
}

/**
 * brief: 
 *
 * @Returns: 
 */
void *
joylink_agent_cloud_loop(void* data)
{
    struct timeval select_time_out;
    int32_t fds[AGENT_DEV_MAX + 1];
    int32_t max_fd = 0;
    int32_t cloud_fd = 0;
    int32_t fd_cnt = 0;
    int32_t i;
    int32_t ret;

    joylink_agent_cloud_init();
    _g_p_cmg->cloud_thread_run = 1;

    jl_agent_cloud_mg_t *pcmgv = (jl_agent_cloud_mg_t*)data;
    while(_g_p_cmg->cloud_thread_run){
        fd_set  read_fds;
        FD_ZERO(&read_fds);
        
        cloud_fd = joylink_agent_cloud_fd();
        fds[0] = cloud_fd;

        fd_cnt = joylink_agent_devs_get_fds(&fds[1]);
        if(fd_cnt > 0){
            fd_cnt = fd_cnt + 1; 
        }else{
            fd_cnt = 1;
        }


        for(i = 0; i < fd_cnt; i++){
            if (fds[i] > 0) {
                FD_SET(fds[i], &read_fds);    
                max_fd = max_fd > fds[i]? max_fd : fds[i];
            }
        }

        /*select for msg.*/
        select_time_out.tv_usec = 500 * 1000L;
        select_time_out.tv_sec = (long)0;

        ret = joylink_adapter_select(max_fd + 1, 
                &read_fds, NULL, NULL, &select_time_out);
        log_info("select ret: %d", ret);

        if (ret < 0){
            log_error("select err: %d!\r\n", ret);
        }else if (ret > 0){
            if (FD_ISSET(cloud_fd, &read_fds)) {
                joylink_agent_cloud_server_proc(cloud_fd);
            }
            for(i = 1; i < fd_cnt; i++){
                if (FD_ISSET(fds[i], &read_fds)) {

                    joylink_agent_devs_lock();
                    joylink_agent_dev_req_proc(fds[i]);
                    joylink_agent_devs_unlock();

                }
            }
        }

    }

    return NULL;
}

/**
 * brief: 
 */
void
joylink_agent_gw_thread_start()
{
    if(0 != pthread_create(&_g_p_cmg->ntid, NULL, joylink_agent_cloud_loop, (void*)_g_p_cmg)){
        log_error("cloud thread create err!\n");
    }
}

/**
 * brief: 
 */
void
joylink_agent_gw_thread_stop()
{
    _g_p_cmg->cloud_thread_run = 0;
    pthread_cancel(_g_p_cmg->ntid);
}

/**
 * brief: 
 *
 * @Param: src
 * @Param: sin_recv
 * @Param: addrlen
 *
 * @Returns: 
 */
E_JLRetCode_t 
joylink_agent_proc_dev_add(uint8_t *src, struct sockaddr_in *sin_recv, socklen_t addrlen)
{
    int len = 0;
    int num = 0;
    int i;
    char data[JL_MAX_PACKET_LEN] = {0};

    JLAddAgentDev_t *devs = NULL;
    if(NULL == (devs = joylink_agent_parse_dev_add(src, &num))){
        log_error("parse error");
        return E_RET_ERROR;
    }

    for(i = 0; i < num; i++){
        log_debug("agent dev:%s", devs[i].feedid);
        joylink_agent_dev_add(devs[i].feedid, devs[i].ackey);
    }

    if(devs != NULL){
        free(devs);
    }

    *((int*)data) = time(NULL);
    sprintf(data + 4, "{\"code\":%d, \"msg\":\"%s\"}", 0, "sucesss");
    log_info("rsp data:%s:len:%d", data + 4, len);

    len = joylink_encrypt_lan_basic(
            _g_pdev->send_buff, 
            JL_MAX_PACKET_LEN,
            ET_ACCESSKEYAES,
            PT_SUB_ADD,
            (uint8_t *)_g_pdev->jlp.localkey, 
            (const uint8_t*)data, 
            strlen(data + 4) + 4);

    if(len <= 0 || len > JL_MAX_PACKET_LEN){
        log_error("packet error ret:%d", len);
    }else{
        len = sendto(_g_pdev->lan_socket, 
                _g_pdev->send_buff, len, 0,
                (SOCKADDR*)sin_recv, addrlen);

        if(len < 0){
            log_error("send error");
        }
        log_info("send ret:%d",len);
    }

    return E_RET_OK;
}

/**
 * brief: 
 *
 * @Param: src
 * @Param: sin_recv
 * @Param: addrlen
 *
 * @Returns: 
 */
E_JLRetCode_t 
joylink_agent_proc_dev_del(uint8_t *src, struct sockaddr_in *sin_recv, socklen_t addrlen)
{
    int len = 0;
    int num = 0;
    int i;
    char data[JL_MAX_PACKET_LEN] = {0};

    JLAddAgentDev_t *devs = NULL;
    if(NULL == (devs = joylink_agent_parse_dev_del(src, &num))){
        log_error("parse error");
        return E_RET_ERROR;
    }

    for(i = 0; i < num; i++){
        joylink_agent_dev_del(devs[i].feedid);
    }

    if(devs != NULL){
        free(devs);
    }

    *((int*)data) = time(NULL);
    sprintf(data + 4, "{\"code\":%d, \"msg\":\"%s\"}", 0, "sucesss");
    log_info("rsp data:%s:len:%d", data + 4, len);

    len = joylink_encrypt_lan_basic(
            _g_pdev->send_buff, 
            JL_MAX_PACKET_LEN,
            ET_ACCESSKEYAES,
            PT_SUB_ADD,
            (uint8_t *)_g_pdev->jlp.localkey, 
            (const uint8_t*)data, 
            strlen(data + 4) + 4);

    if(len <= 0 || len > JL_MAX_PACKET_LEN){
        log_error("packet error ret:%d", len);
    }else{
        len = sendto(_g_pdev->lan_socket, 
                _g_pdev->send_buff, len, 0,
                (SOCKADDR*)sin_recv, addrlen);

        if(len < 0){
            log_error("send error");
        }
        log_info("send ret:%d",len);
    }

    return E_RET_OK;
}

/**
 * brief: 
 *
 * @Param: src
 * @Param: sin_recv
 * @Param: addrlen
 *
 * @Returns: 
 */
E_JLRetCode_t 
joylink_agent_proc_get_dev_list(uint8_t *src, struct sockaddr_in *sin_recv, socklen_t addrlen)
{
    int len = 0;
    int num = 0;
    int i;
    char data[JL_MAX_PACKET_LEN] = {0};
    char *p = NULL;

    *((int*)data) = time(NULL);

    p = joylink_agent_json_packet_dev_list(0, "success");
    if(strlen(p) > sizeof(data)){
        sprintf(data + 4, "{\"code\":%d, \"msg\":\"%s\"}", 
                -1, "dev list too many to packet.");
    }else{
        memcpy(data + 4, p, strlen(p));
        free(p);
    }

    log_info("rsp data:%s:len:%d", data + 4, len);
    len = joylink_encrypt_lan_basic(
            _g_pdev->send_buff, 
            JL_MAX_PACKET_LEN,
            ET_ACCESSKEYAES,
            PT_AGENT_GET_DEV_LIST,
            (uint8_t *)_g_pdev->jlp.localkey, 
            (const uint8_t*)data, 
            strlen(data + 4) + 4);

    if(len <= 0 || len > JL_MAX_PACKET_LEN){
        log_error("packet error ret:%d", len);
    }else{
        len = sendto(_g_pdev->lan_socket, 
                _g_pdev->send_buff, len, 0,
                (SOCKADDR*)sin_recv, addrlen);

        if(len < 0){
            log_error("send error");
        }
        log_info("send ret:%d",len);
    }

    return E_RET_OK;
}

void
joylink_agent_req_cloud_proc()
{
    joylink_agent_auth_req();
    joylink_agent_HB_req();
    joylink_agent_snap_req();
}
