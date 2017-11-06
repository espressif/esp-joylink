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
#endif
#include "joylink.h"
#include "joylink_utils.h"
#include "joylink_packets.h"
#include "joylink_crypt.h"
#include "joylink_json.h"
#include "joylink_extern.h"
#include "joylink_sub_dev.h"

JLDevice_t  _g_dev = {
    .server_socket = -1,
    .server_st = 0,
    .hb_lost_count = 0,
    .lan_socket = -1,
    .local_port = 80,
#ifdef ESP_8266
    .jlp.mac= "A0:20:A6:00:14:74",
    .jlp.version = 1,
    .jlp.accesskey = "",//NDJY9396Z9P4KNDU
    .jlp.localkey = "",
    .jlp.feedid = "",
    .jlp.devtype = E_JLDEV_TYPE_NORMAL,
    .jlp.joylink_server = "",
    .jlp.server_port = 2002, 
    .jlp.firmwareVersion = "001",
    .jlp.CID= "011c022b",
    .jlp.modelCode = "A2",
    .jlp.uuid = "GNQIYS",    
    .jlp.lancon = E_LAN_CTRL_ENABLE,
    .jlp.cmd_tran_type = E_CMD_TYPE_JSON
#endif
};

JLDevice_t  *_g_pdev = &_g_dev;

extern void joylink_proc_lan();
extern void joylink_proc_server();
extern int joylink_proc_server_st();
extern int joylink_ecc_contex_init(void);

void
joylink_dev_set_ver(short ver)
{
    _g_pdev->jlp.version = ver;
    joylink_dev_set_attr_jlp(&_g_pdev->jlp);
}

short
joylink_dev_get_ver()
{
    return _g_pdev->jlp.version;
}

void 
joylink_main_loop(void)
{ 
    int ret;
	struct sockaddr_in sin;
    bzero(&sin, sizeof(sin));
	struct timeval  selectTimeOut;
	static uint32_t serverTimer;
	joylink_util_timer_reset(&serverTimer);
	static int interval = 0;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(_g_pdev->local_port);
	_g_pdev->lan_socket = socket(AF_INET, SOCK_DGRAM, 0);
	
	if (_g_pdev->lan_socket < 0){
		return;
	}
	
	int broadcastEnable = 1;
	if (setsockopt(_g_pdev->lan_socket, 
                SOL_SOCKET, SO_BROADCAST, 
                (uint8_t *)&broadcastEnable, 
                sizeof(broadcastEnable)) < 0){
		log_error("SO_BROADCAST ERR");
    }
	
	if(0 > bind(_g_pdev->lan_socket, (SOCKADDR*)&sin, sizeof(SOCKADDR))){
		perror("Bind lan socket error!");
    }
	while (1) {
        //if ((joylink_util_getsys_ms() & 0x7FFFFFFF) < interval) {
        //    printf("system time was reset!\n");
        //    joylink_util_timer_reset(&serverTimer);
        //}
		if (joylink_util_is_time_out(&serverTimer, interval)) {
			joylink_util_timer_reset(&serverTimer);
            if(joylink_dev_is_net_ok() == E_RET_OK){    // check device if connected internet
                interval = joylink_proc_server_st();
            }else{
                interval = 1000 * 3; // delay 3s to wait connect AP
            }
		}
		fd_set  readfds;
		FD_ZERO(&readfds);
		FD_SET(_g_pdev->lan_socket, &readfds);
        
        if (_g_pdev->server_socket != -1 
               && _g_pdev->server_st > 0){
            FD_SET(_g_pdev->server_socket, &readfds);
        }

        int maxFd = (int)_g_pdev->server_socket > 
            (int)_g_pdev->lan_socket ? 
            _g_pdev->server_socket : _g_pdev->lan_socket;

        selectTimeOut.tv_usec = 0L;
        selectTimeOut.tv_sec = (long)1;
		ret = select(maxFd + 1, &readfds, NULL, NULL, &selectTimeOut);
		if (ret < 0) {
			log_error("Select ERR: %s!", strerror(errno));
			continue;
		} else if (ret == 0) {
			continue;
		}
		if (FD_ISSET(_g_pdev->lan_socket, &readfds)) {
            log_debug("lan cmd down");
            joylink_proc_lan();
		} else if ((_g_pdev->server_socket != -1) && 
            (FD_ISSET(_g_pdev->server_socket, &readfds))) {
            log_debug("server cmd down");
            joylink_proc_server();
		}
        log_debug("\n************\nHighWater:%d, free RAM:%d\n*************\n",\
             uxTaskGetStackHighWaterMark( NULL ), system_get_free_heap_size());
	}
}

static void 
joylink_dev_init()
{
    joylink_dev_get_jlp_info(&_g_pdev->jlp);
    log_debug("feedid:%s", _g_pdev->jlp.feedid);
    log_debug("uuid:%s", _g_pdev->jlp.uuid);
    log_debug("accesskey:%s", _g_pdev->jlp.accesskey);
    log_debug("localkey:%s", _g_pdev->jlp.localkey);
}

void 
joylink_main_start(void)
{
    //extern void joylink_init_aes_table(void);
    //joylink_init_aes_table();
    joylink_ecc_contex_init();
    joylink_dev_init();
	joylink_main_loop();
}

#ifdef _TEST_
int 
main()
{
    joylink_main_start();
	return 0;
}
#else

#endif
