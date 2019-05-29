/* --------------------------------------------------
 * @file: test.c
 *
 * @brief: 
 *
 * @version: 1.0
 *
 * @date: 10/08/2015 09:28:27 AM
 *
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

#include "joylink_json.h"
#include "joylink_agent.h"
#include "joylink_agent_devs.h"

extern void
joylink_agent_gw_thread_start();

extern int 
joylink_packet_server_hb_xxx_req(void);

/**
 * brief: 
 */
static void 
joylink_server_st_init()
{
    int ret = -1;
    struct sockaddr_in saServer; 
    bzero(&saServer, sizeof(saServer)); 

    saServer.sin_family = AF_INET;

    /*saServer.sin_port = htons(_g_pdev->jlp.server_port);*/
    saServer.sin_port = htons( JL_AGENT_GW_PORT);

    saServer.sin_addr.s_addr = inet_addr("127.0.0.1");

    _g_pdev->server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (_g_pdev->server_socket < 0 ){
        log_error("socket() failed!\n");
        return;
    }
    ret = connect(_g_pdev->server_socket, 
            (struct sockaddr *)&saServer,
            sizeof(saServer));

    if(ret < 0){
        log_error("connect() server failed!\n");
        close(_g_pdev->server_socket);
        _g_pdev->server_st = 0;
    }else{
        _g_pdev->server_st = 1;
    }
    _g_pdev->hb_lost_count = 0;
}

void test_send()
{
    int ret;
    int len = 0; 

    joylink_server_st_init();

    len = joylink_packet_server_hb_xxx_req();

    ret = send(_g_pdev->server_socket, _g_pdev->send_buff, len, 0);
    if(ret < 0){
        log_error("send error ret:%d", ret);
    }
    log_debug("HB----->");
}

void test_adevs_init()
{
    AgentDev_t ad; 
    memset(&ad, 0, sizeof(ad));
    
    memcpy(ad.feedid, _g_pdev->jlp.feedid, JL_MAX_FEEDID_LEN);
    memcpy(ad.accesskey, _g_pdev->jlp.accesskey, JL_MAX_FEEDID_LEN);
    joylink_agent_dev_add(ad.feedid, ad.accesskey);
    AgentDev_t *p = joylink_agent_dev_get(ad.feedid);

    log_debug("after add :%s:%s", p->feedid, p->accesskey);
}

void test_adevs()
{
    AgentDev_t ad = {.feedid = "feedid111111111", .accesskey= "ffffff"};

    AgentDev_t ab;
    memset(&ab, 0, sizeof(ab));
    memcpy(&ab, &ad, sizeof(AgentDev_t));

    joylink_agent_dev_add(ad.feedid, ad.accesskey);

    AgentDev_t *p = joylink_agent_dev_get(ad.feedid);

    log_debug("after add :%s:%s", p->feedid, p->accesskey);
   
    strcpy(ad.feedid, "666666");
    joylink_agent_dev_upd(ab.feedid, &ad);

    p = joylink_agent_dev_get(ad.feedid);
    if(p != NULL){
        log_debug("after update :%s:%s", p->feedid, p->accesskey);
    }else{
        log_debug("after update get error"); 
    }

    joylink_agent_dev_del(ad.feedid);

    p = joylink_agent_dev_get(ad.feedid);
    if(p == NULL){
        log_debug("after del no found agent dev");
    }
}

extern JLAddAgentDev_t*
joylink_agent_parse_dev_add(const uint8_t* pMsg, int* out_num);

void test_add_agent_dev()
{
    char* ts = "[{\"feedid\":\"1ffff\",\"ackey\":\"1keyfff\"}, {\"feedid\":\"2ffff\",\"ackey\":\"2keyfff\"}]";
    int32_t num = 0;

    int i;
    for(i=0; i < num; i++){
        /*log_debug("---:feedid:%s\nackey:%s", p[i].feedid, p[i].ackey);*/
    }

}

void test_file_save()
{
    JLPInfo_t jlp[10];
    JLPInfo_t fjlp[10];

    memset(&jlp, 0, sizeof(jlp));
    memset(&fjlp, 0, sizeof(fjlp));

    strcpy(jlp[1].feedid, "my feedid");
    log_debug("before save file:%s", jlp[1].feedid);


    char  *file = "joylink_info.txt";
    FILE *outfile, *infile;

    outfile = fopen(file, "wb" );

    fwrite(jlp, sizeof(jlp), 1, outfile );
    fclose(outfile);

    infile = fopen(file, "rb");
    int i;
    for(i = 0; i< 10; i++){
        fread(&fjlp[i], sizeof(JLPInfo_t), 1, infile);
    }
    fclose(infile);

    log_debug("after save file:%s", fjlp[1].feedid);
}


/* Create a bunch of objects as demonstration. */
int main(int argc , char **argv)
{
    if(argc < 2){
        log_error("USAG:%s server/client", argv[0]);
        exit(-1);
    }

    if(!strcmp("server", argv[1])){
        test_adevs_init();
        joylink_agent_gw_thread_start();
        while(1){
            sleep(1);
        }
    }

    if(!strcmp("client", argv[1])){
        test_send();
    }

    if(!strcmp("test", argv[1])){
        /*test_adevs();*/
        /*log_debug("sssssssss:%d", sizeof(JLAuthRsp_t));*/
        /*test_adevs_init();*/
        /*test_add_agent_dev();*/
        test_file_save();
    }

    return 0;
}
