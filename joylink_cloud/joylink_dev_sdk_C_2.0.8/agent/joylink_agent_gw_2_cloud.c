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

#include "joylink_agent.h"
#include "joylink_agent_devs.h"
#include "joylink_agent_json.h"
#include "joylink_log.h"

extern void 
joylink_cloud_fd_lock();

extern void 
joylink_cloud_fd_unlock();

extern char *
joylink_package_ota_fff_upload();

extern char * 
joylink_agent_create_cloud_auth_json_req_f();

extern void 
joylink_agent_devs_lock();

extern void 
joylink_agent_devs_unlock();

/**
 * brief: 
 *
 * @Param: msg
 * @Param: msg_len
 * @Param: cmd
 *
 * @Returns: 
 */

extern E_JLBOOL_t
is_joylink_server_st_work();

E_JLRetCode_t
joylink_agent_package_and_send(char* msg, int32_t msg_len, int32_t cmd)
{
    if(NULL == msg || msg_len <= 0){
        log_error("Param error");
        return E_RET_ERROR;
    }
    int ret = E_RET_ERROR;
    char *data = NULL;

    if(!is_joylink_server_st_work()){
        log_error("cloud is not work");
        return E_RET_ERROR;
    }

    if(NULL == (data = (char*)malloc(msg_len + 32))){
        log_error("malloc is error");
        return E_RET_ERROR;
    }

    ret = joylink_encypt_server_rsp(
            (uint8_t*)data,
            msg_len + 32, 
            cmd,
            _g_pdev->jlp.sessionKey, 
            (uint8_t*)msg,
            msg_len);

    if(ret < 0){
        log_error("packe error:ret:%d", ret);
    }else{
        joylink_cloud_fd_lock();
        ret = send(_g_pdev->server_socket, data, ret, 0);
        log_info("VVVVVVVVVVV:send ret:%d", ret);
        joylink_cloud_fd_unlock();
        if(ret < 0){
            log_error("send error ret:%d", ret);
        }else{
            ret = E_RET_OK;
        }
    }
RET:
    if(NULL != data){
        free(data);
    }
    return ret;
}

/**
 * brief: 
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_agent_auth_req()
{
    /*
     *1 get agent devs data
     *2 packet data as msg
     *3 encypt and send 
     */
    char * p_json = joylink_agent_create_cloud_auth_json_req_f();
    if(NULL == p_json){
        return E_RET_ERROR;
    }
    char *p = (char*)malloc(strlen(p_json) + 4); 
    if(NULL != p){
        int32_t *pi = (int*)p;
        *pi = time(NULL);
        memcpy(p + 4, p_json, strlen(p_json));
        log_debug("auth json len:%d: text:%s:", (int)strlen(p_json),  p_json);
        joylink_agent_package_and_send(p, strlen(p_json) + 4, PT_AGENT_AUTH);
        free(p_json);
        free(p);
    }
    return E_RET_OK;
}

/**
 * brief: 
 *
 * @Param: src
 * @Param: len
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_agent_auth_rsps(char *src, int len)
{
    /*
     *1 check the code 
     */
    AgentAuthRsp_t *p = (AgentAuthRsp_t *)src;
    log_info("AGENT AUTH rsp: time:%d, code:%d", p->timestamp, p->code);

    if(p->code != E_RET_OK && len > 0){
        log_debug("error feedids:%s", p->error_devs);
    }
    return E_RET_OK;
}

/**
 * brief: 
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_agent_HB_req()
{
    /*
     *1 get agent devs data
     *2 packet data as msg
     *3 encypt and send 
     */

    char * p_json = NULL;
    p_json = joylink_agent_create_cloud_hb_json_req();
    if(NULL == p_json){
        return E_RET_ERROR;
    }
    char *p = (char*)malloc(strlen(p_json) + 4); 
    if(NULL != p){
        int32_t *pi = (int*)p;
        *pi = time(NULL);
        memcpy(p + 4, p_json, strlen(p_json));
        log_debug("auth json len:%d: text:%s:", (int)strlen(p_json),  p_json);
        joylink_agent_package_and_send(p, strlen(p_json) + 4, PT_AGENT_HB);
        free(p_json);
        free(p);
    }
    return E_RET_OK;
}

/**
 * brief: 
 *
 * @Param: src
 * @Param: len
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_agent_HB_rsps(char *src, int len)
{
    /*
     *1 check the code 
     */
    AgentHBRsp_t *p = NULL;
    p = (AgentHBRsp_t *)src;
    if(p->code != 0){
        log_error("HB rsps error");
    }
    log_info("AGENT HB rsp: time:%d, code:%d", p->timestamp, p->code);

    return E_RET_OK;
}

