// Copyright 2015-2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "string.h"

#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_system.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sys/socket.h"

#include "joylink_softap.h"
#include "joylink_softap_extern.h"
#include "esp_joylink.h"

#include "joylink_auth_uECC.h"
#include "joylink_crypt.h"

#define TAG   "softap"

#define ESP_JOYLINK_TCP_LOCAL_PORT         3000

#define ESP_JOYLINK_UDP_LOCAL_PORT          4320
#define ESP_JOYLINK_UDP_REMOTE_PORT         9999

static int esp_joylink_tcp_socket = -1;
static int esp_joylink_udp_socket = -1;

void print_buf(uint8 *buf,uint8 size)
{
	uint8 i;
	for(i = 0;i < size; i++)
	{
		printf("%x ",*(buf+i));
	}
	printf("\n");
}

void udp_send(const void *data, uint16 len)
{
    struct sockaddr_in dest_addr;
    
    if (esp_joylink_udp_socket >= 0) {
        memset(&dest_addr, 0, sizeof(dest_addr));
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_addr.s_addr = INADDR_BROADCAST;
        dest_addr.sin_port = htons(ESP_JOYLINK_UDP_REMOTE_PORT);
        dest_addr.sin_len = sizeof(dest_addr);
        
        sendto(esp_joylink_udp_socket, data, len, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    }
}

void tcp_send(const void *data, uint16 len)
{
    if (esp_joylink_tcp_socket >= 0) {
        write(esp_joylink_tcp_socket,data,len);
    }
}

void get_mac_address(uint8 *address,uint8 len)
{
}

uint32 apiRand (void)
{
    return esp_random();
}

void aes_cbc_encrypt (uint8 PlainText[],
                       uint32 PlainTextLength,
                       uint8 Key[],
                       uint32 KeyLength,
                       uint8 IV[],
                       uint32 IVLength,
                       uint8 CipherText[],
                       uint32 *CipherTextLength)
{
    int maxOutLen = *CipherTextLength;

    int len = device_aes_encrypt(Key, KeyLength, IV, PlainText, PlainTextLength, CipherText, maxOutLen);

    *CipherTextLength = len;
}

void aes_cbc_decrypt (uint8 CipherText[],
                       uint32 CipherTextLength,
                       uint8 Key[],
                       uint32 KeyLength,
                       uint8 IV[],
                       uint32 IVLength,
                       uint8 PlainText[],
                       uint32 *PlainTextLength)
{
    int maxOutLen = *PlainTextLength;
    
    {
        int loop = 0;
        printf("CipherText(%d):\r\n",CipherTextLength);
        for (loop = 0;loop < CipherTextLength;loop++) {
            printf("%02x ",CipherText[loop]);
        }
        printf("\r\n");
        
        printf("Key(%d):\r\n",KeyLength);
        for (loop = 0;loop < KeyLength;loop++) {
            printf("%02x ",Key[loop]);
        }
        printf("\r\n");
        
        printf("IV(%d):\r\n",IVLength);
        for (loop = 0;loop < IVLength;loop++) {
            printf("%02x ",IV[loop]);
        }
        printf("\r\n");
    }
    
    
    int len = device_aes_decrypt(Key, KeyLength, IV, CipherText, CipherTextLength, PlainText, maxOutLen);

    {
        int loop = 0;
        printf("PlainText(%d):\r\n",len);
        for (loop = 0;loop < len;loop++) {
            printf("%02x ",PlainText[loop]);
        }
    }
    *PlainTextLength = len;
}


void joylink_udp_send_publickey(void *arg)
{
	for(;;) {
    	joylink_softap_udpbroad();
		vTaskDelay (100 / portTICK_RATE_MS);
	}
}

static void esp_joylink_softap_task(void *pvParameters)
{
    struct sockaddr_in saddr = { 0 };
    socklen_t socklen = 0;
    int err = 0;
    int server_sock = -1;
    int buf_size = 512;
    int softap_ret = 0; 
    int len = 0;
    joylinkSoftAP_Result_t* pRet = NULL;
    uint8 *buf = (uint8*)malloc(buf_size);
    wifi_config_t config;
	xTaskHandle udp_send_handle;
    
	extern uint8 softap_ssid[MAX_LEN_OF_SSID+1];
	
	joylink_softap_init();
	memset(&config,0x0,sizeof(config));
    esp_wifi_set_mode(WIFI_MODE_APSTA);
    
	printf("ssid:%s\r\n",softap_ssid);
	config.ap.ssid_len = strlen((char*)softap_ssid);
	if (config.ap.ssid_len > sizeof(config.ap.ssid)) {
		config.ap.ssid_len = sizeof(config.ap.ssid);
	}
	strncpy((char*)config.ap.ssid, (char*)softap_ssid, config.ap.ssid_len);
	config.ap.max_connection = 4;
	config.ap.channel = 12;
	esp_wifi_set_config(WIFI_IF_AP,&config);
	
	if (buf == NULL) {
        goto err1;
    }
    
    if (esp_joylink_udp_socket < 0) {
        esp_joylink_udp_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (esp_joylink_udp_socket < 0) {
            ESP_LOGE(TAG, "Failed to create socket. Error %d", errno);
            goto err1;
        }

        saddr.sin_family = PF_INET;
        saddr.sin_port = htons(ESP_JOYLINK_UDP_LOCAL_PORT);
        saddr.sin_addr.s_addr = htonl(INADDR_ANY);
        err = bind(esp_joylink_udp_socket, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
        if (err < 0) {
            ESP_LOGE(TAG, "Failed to bind udp socket. Error %d", errno);
            goto err2;
        }
    }
    
    if (server_sock < 0) {
        server_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_IP);
        if (server_sock < 0) {
            goto err2;
        }
        
        saddr.sin_family = PF_INET;
        saddr.sin_port = htons(ESP_JOYLINK_TCP_LOCAL_PORT);
        saddr.sin_addr.s_addr = htonl(INADDR_ANY);
        err = bind(server_sock, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
        if (err < 0) {
            ESP_LOGE(TAG, "Failed to bind tcp socket. Error %d", errno);
            goto err3;
        }
        
        if (listen(server_sock, 32) != 0) {
            ESP_LOGI(TAG, "listen failed");
            goto err3;
        }
    }
	xTaskCreate(joylink_udp_send_publickey, "joylink_udp_send_publickey", 2048, NULL, 1, &udp_send_handle);

    
    for (;;) {
        socklen = sizeof(saddr);
        esp_joylink_tcp_socket = accept(server_sock, (struct sockaddr *)&saddr, &socklen);
        
        for (;;) {
            len = read(esp_joylink_tcp_socket, buf, buf_size);

            if (len >= 0) {
                buf[len] = 0;
                ESP_LOGI(TAG,"softap tcp server recv: %s", buf);

                joylink_softap_tcppacket(buf, len);
                
                pRet = (joylinkSoftAP_Result_t*)malloc(sizeof(joylinkSoftAP_Result_t));
                softap_ret = joylink_softap_result(pRet);
                
				ESP_LOGI(TAG,"softap_ret:%d", softap_ret);
                if (softap_ret == 0) {
                    ESP_LOGI(TAG,"SSID:%s", pRet->ssid);
                    ESP_LOGI(TAG,"PASSWORD:%s", pRet->pass);
                    break;
                }
                
                free(pRet);
            } else {
                close(esp_joylink_tcp_socket);
                esp_joylink_tcp_socket = -1;
            }
        }
        
        if (softap_ret == 0) {
            break;
        }
    }
    
    
    memset(&config,0x0,sizeof(config));
    esp_wifi_get_config(ESP_IF_WIFI_STA, &config);
    memcpy(config.sta.ssid, pRet->ssid ,sizeof(config.sta.ssid));
    memcpy(config.sta.password, pRet->pass, sizeof(config.sta.password));
    ESP_LOGI(TAG,"connect to AP, SSID:%s, password:%s", config.sta.ssid, config.sta.password);
    esp_wifi_set_config(ESP_IF_WIFI_STA,&config);
    esp_wifi_connect();
    
    esp_joylink_wifi_save_info(config.sta.ssid, config.sta.password);
    free(pRet);

	vTaskDelete(udp_send_handle);
    
err3:
    close(server_sock);
    server_sock = -1;
err2:
    close(esp_joylink_udp_socket);
    esp_joylink_udp_socket = -1;
err1:
    
    esp_wifi_set_mode(WIFI_MODE_STA);
    vTaskDelete(NULL);
}

void esp_joylink_softap_innet(void)
{
    ESP_LOGI(TAG, "*********************************");
    ESP_LOGI(TAG, "*    ENTER SOFTAP CONFIG MODE   *");
    ESP_LOGI(TAG, "*********************************");
    xTaskCreate(esp_joylink_softap_task, "softap_task", 1024*4, NULL, 1, NULL);
}
