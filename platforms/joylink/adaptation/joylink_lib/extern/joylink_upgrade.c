/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#include "esp_common.h"
#include "lwip/mem.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "upgrade.h"
#include "lwip/netdb.h"

#include "joylink_log.h"
//#include "esp_joylink.h"

typedef struct {
    struct sockaddr_in sockaddrin;          /**< socket of upgrading */
    uint8 ota_url[100];                             /**< the url of upgrading server */
    char ota_server[32];
    uint16_t ota_port;
    char *ota_bin_path;
} joylink_upgrade_t;

joylink_upgrade_t *g_joylink_upgrade = NULL;

#define JOYLINK_OTA_TASK_STACK  (256+128)
#define JOYLINK_OTA_TASK_PRIOTY (3)
#define JOYLINK_OTA_TIMEOUT     (120000) /*!< 120000ms */
#define DNS_GET_HOST_RETRY      (5)
#define JOYLINK_OTA_RX_MAXLEN   (1024)  /*!< limit the each packet length of OTA */

xTimerHandle xTimeOtaCheck = NULL;

//#define pheadbuffer
static const char pheadbuffer[] = 
"GET %s HTTP/1.1\r\n\
Host: %s\r\n\
Referer:http://%s\r\n\
Connection: close\r\n\
Pragma: no-cache\r\n\
User-Agent: Mozilla/4.0 (compatible; MSIE 5.00; Windows 98)\r\n\
Accept: */*\r\n\
Content-Type: application/json\r\n\r\n";

/*********************global param define start ******************************/
LOCAL os_timer_t upgrade_timer;
LOCAL uint32 totallength = 0;
LOCAL uint32 sumlength = 0;
LOCAL bool flash_erased = false;
xTaskHandle *ota_task_handle = NULL;

/*********************global param define end *******************************/
#if 1
/******************************************************************************
 * FunctionName : upgrade_recycle
 * Description  : recyle upgrade task, if OTA finish switch to run another bin
 * Parameters   :
 * Returns      : none
*******************************************************************************/
LOCAL void upgrade_recycle( xTimerHandle xTimer )
{
	totallength = 0;
    sumlength = 0;
    flash_erased = false;
    system_upgrade_recycle();
	printf("upgrade_flag_check:%d\r\n",system_upgrade_flag_check());
    if (system_upgrade_flag_check() == UPGRADE_FLAG_FINISH) {
        joylink_dev_ota_status_upload(2, 100, "ota ok", 1);
        printf("[ota] HighWater %d\n", uxTaskGetStackHighWaterMark(NULL));
        //save version to flash
		vTaskDelay(100 / portTICK_RATE_MS);
//		uart_swap_switch_off(); 
        system_upgrade_reboot(); // if need
    } else {
//    	uart_swap_switch_off();
        joylink_dev_ota_status_upload(3, 100, "ota fail", 1);
        vTaskDelay(100 / portTICK_RATE_MS);
		system_restart();
	}
	vTaskDelete(ota_task_handle);
    ota_task_handle = NULL;
}

