#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#ifndef ESP_8266
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <ifaddrs.h>
#include <netdb.h>
#else
#include "esp_common.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "freertos/semphr.h"
//#include "lwip/apps/sntp.h"
#endif

#include "joylink.h"
#include "joylink_utils.h"
#include "joylink_packets.h"
#include "joylink_crypt.h"
#include "joylink_json.h"
#include "joylink_extern.h"
#include "joylink_sub_dev.h"
#include "joylink_dev_timer.h"
#include "joylink_config.h"

#define GET_HOST_BY_NAME
//#define START_SNTP_SERVER
#define JOYLINK_TASK_STACK_SNTP    (256+64) //remain 160
#define JOYLINK_SNTP_TASK_PRIORITY (2)

extern int _g_model_code_flag;

static void 
joylink_server_st_time_out_check()
{
    log_debug("server st :%d", _g_pdev->server_st);
    if(_g_pdev->hb_lost_count > JL_MAX_SERVER_HB_LOST){
        if (_g_pdev->server_st == JL_SERVER_ST_WORK){
            log_info("Server HB Lost, Retry Auth!");
            _g_pdev->hb_lost_count = 0;
            _g_pdev->server_st = JL_SERVER_ST_AUTH;
        }

        if(_g_pdev->server_st == JL_SERVER_ST_AUTH){
            log_info("Auth ERR, Reconnect!");
            _g_pdev->hb_lost_count = 0;
            _g_pdev->server_st = JL_SERVER_ST_INIT;
            close(_g_pdev->server_socket);
            _g_pdev->server_socket = -1;
        }
    }
}

#ifdef START_SNTP_SERVER
void sys_start_sntp_server(void *para)
{
    int cnt = 0, ret;
    xSemaphoreHandle *xsem_sntp = (xSemaphoreHandle *)para;
    ip_addr_t   *addr   =   (ip_addr_t  *)os_zalloc(sizeof(ip_addr_t));
    sntp_setservername(0,JOYLINK_SNTP_SERVER1_NAME);    //"cn.ntp.org.cn"   set server  0   by  domain  name
    sntp_setservername(1,JOYLINK_SNTP_SERVER2_NAME);    //  set server  1   by  domain  name
    ipaddr_aton("210.72.145.44", addr);
    sntp_setserver(2, addr);    //  set server  2   by  IP  address
    sntp_init();
    ret = sntp_enabled();
    os_free(addr);
    while(1) {
        uint32  current_stamp;
        current_stamp = sntp_get_current_timestamp();
        log_debug("HighWater %d", uxTaskGetStackHighWaterMark(NULL));
        if (current_stamp == 0) {
            log_debug("sntp conn...");
            vTaskDelay(300 / portTICK_RATE_MS);
        } else {
            log_info("sntp ok: %d, %s, %d", current_stamp, sntp_get_real_time(current_stamp), time(NULL));
            break;
        }
        /* if try 50 time */
        if (cnt++>50) {
            sntp_stop();
            system_restart();
            break;
        }
    }
    xSemaphoreGive(*xsem_sntp);
    vTaskDelete(NULL);
}
#endif

static void 
joylink_server_st_init()
{
    int ret = -1;
    struct sockaddr_in saServer; 
    bzero(&saServer, sizeof(saServer)); 

    saServer.sin_family = AF_INET;

    saServer.sin_port = htons(_g_pdev->jlp.server_port);

#ifdef GET_HOST_BY_NAME
    struct hostent *host;
    if((host=gethostbyname(_g_pdev->jlp.joylink_server)) == NULL) {
        perror("gethostbyname error");
        return ;
    }
    saServer.sin_addr = *((struct in_addr *)host->h_addr);
    log_debug("server: %s:%d\n",_g_pdev->jlp.joylink_server, _g_pdev->jlp.server_port);
    log_debug("gethostbyname: %s:%d\n",inet_ntoa(saServer.sin_addr), saServer.sin_port);    
#else
    saServer.sin_addr.s_addr = inet_addr(_g_pdev->jlp.joylink_server);
    log_debug("NO GET_HOST_BY_NAME");
#endif

#ifdef START_SNTP_SERVER
    xSemaphoreHandle xsem_sntp = (xSemaphoreHandle)xSemaphoreCreateBinary();
    xTaskCreate(sys_start_sntp_server,"_sntp", JOYLINK_TASK_STACK_SNTP, &xsem_sntp, JOYLINK_SNTP_TASK_PRIORITY, NULL);
    xSemaphoreTake(xsem_sntp, portMAX_DELAY);
    log_debug("sntp finish");
#endif

    _g_pdev->server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (_g_pdev->server_socket < 0 ){
        log_error("socket() failed!");
        return;
    }
    ret = connect(_g_pdev->server_socket, 
            (struct sockaddr *)&saServer,
            sizeof(saServer));

    if(ret < 0){
        log_error("connect() server failed!");
        close(_g_pdev->server_socket);
        _g_pdev->server_st = 0;
    }else{
        _g_pdev->server_st = 1;
    }
    _g_pdev->hb_lost_count = 0;
}

