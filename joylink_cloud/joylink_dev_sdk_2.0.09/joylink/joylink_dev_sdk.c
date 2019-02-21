#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if defined(__MTK_7687__)
#include <stdint.h>
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#else
#include <stdarg.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
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
#include "joylink_dev.h"
#include "joylink_sub_dev.h"
#include "joylink_config_handle.h"
#include "joylink_extern.h"

JLDevice_t  _g_dev = {
    .jlp.feedid = "12345678901234567890123456789012",
    .jlp.accesskey = "key45678901234567890123456789012",
    .server_socket = -1,
    .server_st = 0,
    .hb_lost_count = 0,
    .lan_socket = -1,
    .local_port = 80
};

JLDevice_t  *_g_pdev = &_g_dev;

extern int
joylink_proc_server_st();

extern void
joylink_proc_lan();

extern void
joylink_proc_server();

extern int 
joylink_ecc_contex_init(void);

extern void 
joylink_cloud_fd_lock();

extern void 
joylink_cloud_fd_unlock();

extern void
joylink_agent_gw_thread_start();

/**
 * brief: 
 *
 * @Param: ver
 */
void
joylink_dev_set_ver(short ver)
{
    _g_pdev->jlp.version = ver;
    joylink_dev_set_attr_jlp(&_g_pdev->jlp);
}

/**
 * brief: 
 *
 * @Returns: 
 */
short
joylink_dev_get_ver()
{
    return _g_pdev->jlp.version;
}

extern int
joylink_is_usr_timestamp_ok(char *usr, int timestamp);

void 
joylink_test()
{
    /*int t = time(NULL);*/
    /*int t = 10;*/
    int ttret;
    static int index = 0; 
    char buff[64] = {0};
    
    sprintf(buff, "192.168.8.%d", index);
    ttret = joylink_is_usr_timestamp_ok(buff, index);
    index++;
    printf("----- user:%s\n", buff); 

    if(ttret == 0){
        printf("----- timestamp is invalid\n"); 
    }else{
        printf("----- timestamp is valid\n"); 
    }
}

extern void
joylink_agent_req_cloud_proc();

char getin_config_flag = 0;

/**
 * brief: 
 */
void 
joylink_main_loop(void)
{
    int ret;
	struct sockaddr_in sin;
    bzero(&sin, sizeof(sin));

	struct timeval  selectTimeOut;
	static uint32_t serverTimer;
	static int interval = 0;

	joylink_util_timer_reset(&serverTimer);

	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(_g_pdev->local_port);
	_g_pdev->lan_socket = socket(AF_INET, SOCK_DGRAM, 0);

	if (_g_pdev->lan_socket < 0){
		printf("socket() failed!\n");
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

	while (1){
        /*joylink_test();*/

	if(getin_config_flag == 1){
		getin_config_flag = 0;
		joylink_config_start(60*1000);
	}
	else if(getin_config_flag == 2)
	{
		getin_config_flag = 0;
		joylink_config_stop();
	}

        joylink_cloud_fd_lock();

		if (joylink_util_is_time_out(serverTimer, interval)){
			joylink_util_timer_reset(&serverTimer);
            if(joylink_dev_is_net_ok()){
                interval = joylink_proc_server_st();
            }else{
                interval = 1000 * 10;
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
		if (ret < 0){
			printf("Select ERR: %s!\r\n", strerror(errno));
            goto UNLOOK_FD;
		}else if (ret == 0){
            goto UNLOOK_FD;
		}else{

            if (FD_ISSET(_g_pdev->lan_socket, &readfds)){
                joylink_proc_lan();
            }else if((_g_pdev->server_socket != -1) && 
                (FD_ISSET(_g_pdev->server_socket, &readfds))){
                joylink_proc_server();
            }
        }

        UNLOOK_FD:
        joylink_cloud_fd_unlock();
#ifdef _AGENT_GW_
        if(E_RET_TRUE == is_joylink_server_st_work()){
            /*log_debug("proc agent_req_cloud proc");*/
            joylink_agent_req_cloud_proc();
        }
#endif
	}
}


/**
 * brief: 
 */
int
joylink_check_cpu_mode(void)  
{  
    union
    {  
        int data_int;  
        char data_char;  
    } temp;

    temp.data_int = 1;  

    if(temp.data_char != 1){
	return -1;
    }else{
	return 0;
    }
}

/**
 * brief: 
 */
static void 
joylink_dev_init()
{
    /**
     *NOTE: If your model_code_flag is configed in cloud, 
     *Please set _g_pdev->model_code_flag = 1. 
     */
    /*_g_pdev->model_code_flag = 1;*/
	char mac[16] = {0};
	char key[68] = {0};

	printf("\n/**************info********************/\n");
	printf("sdk version: %s\n", _VERSION_);
	printf("dev version: %d\n",JLP_VERSION);
	printf("dev uuid: %s\n", JLP_UUID);
	printf("dev type: %d\n", E_JLDEV_TYPE_GW);
	printf("dev lan strl: %d\n", JLP_LAN_CTRL);
	printf("dev cmd type: %d\n", JLP_CMD_TYPE);

	if(joylink_dev_get_user_mac(mac) < 0){
    		printf("dev mac: error!\n");
	}else{
		printf("dev mac: %s\n", mac);
	}
	if(joylink_dev_get_private_key(key) < 0){
    		printf("dev key: error!\n");
	}else{
		printf("dev key: %s\n", key);
	}
	if(joylink_check_cpu_mode() == 0){
    		printf("plt mode: --ok! little-endian--\n");
	}else{
		printf("plt mode: --error! big-endian--\n");
	}
	printf("/*************************************/\n\n");

	joylink_dev_get_jlp_info(&_g_pdev->jlp);
	joylink_dev_get_idt(&_g_pdev->idt);
	log_debug("--->feedid:%s", _g_pdev->jlp.feedid);
	log_debug("--->uuid:%s", _g_pdev->jlp.uuid);
	log_debug("--->accesskey:%s", _g_pdev->jlp.accesskey);
	log_debug("--->localkey:%s", _g_pdev->jlp.localkey);
}

/**
 * brief: 
 *
 * @Returns: 
 */  

int 
joylink_main_start()
{
    joylink_ecc_contex_init();
    joylink_dev_init();

#ifdef _AGENT_GW_
    joylink_agent_devs_load();
    joylink_agent_gw_thread_start();
#endif
	joylink_main_loop();
	return 0;
}