/******************************************************************************
 * FunctionName : upgrade_download
 * Description  : parse http response ,and download remote data and write in flash
 * Parameters   : int sta_socket : ota client socket fd
 *                char *pusrdata : remote data
 *                length         : data length
 * Returns      : none
*******************************************************************************/
void upgrade_download(int sta_socket,char *pusrdata, unsigned short length)
{
    char *ptr = NULL;
    char *ptmp2 = NULL;
    char lengthbuffer[32];
    static int s_up_cnt = 0;
    if (totallength == 0 && (ptr = (char *)strstr(pusrdata, "\r\n\r\n")) != NULL &&
            (ptr = (char *)strstr(pusrdata, "Content-Length")) != NULL) {
        printf("recv header: %s\r\n",pusrdata);
        ptr = (char *)strstr(pusrdata, "\r\n\r\n");
        length -= ptr - pusrdata;
        length -= 4;
        ptr = (char *)strstr(pusrdata, "Content-Length: ");
        if (ptr != NULL) {
            ptr += 16;
            ptmp2 = (char *)strstr(ptr, "\r\n");
            if (ptmp2 != NULL) {
                memset(lengthbuffer, 0, sizeof(lengthbuffer));
                memcpy(lengthbuffer, ptr, ptmp2 - ptr);
                sumlength = atoi(lengthbuffer);
                if(sumlength > 0) {
                	if (false == system_upgrade(pusrdata, sumlength)) {
                		system_upgrade_flag_set(UPGRADE_FLAG_IDLE);	
                        goto ota_recycle;
                	}
                	flash_erased = true;
                	ptr = (char *)strstr(pusrdata, "\r\n\r\n");
                	if (false == system_upgrade(ptr + 4, length)){
                		system_upgrade_flag_set(UPGRADE_FLAG_IDLE);	
                        goto ota_recycle;
                	}
                	totallength += length;
                	printf("sumlength = %d\n",sumlength);
                    s_up_cnt = 0;
                	return;
                }
            } else {
                printf("sumlength failed\n");
                system_upgrade_flag_set(UPGRADE_FLAG_IDLE);	
                goto ota_recycle;
            }
        } else {
            printf("Content-Length: failed\n");
            system_upgrade_flag_set(UPGRADE_FLAG_IDLE);	
            goto ota_recycle;
        } 
    } else {
        totallength += length;
        printf("totallen = %d\n",totallength);
        if (false == system_upgrade(pusrdata, length)){
        	system_upgrade_flag_set(UPGRADE_FLAG_IDLE);
            goto ota_recycle;
        }
        if (totallength >= sumlength) {
	        printf("upgrade file download finished.\n");
//            joylink_dev_ota_status_upload(1, 100, "fw installing", 0);
	        if(upgrade_crc_check(system_get_fw_start_sec(),sumlength) != true) {
				printf("upgrade crc check failed !\n");
		        system_upgrade_flag_set(UPGRADE_FLAG_IDLE);	
		        goto ota_recycle;
		    }
            system_upgrade_flag_set(UPGRADE_FLAG_FINISH);
            goto ota_recycle;      
        } else {
            /* count num upload once process */
//            s_up_cnt++;
//            if (0 == s_up_cnt%80) {
//                joylink_dev_ota_status_upload(0, totallength*100/sumlength, "downloading", 0);
//                printf("[ota] HighWater %d\n", uxTaskGetStackHighWaterMark(NULL));
//            }
            return;
        }        
    }

ota_recycle :
	printf("go to ota recycle");
    close(sta_socket);
    upgrade_recycle(NULL);
}

//http://storage.360buyimg.com/devh5/1500354795468_user.bin
int parse_url(const char *url)
{
    int ret = -1;
    int i;
    char *pstr = NULL;
    char *ptal = NULL;
    if (NULL == url) {
        return ret;
    }
    pstr = strstr(url, "http");
    if (NULL != pstr) {
        ptal = strstr(pstr+4, "://");
        ptal += 3;
        /* find server ip and port */
        for (i=0; i<(strlen(url)-7); i++) {
            if (ptal[i] == ':' || ptal[i] == '/'){
                break;
            }
        }
        memcpy(g_joylink_upgrade->ota_server, ptal, i);
        if (ptal[i] == '/') {
            g_joylink_upgrade->ota_port = 80;
            g_joylink_upgrade->ota_bin_path = ptal+i;
        } else {
            ptal += (i+1);
            pstr = strstr(ptal, "/");
            if (NULL == pstr) {
                return ret;
            }
            char port[5];
            bzero(port, 5);
            memcpy(port, ptal, pstr-ptal);
            g_joylink_upgrade->ota_port = atoi(port);
            g_joylink_upgrade->ota_bin_path = pstr+1;
        }
        ret = 0;
    }
    return ret;
}

int parse_server_url(const char *server_url, struct in_addr* out_addr)
{
    struct hostent *host;
    if((host=gethostbyname(server_url)) == NULL) {
        perror("gethostbyname error");
        return -1;
    }
    memcpy(out_addr, (struct in_addr *)host->h_addr, sizeof(struct in_addr));
    return 0;
}