static void 
joylink_server_st_auth()
{
    int ret;
    int len = joylink_packet_server_auth_rsp();

    ret = send(_g_pdev->server_socket, _g_pdev->send_buff, len, 0);
    if(ret < 0){
        log_error("send error ret:%d", ret);
    }
    log_debug("Auth with server, ret:%d\n", ret);
}

static void 
joylink_server_st_work()
{
    int ret;
    int len = 0; 
    if(E_JLDEV_TYPE_GW != _g_pdev->jlp.devtype){
        len = joylink_packet_server_hb_req();
    }else{
        len = joylink_packet_server_sub_hb_req();
    }    

    ret = send(_g_pdev->server_socket, _g_pdev->send_buff, len, 0);
    if(ret < 0){
        log_error("send error ret:%d", ret);
    }
    log_debug("Heartbeat----->");
    _g_pdev->hb_lost_count ++;
}

int
joylink_proc_server_st()
{
    int interval = 5000;

    if ((!strlen(_g_pdev->jlp.feedid)) || (!strlen(_g_pdev->jlp.joylink_server))) {
        log_debug("server isn't readly");
        return interval;
    }

    log_debug("feed id %s", _g_pdev->jlp.feedid);

    joylink_server_st_time_out_check();

    switch (_g_pdev->server_st){
        case JL_SERVER_ST_INIT:
            joylink_server_st_init();
            interval = 2000;
            break;

        case JL_SERVER_ST_AUTH:
            joylink_server_st_auth();
            if (_g_pdev->hb_lost_count == 0){
                interval = 500;
            }
            interval = 500;
            _g_pdev->hb_lost_count++;
            break;

        case JL_SERVER_ST_WORK:
            joylink_server_st_work();
            interval = 15000;
            break;

        default:
            log_debug("Unknow server st:%d", _g_pdev->server_st);
            break;
    }

    joylink_dev_set_connect_st(_g_pdev->server_st);

    return interval;
}