/**
 * brief: 
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_agent_snap_req()
{
    /*
     *1 get agent devs data
     *2 packet data as msg
     *3 encypt and send 
     */
    char * p_json = NULL; 
    p_json = joylink_agent_create_cloud_snap_json_req();
    if(NULL == p_json){
        return E_RET_ERROR;
    }
    char *p = (char*)malloc(strlen(p_json) + 4); 
    if(NULL != p){
        int32_t *pi = (int*)p;
        *pi = time(NULL);
        memcpy(p + 4, p_json, strlen(p_json));
        log_debug("auth json len:%d: text:%s:", (int)strlen(p_json),  p_json);
        joylink_agent_package_and_send(p, strlen(p_json) + 4, PT_AGENT_UPLOAD);
        free(p_json);
        free(p);
    }
    return E_RET_OK;
}

/**
 * brief: 
 *
 * @Param: src
 * @Param: len
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_agent_snap_rsps(char *src, int len)
{
    /*
     *1 check the code 
     */
    AgentSnapsRsp_t *p = NULL;
    p = (AgentSnapsRsp_t*)src;

    if(p->code != 0){
        log_error("AGENT SNAP rsps error");
    }

    log_info("AGENT SNAP rsp: time:%d, code:%d", p->timestamp, p->code);

    return E_RET_OK;
}

/**
 * brief: 
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_agent_ota_st_to_cloud_req(char* src, int ret)
{
    joylink_agent_package_and_send(src, ret, PT_UPLOAD);

    return E_RET_OK;
}

/**
 * brief: 
 *
 * @Param: recPainText
 * @Param: src_len
 * @Param: o_result
 * @Param: o_len
 */
E_JLRetCode_t
joylink_agent_dev_ctrl(uint8_t* recPainText, int32_t src_len, uint8_t* o_result, int32_t o_max, int32_t *o_len)
{
    /**
     *1 get feedid
     *2 encrypt dev msg
     *3 send dev msg
     *4 wait msg ack
     *5 decrypt dev ack  
     */
    int ret = E_RET_ERROR;
    int len = 0;
    char data[JL_MAX_PACKET_LEN * 10] = {0};
    char feedid[33] = {0};
    JLPacketParam_t param;

    memcpy(feedid, recPainText, 32);
    memset(&param, 0, sizeof(param));

    /*joylink_agent_devs_lock();*/

     /*1 get feedid*/
    AgentDev_t * p = joylink_agent_dev_get(feedid);
    if(p == NULL){
        log_error("agent dev get is error");
        goto ERROR;
    }

    log_info("AGENT_CTRL:%s", recPainText + 32 + 4);
     /*2 encrypt as dev msg*/
    len = joylink_agent_cloud_rsp_packet(
                (uint8_t*)data, 
                sizeof(data), 
                PT_SERVERCONTROL, 
                NULL, 
                0, 
                (uint8_t*)p->sessionkey, 
                (const uint8_t*)(recPainText + 32), 
                src_len - 32);
    if(len < 0){
        log_error("packet error");
        goto ERROR;
    }

     /*3 send dev msg*/
    ret = send(p->fd, data, len, 0);
    if(ret < 0){
        log_error("send error ret:%d", ret);
        goto ERROR;
    }

     /*4 wait msg ack*/
    memset(data, 0, sizeof(data));
    ret = joylink_agent_tcp_recv(p->fd, (char*)data, sizeof(data));
    if (ret == -1 || ret == 0){
        close(p->fd);
        log_info("Server close, Reconnect!\r\n");
        goto ERROR;
    }

     /*5 decrypt dev ack*/
    ret = joylink_dencypt_server_req(&param, data, ret, 
            o_result, o_max, feedid);

    if (ret < 1){
        log_debug("dencypt erorr ret");
        goto ERROR;
    }

    *o_len = ret;
    /*joylink_agent_devs_unlock();*/
    return  E_RET_OK;
ERROR:
    /*joylink_agent_devs_unlock();*/
    return E_RET_ERROR;
}

/**
 * brief: 
 *
 * @Param: recPainText
 * @Param: src_len
 */