/******************************************************************************
 * FunctionName : fota_begin
 * Description  : ota_task function
 * Parameters   : task param
 * Returns      : none
*******************************************************************************/
void fota_begin(void *pvParameters)
{
    int recbytes;
    int sin_size;
    int sta_socket;
    struct sockaddr_in *remote_ip;
    log_debug("fota begin");
    
    char *pbuf = (char *)zalloc(JOYLINK_OTA_RX_MAXLEN);
    if(NULL == pbuf) {
        log_error("err");
    }
    
    /* find out ota server and port */
    if(0 != parse_url(g_joylink_upgrade->ota_url)) {
        log_error("err");
    }
    /* DNS parse - get host by name */
    if(0 != parse_server_url(g_joylink_upgrade->ota_server, &g_joylink_upgrade->sockaddrin.sin_addr) ) {
        log_error("err");
    }
    g_joylink_upgrade->sockaddrin.sin_port = htons(g_joylink_upgrade->ota_port);
    log_info("server: %s:%d\n",g_joylink_upgrade->ota_server, g_joylink_upgrade->ota_port);
	log_info("gethostbyname: %s:%d\n",\
        inet_ntoa(g_joylink_upgrade->sockaddrin.sin_addr), g_joylink_upgrade->sockaddrin.sin_port);
    log_debug("high water: %d", uxTaskGetStackHighWaterMark(NULL));
    while (1) {
	    sta_socket = socket(PF_INET,SOCK_STREAM,0);
	    if (-1 == sta_socket)
	    {
	        close(sta_socket);
	        printf("socket fail !\r\n");
	        continue;
	    }
        g_joylink_upgrade->sockaddrin.sin_family = AF_INET;
        remote_ip = &g_joylink_upgrade->sockaddrin;
        log_debug("socket ok, connect to %s\r\n",inet_ntoa(remote_ip->sin_addr));
	    if(0 != connect(sta_socket,(struct sockaddr *)(remote_ip),sizeof(struct sockaddr)))
	    {
	        close(sta_socket);
	        printf("connect fail!\r\n");
	        system_upgrade_flag_set(UPGRADE_FLAG_IDLE);	
	        upgrade_recycle(NULL);
	    }//inet_ntoa(g_joylink_upgrade->sockaddrin.sin_addr)
	    sprintf(pbuf, pheadbuffer, g_joylink_upgrade->ota_bin_path,\
                                   g_joylink_upgrade->ota_server,\
                                   g_joylink_upgrade->ota_server);
	    log_debug("\n%s", pbuf);
        if(write(sta_socket,pbuf,strlen(pbuf)+1) < 0) {
            close(sta_socket);
            log_error("send fail\n");
            free(pbuf);
            system_upgrade_flag_set(UPGRADE_FLAG_IDLE);	
            upgrade_recycle(NULL);
	    }
        while((recbytes = read(sta_socket ,pbuf,JOYLINK_OTA_RX_MAXLEN)) >= 0){           
            if(recbytes != 0 ) {
                upgrade_download(sta_socket,pbuf,recbytes);
            }
        }
        log_debug("recbytes = %d",recbytes);
        if(recbytes < 0){
            log_error("read data fail!\r\n");
            close(sta_socket);
            system_upgrade_flag_set(UPGRADE_FLAG_IDLE);	
	        upgrade_recycle(NULL);
        }
    }
	free(pbuf);
	close(sta_socket);
    system_upgrade_flag_set(UPGRADE_FLAG_IDLE);	
    upgrade_recycle(NULL);
} 

int joylink_upgrade_start(const char *url)
{
    if (NULL == g_joylink_upgrade) {
        g_joylink_upgrade = (joylink_upgrade_t *)zalloc(sizeof(joylink_upgrade_t));
    }
    if (NULL == g_joylink_upgrade) {
        log_error("err");
        return -1;
    }
    strcpy(g_joylink_upgrade->ota_url, url);
    system_upgrade_flag_set(UPGRADE_FLAG_START);
    system_upgrade_init();
	xTaskCreate(fota_begin, "fota_task", JOYLINK_OTA_TASK_STACK, NULL, JOYLINK_OTA_TASK_PRIOTY, ota_task_handle);

//    os_timer_disarm(&upgrade_timer);
//    os_timer_setfn(&upgrade_timer, (os_timer_func_t *)upgrade_recycle, NULL);
//    os_timer_arm(&upgrade_timer, JOYLINK_OTA_TIMEOUT, 0);

    if (NULL != xTimeOtaCheck) {
        xTimerStop(xTimeOtaCheck,portMAX_DELAY);
        xTimerDelete(xTimeOtaCheck,portMAX_DELAY);
        xTimeOtaCheck = NULL;
    }
    xTimeOtaCheck = xTimerCreate("ota_timer",
               JOYLINK_OTA_TIMEOUT/portTICK_RATE_MS,
               pdFAIL,
               NULL,
               upgrade_recycle);
   xTimerStart(xTimeOtaCheck, portMAX_DELAY);
   return 0;
}

#endif