int
joylink_server_upload_modelcode_req()
{
    int ret = -1;
    int time_v;
    int len;
    char data[JL_MAX_PACKET_LEN] = {0};

    ret = joylink_dev_get_modelcode(data + 4, JL_MAX_PACKET_LEN - 4);

    if(ret > 0){
        time_v = time(NULL);
        memcpy(data, &time_v, 4);

        len = joylink_encypt_server_rsp(
                _g_pdev->send_buff,
                JL_MAX_PACKET_LEN, PT_MODEL_CODE,
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
joylink_server_upload_req()
{
    int ret = -1;
    int time_v;
    int len;
    //char data[JL_MAX_PACKET_LEN];
    char *data = (char *)malloc(JL_MAX_PACKET_LEN);
    bzero(data,JL_MAX_PACKET_LEN);

    ret = joylink_dev_get_snap_shot(data + 4, JL_MAX_PACKET_LEN - 4, 1);

    if(ret > 0){
        time_v = time(NULL);
        log_debug("time_v:%d", time_v);
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
            log_debug("send to server len:%d: ret:%d\n", len, ret);
        }else{
            log_error("send data too big or packe error: ret:%d", ret);
        }
    }

    if(NULL != data) {
        log_debug("free heap：%p", data);
        free(data);
    }

    return ret;
}

static int
joylink_server_subdev_upload_req()
{
    int ret = -1;
    int time_v;
    int len;
    //char data[JL_MAX_PACKET_LEN];
    char *data = (char *)malloc(JL_MAX_PACKET_LEN);
    int i;
    JLDevInfo_t *alldevinfo = NULL;
    int count;
    int scan_type = 0;
    char *snap_shot = NULL;

    bzero(data,JL_MAX_PACKET_LEN);
    alldevinfo = joylink_dev_sub_devs_get(&count, scan_type);

    for(i = 0; i < count; i++){
        snap_shot = joylink_dev_sub_get_snap_shot(alldevinfo[i].jlp.feedid, &ret);
    
        if(snap_shot != NULL){
            ret = joylink_encrypt_sub_dev_data(
                    (uint8_t*)(data + 4 + 32), 
                    JL_MAX_PACKET_LEN - 4 -32,
                    ET_ACCESSKEYAES, 
                    (uint8_t*)alldevinfo[i].jlp.accesskey, 
                    (uint8_t*)snap_shot, 
                    ret);

            free(snap_shot);
            snap_shot = NULL;
        }
        if(ret > 0){
            time_v = time(NULL);
            memcpy(data, &time_v, 4);
            memcpy(data+4, alldevinfo[i].jlp.feedid, 32);

            len = joylink_encypt_server_rsp(_g_pdev->send_buff, JL_MAX_PACKET_LEN, 
                PT_SUB_UPLOAD, _g_pdev->jlp.sessionKey, 
                (uint8_t*)data, ret + 4 + 32) ;

            if(len > 0 && len < JL_MAX_PACKET_LEN){
                ret = send(_g_pdev->server_socket, _g_pdev->send_buff, len, 0);
                if(ret < 0){
                    log_error("send error ret:%d", ret);
                }
                log_debug("send to server len:%d: ret:%d", len, ret);
            }else{
                log_error("send data too big or packet error:ret:%d", ret);
            }
        }
    }

    if(NULL == alldevinfo){
        free(alldevinfo);
    }
    
    if(NULL != data) {
        free(data);
    }

    return ret;
}

static void
joylink_proc_server_auth(uint8_t* recPainText)
{
    JLAuthRsp_t* p = (JLAuthRsp_t*)recPainText;
    bzero(_g_pdev->jlp.sessionKey, 33); 
    memcpy(_g_pdev->jlp.sessionKey, p->session_key, 32);

    log_debug("OK===>Rand=%u,Time:%u, sessionKey:!", 
            p->random_unm, p->timestamp);

    log_info("AUTH OK");

    if(E_JLDEV_TYPE_GW != _g_pdev->jlp.devtype){
        joylink_server_upload_req();
    }
    _g_pdev->server_st = JL_SERVER_ST_WORK;
}

static void
joylink_proc_server_hb(uint8_t* recPainText)
{
    _g_pdev->server_st = JL_SERVER_ST_WORK;
    JLHearBeatRst_t* p = (JLHearBeatRst_t*)recPainText;
    _g_pdev->hb_lost_count = 0;
    
    _g_pdev->cloud_timestamp = p->timestamp;
    log_info("HEAT BEAT OK===>Code=%u,Time:%u!",
            p->code, p->timestamp);

    if(_g_pdev->model_code_flag == 1) {
        joylink_server_upload_modelcode_req();
    }

    if(E_JLDEV_TYPE_GW != _g_pdev->jlp.devtype){
        joylink_server_upload_req();
    }else{
        joylink_server_subdev_upload_req();
    }
}

static void
joylink_proc_server_ctrl(uint8_t* recPainText)
{
    int ret = -1;
    int len;
    //char data[JL_MAX_PACKET_LEN];
    char *data = (char *)malloc(JL_MAX_PACKET_LEN);
    JLContrl_t ctr;
    bzero(&ctr, sizeof(ctr));
    bzero(data,JL_MAX_PACKET_LEN);

    log_debug("Control from server:%s: len:%d",
            recPainText + 12, (int)strlen((char*)(recPainText + 12)));

    if(-1 == joylink_dev_script_ctrl((const char *)recPainText, &ctr, 2)){
        goto RET;
    }
    ret = joylink_packet_script_ctrl_rsp(data, JL_MAX_PACKET_LEN, &ctr, 1);
    log_debug("rsp data:%s:len:%d",
            (char *)(data + 12), 
            (int)strlen((char *)(data + 12)));

    len = joylink_encypt_server_rsp(_g_pdev->send_buff, JL_MAX_PACKET_LEN, 
            PT_SERVERCONTROL, _g_pdev->jlp.sessionKey, 
            (uint8_t*)data, ret);

    if(len > 0 &&  len < JL_MAX_PACKET_LEN){
        ret = send(_g_pdev->server_socket, _g_pdev->send_buff, len, 0);
        if(ret < 0){
            log_error("send error ret:%d", ret);
        }
        log_info("send to server len:%d: ret:%d", len, ret);
    }else{
        log_error("packet error ret:%d", ret);
    }

RET:
    if(NULL != data) {
        free(data);
    }

    return;
}

static void
joylink_proc_server_upload(uint8_t* recPainText)
{
    JLDataUploadRsp_t* p = (JLDataUploadRsp_t *)recPainText;
    log_info("UPLOAD OK timestamp %d code %d", p->timestamp, p->code);
}

static void
joylink_proc_server_sub_hb(uint8_t* recPainText)
{
    JLDataSubHbRsp_t* p = (JLDataSubHbRsp_t *)recPainText;
    log_info("SUB_HB OK -->timestamp %d code %d", p->timestamp, p->code);

    _g_pdev->server_st = JL_SERVER_ST_WORK;
    _g_pdev->hb_lost_count = 0;

    if(E_JLDEV_TYPE_GW != _g_pdev->jlp.devtype){
        joylink_server_upload_req();
    }else{
        joylink_server_subdev_upload_req();
    }
}

static void
joylink_proc_server_sub_upload(uint8_t* recPainText)
{
    JLDataSubUploadRsp_t* p = (JLDataSubUploadRsp_t *)recPainText;
    log_info("SUB DEV UPLOAD OK timestamp %d feedid %s code %d\r\n", 
        p->timestamp, p->feedid, p->code);
}

static void
joylink_proc_server_sub_ctrl(uint8_t* recPainText, unsigned short payloadlen)
{
    int ret = -1;
    int len;
    //char data[JL_MAX_PACKET_LEN];
    char *data = (char *)malloc(JL_MAX_PACKET_LEN);
    JLSubContrl_t ctr;
    bzero(&ctr, sizeof(ctr));
    bzero(data,JL_MAX_PACKET_LEN);
 
    if(-1 == joylink_subdev_script_ctrl(recPainText, &ctr, payloadlen)){
        return;
    }

    ret = joylink_packet_subdev_script_ctrl_rsp(data, JL_MAX_PACKET_LEN, &ctr);

    //int *org_s = (int*)(recPainText + 8);
    //int *p_ser = (int*)(data + 8);
    //log_debug("---org serial:%d:packet serial:%d, %d", *org_s, *p_ser,  ctr.serial);

    log_debug("rsp data:%s:len:%d",
            (char *)(data + 12), 
            (int)strlen((char *)(data + 12)));

    len = joylink_encypt_server_rsp(_g_pdev->send_buff, JL_MAX_PACKET_LEN, 
            PT_SUB_CLOUD_CTRL, _g_pdev->jlp.sessionKey, 
            (uint8_t*)data, ret);

    if(len > 0 &&  len < JL_MAX_PACKET_LEN){
        ret = send(_g_pdev->server_socket, _g_pdev->send_buff, len, 0);
        if(ret < 0){
            log_error("send error ret:%d", ret);
        }
        log_info("send to server len:%d: ret:%d", len, ret);
    }else{
        log_error("packet error ret:%d", ret);
    }

    if(NULL != data) {
        free(data);
    }
    
}

static void
joylink_proc_server_sub_unbind(uint8_t* recPainText, unsigned short payloadlen)
{
    int ret = -1;
    int len;
    //char data[JL_MAX_PACKET_LEN];
    char *data = (char *)malloc(JL_MAX_PACKET_LEN);
    bzero(data,JL_MAX_PACKET_LEN);
    JLSubUnbind_t unbind;
    bzero(&unbind, sizeof(unbind));

    if(-1 == joylink_subdev_unbind(recPainText, &unbind)){
        return;
    }
    ret = joylink_packet_subdev_unbind_rsp(data, JL_MAX_PACKET_LEN, &unbind);
    if(-1 == ret){
        goto RET;
    }
    len = joylink_encypt_server_rsp(_g_pdev->send_buff, JL_MAX_PACKET_LEN,
        PT_SUB_UNBIND, _g_pdev->jlp.sessionKey,
        (uint8_t*)data, payloadlen);

    if(len > 0 &&  len < JL_MAX_PACKET_LEN){
        ret = send(_g_pdev->server_socket, _g_pdev->send_buff, len, 0);
        if(ret < 0){
            log_error("send error ret:%d", ret);
        }
        log_info("send to server len:%d: ret:%d", len, ret);
    }else{
        log_error("packet error ret:%d", ret);
    }

    if(NULL != unbind.info){
        free(unbind.info);
    }
    
RET:
    if(NULL != data) {
        free(data);
    }

    return;
}

void
joylink_server_ota_status_upload_req(JLOtaUpload_t *otaUpload)
{
    int ret = -1;
    int len = 0;
    len = joylink_package_ota_upload_req(otaUpload);

    if(len > 0 && len <JL_MAX_PACKET_LEN){
        ret = send(_g_pdev->server_socket, _g_pdev->send_buff, len, 0);
        if(ret < 0){
            log_error("send error ret:%d", ret);
        }
        log_info("send to server len:%d:ret:%d", len, ret);
    }else{
        log_error("packet error ret:%d", ret);
    }
}

static void
joylink_proc_server_ota_order(uint8_t* json, int json_len)
{
    int ret = -1;
    int len = 0;
    JLOtaOrder_t otaOrder;
    bzero(&otaOrder, sizeof(otaOrder));

    ret = joylink_parse_server_ota_order_req(&otaOrder, (char *)json);
    if(ret != 0){
        return;
    }
    _g_pdev->jlp.crc32 = otaOrder.crc32;
    
    ret = joylink_dev_ota(&otaOrder);
    if(ret != 0){
        log_error("dev ota error:ret:%d", ret);
        return;
    }

    len = joylink_packet_server_ota_order_rsp(&otaOrder);

    if(len > 0 && len < JL_MAX_PACKET_LEN){ 
        ret = send(_g_pdev->server_socket, _g_pdev->send_buff, len, 0);
        if(ret < 0){
            log_error("send error ret:%d", ret);
        }else{
            log_info("send to server len:%d: ret:%d", len, ret);
        }
    }else{
        log_error("packet error ret:%d", ret);
    }

    joylink_dev_ota_status_upload(0, 0, "downloading", 0);
}

static void
joylink_proc_server_ota_upload(uint8_t* json)
{
    int ret = -1;
    JLOtaUploadRsp_t otaUpload;
    bzero(&otaUpload, sizeof(JLOtaUploadRsp_t));

    ret = joylink_parse_server_ota_upload_req(&otaUpload, (const char*)json);
    
    if(ret != 0){
        return;
    }

    extern void joylink_ota_upload_rsp_ok();
    joylink_ota_upload_rsp_ok();
    log_info("OTA UPLOAD OK code %d msg %s",
        otaUpload.code, otaUpload.msg);
}

int
joylink_server_recv(char fd, char *rec_buff, int max)
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

void
joylink_proc_server()
{
    //uint8_t recBuffer[JL_MAX_PACKET_LEN * 2 ];
    //uint8_t recPainText[JL_MAX_PACKET_LEN * 2 + 16];

    uint8_t *recBuffer = (uint8_t*)malloc(JL_MAX_PACKET_LEN * 2);
    uint8_t *recPainText = (uint8_t*)malloc(JL_MAX_PACKET_LEN * 2 + 16);
    bzero(recBuffer, JL_MAX_PACKET_LEN * 2);
    bzero(recPainText, JL_MAX_PACKET_LEN * 2 + 16);
    int ret;
    
    ret = joylink_server_recv(_g_pdev->server_socket, (char*)recBuffer, JL_MAX_PACKET_LEN * 2);
    if (ret == -1 || ret == 0){
        close(_g_pdev->server_socket);
        _g_pdev->server_socket = -1;
        _g_pdev->server_st = 0;
        log_info("Server close, Reconnect!");
        goto RET;
    }

    JLPacketParam_t param;
    ret = joylink_dencypt_server_req(&param, recBuffer, ret, recPainText, JL_MAX_PACKET_LEN * 2);

    if (ret < 1){
        return;
    }

    log_info("Server org ctrl type:%d", param.type);
    switch (param.type){
        case PT_AUTH:
            joylink_proc_server_auth(recPainText);
            break;
        case PT_BEAT:
            joylink_proc_server_hb(recPainText);
            break;
        case PT_MODEL_CODE:
            /*model code only report once, reset the flag*/
            _g_pdev->model_code_flag = 0;
            break;
        case PT_SERVERCONTROL:
            joylink_proc_server_ctrl(recPainText);
            break;
        case PT_UPLOAD:
            joylink_proc_server_upload(recPainText);
            break;
        case PT_SUB_HB:
            joylink_proc_server_sub_hb(recPainText);
            break;
        case PT_SUB_UPLOAD:
            joylink_proc_server_sub_upload(recPainText);
            break;
        case PT_SUB_CLOUD_CTRL:
            joylink_proc_server_sub_ctrl(recPainText, ret);
            break;
        case PT_SUB_UNBIND:
            joylink_proc_server_sub_unbind(recPainText, ret);
            break;
        case PT_OTA_ORDER:
            joylink_proc_server_ota_order((recPainText + 4), (ret - 4));
            break;
        case PT_OTA_UPLOAD:
            joylink_proc_server_ota_upload(recPainText + 4);
            break;
        case PT_TIME_TASK:
            joylink_proc_time_task((char *)recPainText, ret);
            break;
        default:
            log_debug("Unknow param type.");
            break;
    }

RET:
    if (NULL != recBuffer) {
        free(recBuffer);
    }
    
    if (NULL != recPainText) {
        free(recPainText);
    }

    return;
}