void
joylink_agent_server_ctrl(uint8_t* recPainText, int32_t src_len)
{
    /**
     *1 send msg to dev and get result
     *2 packge result as agent result
     *3 send result to cloud
     */

    int ret = -1;
    int len;
    char data[JL_MAX_PACKET_LEN * 10] = {0};
    char *p = NULL;

    log_debug("Control from server:%s:len:%d\n",
            recPainText + 12 + 32, (int)strlen((char*)(recPainText + 12 + 32)));
   
    printf("CCCC:start:%ld", time(NULL));
    fflush(stdout);
    joylink_agent_devs_lock();
    /*
     *1 send msg to dev and get result
     */
    if(E_RET_OK != joylink_agent_dev_ctrl((char *)recPainText, src_len, data + 32, sizeof(data), &len)){
        log_error("agent server ctrl error");
    }else{
        /*
         *2 packge result as agent result
         */
        /*copy feedid*/
        memcpy(data, recPainText, 32); 
        len += 32;
    }
    joylink_agent_devs_unlock();
    printf("CCCC:end:%ld", time(NULL));

    /*len + encypt tail len 32*/
    if(len + 32 > sizeof(_g_pdev->send_buff)){
        p = (char*)malloc(len + 32);
        memset(p, 0, len + 32);
    }else{
        p = _g_pdev->send_buff;
    }

    len = joylink_encypt_server_rsp(p, len + 32, 
            PT_AGENT_DEV_CTRL, _g_pdev->jlp.sessionKey, 
            (uint8_t*)data, len);

    if(len > 0){
        if(0 > (ret = send(_g_pdev->server_socket, p, len, 0))){
            log_error("send error ret:%d", ret);
        }
        log_info("send to server len:%d:ret:%d\n", len, ret);
    }else{
        log_error("packet error ret:%d", ret);
    }
}

/**
 * brief: 
 *
 * @Param: recPainText
 * @Param: src_len
 */
void
joylink_agent_cloud_ota_to_dev(char * feedid, 
        uint8_t* recPainText, int32_t src_len,
        uint8_t* o_result, int32_t o_max, int32_t *o_len)
{
    int ret = E_RET_ERROR;
    int len = 0;
    char data[JL_MAX_PACKET_LEN] = {0};
    JLPacketParam_t param;

    memset(&param, 0, sizeof(param));

    joylink_agent_devs_lock();

     /*1 get feedid*/
    AgentDev_t * p = joylink_agent_dev_get(feedid);
    if(p == NULL){
        log_error("agent dev get is error");
        goto ERROR;
    }

    log_info("CLOUD OTA INFO:%s", recPainText + 4);
     /*2 encrypt as dev msg*/
    len = joylink_agent_cloud_rsp_packet(
                (uint8_t*)data, 
                sizeof(data), 
                PT_OTA_ORDER, 
                NULL, 
                0, 
                (uint8_t*)p->sessionkey, 
                (const uint8_t*)(recPainText), 
                src_len);
    if(len < 0){
        log_error("packet error");
        goto ERROR;
    }

     /*3 send dev msg*/
    ret = send(p->fd, data, len, 0);
    if(ret < 0){
        log_error("send error ret:%d", ret);
        goto ERROR;
    }

     /*4 wait msg ack*/
    memset(data, 0, sizeof(data));
    ret = joylink_agent_tcp_recv(p->fd, (char*)data, sizeof(data));
    if (ret == -1 || ret == 0){
        close(p->fd);
        log_info("dev fd close!\r\n");
        goto ERROR;
    }

     /*5 decrypt dev ack*/
    ret = joylink_dencypt_server_req(&param, data, ret, 
            o_result, o_max, feedid);

    if (ret < 1){
        log_debug("dencypt erorr ret");
        goto ERROR;
    }

    *o_len = ret;
    joylink_agent_devs_unlock();
    return  E_RET_OK;
ERROR:
    joylink_agent_devs_unlock();
    return E_RET_ERROR;
}

/**
 *1 Package request msg to cloud
 *2 Send msg to cloud
 *3 Handle the cloud rsponse
 */
void
joylink_agent_cloud_rsp(int32_t type, char * src, int32_t src_len)
{
    switch (type){
        case PT_AGENT_AUTH:
            log_debug("PT_AGENT_AUTH RSP");
            joylink_agent_auth_rsps(src, src_len);
            break;
        case PT_AGENT_HB:
            log_debug("PT_AGENT_HB RSP");
            joylink_agent_HB_rsps(src, src_len);
            break;
        case PT_AGENT_UPLOAD:
            joylink_agent_snap_rsps(src, src_len);
            log_debug("PT_AGENT_UPLOAD");
            break;
        case PT_AGENT_DEV_CTRL:
            log_debug("PT_AGENT_DEV_CTRL");
            joylink_agent_server_ctrl(src, src_len);
            break;
        default:
            log_debug("Unknow param type.\r\n");
            break;
    }
}
