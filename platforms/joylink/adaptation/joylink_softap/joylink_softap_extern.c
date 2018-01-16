
#include "joylink_softap_extern.h"
#include "lwip/sockets.h"
#include "lwip/err.h"
#include "lwip/ip_addr.h"
#include "lwip/api.h"
#include "esp_common.h"
#include "joylink_aes.h"
#include "joylink_softap.h"
#include "joylink_log.h"

int softap_connfd = -1;

int joylink_udp_send(const void *data, uint16 len)
{
    int ret;
    int sock_fd;
    struct sockaddr_in dest_addr;

    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = INADDR_BROADCAST;
    dest_addr.sin_port = htons(UDP_REMOTE_PORT);
    dest_addr.sin_len = sizeof(dest_addr);

    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    joylink_assert(sock_fd >= 0);

    while (1) {
        sendto(sock_fd, (uint8*)data, len, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        vTaskDelay(100 / portTICK_RATE_MS);
    }
}

void joylink_tcp_send(const void *data, uint16 len)
{
    int length = write(softap_connfd, data, len);
    log_debug("data length of tcp send:%d", length);
}

void aes_cbc_encrypt (uint8 PlainText[],uint32 PlainTextLength,uint8 Key[],uint32 KeyLength,uint8 IV[],uint32 IVLength,uint8 CipherText[],uint32 *CipherTextLength)
{
    int maxOutLen = *CipherTextLength;
    log_debug("aes plaintext len:%d", PlainTextLength);
    log_debug("PlainText:");
    print_buf(PlainText, PlainTextLength);
    log_debug("Key:");
    print_buf(Key, KeyLength);
    log_debug("IV:");
    print_buf(IV, KeyLength);

    int len = device_aes_encrypt(Key, KeyLength, IV, PlainText, PlainTextLength, CipherText, maxOutLen);

    log_debug("CipherText:");
    print_buf(CipherText, len);
    
    *CipherTextLength = len;
    log_debug("len after encrypt:%d", len);
}

void aes_cbc_decrypt (uint8 CipherText[],uint32 CipherTextLength,uint8 Key[],uint32 KeyLength,uint8 IV[],uint32 IVLength,uint8 PlainText[],uint32 *PlainTextLength)
{
    int maxOutLen = *PlainTextLength;
    log_debug("maxOutLen: %d", maxOutLen);

    log_debug("Key:");
    print_buf(Key, KeyLength);

    log_debug("IV:");
    print_buf(IV, KeyLength);

    log_debug("CipherText:");
    print_buf(CipherText, CipherTextLength);

    int len = device_aes_decrypt(Key, KeyLength, IV, CipherText, CipherTextLength, PlainText, maxOutLen);

    log_debug("PlainText:");
    print_buf(PlainText, len);
    
    *PlainTextLength = len;
    log_debug("len after decrypt:%d", len);
}

void get_mac_address(uint8 *address,uint8 len)
{
    wifi_get_macaddr(0, address);
}

unsigned long os_random(void);

uint32 apiRand (void)
{
    return os_random();
}


void joylink_softap_setup(void)
{
    wifi_set_opmode(SOFTAP_MODE);
    joylink_softap_init();
    struct softap_config dev_ap_config;
    bzero(&dev_ap_config,sizeof(dev_ap_config));
    wifi_softap_get_config(&dev_ap_config);
    sprintf(dev_ap_config.ssid, softap_ssid);
    sprintf(dev_ap_config.password, DEFAULT_PASSWORD);
    dev_ap_config.ssid_len = SIZE_SSID;
    dev_ap_config.authmode = AUTH_OPEN;
    dev_ap_config.channel = 6;
    dev_ap_config.max_connection = 4;
    wifi_softap_set_config(&dev_ap_config);
    
    struct ip_info ip_info;
    ip_info.ip.addr=ipaddr_addr(SOFTAP_GATEWAY_IP);
    ip_info.gw.addr=ipaddr_addr(SOFTAP_GATEWAY_IP);
    ip_info.netmask.addr=ipaddr_addr("255.255.255.0");
    wifi_softap_dhcps_stop();
    wifi_set_ip_info(SOFTAP_IF,&ip_info);
    wifi_softap_dhcps_start();

    wifi_softap_get_config(&dev_ap_config);
    wifi_get_ip_info(SOFTAP_IF,&ip_info);
    log_debug("******** Softap Param ********");
    log_debug("ssid:%s",dev_ap_config.ssid);
    log_debug("beacon_interval:%u",dev_ap_config.beacon_interval);
    log_debug("ip:%s",inet_ntoa((ip_info.ip)));
    log_debug("gw:%s",inet_ntoa((ip_info.gw)));
    log_debug("netmask:%s",inet_ntoa((ip_info.netmask)));

}

int joylink_softap_tcp_server(void)
{
    struct sockaddr_in server, client;
    socklen_t socklen = sizeof(client);
    int fd = -1, len, ret;
    int opt = 1, buf_size = 512, msg_size = 512;
    joylinkSoftAP_Result_t ap_info;

    log_debug("setup softap and tcp-server");

    char *buf = (char*)malloc(buf_size);
    if (buf == NULL) {
        log_debug("malloc error\n");
    }

    fd = socket(AF_INET, SOCK_STREAM, 0);
    joylink_assert(fd >= 0);

    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(SOFTAP_GATEWAY_IP);
    server.sin_port = htons(SOFTAP_TCP_SERVER_PORT);

    ret = bind(fd, (struct sockaddr *)&server, sizeof(server));
    joylink_assert(!ret);

    ret = listen(fd, 10);
    joylink_assert(!ret);

    log_info("server %x %d created", ntohl(server.sin_addr.s_addr),
            ntohs(server.sin_port));

    softap_connfd = accept(fd, (struct sockaddr *)&client, &socklen);
    joylink_assert(softap_connfd > 0);
    log_info("client %x %d connected", ntohl(client.sin_addr.s_addr),
            ntohs(client.sin_port));

    while (1) {
        len = read(softap_connfd, buf, buf_size);
        joylink_assert(len >= 0);

        buf[len] = 0;
        log_debug("softap tcp server recv: %s", buf);

        joylink_softap_tcppacket(buf, len);
        if(joylink_softap_success() == TRUE) {
            break;
        }
    }

    close(softap_connfd);
    close(fd);
    if (buf != NULL) {
        free(buf);
    }
    return 0;
}

void joylink_udp_send_publickey(void *arg)
{
    joylink_softap_udpbroad();
}

void joylink_softap_innet_task(void *arg)
{
    joylink_softap_setup();
    xTaskHandle udp_send_handle;
    xTaskCreate(joylink_udp_send_publickey, "joylink_udp_send_publickey", 256+80, NULL, 5, &udp_send_handle);
    joylink_softap_tcp_server();

    joylinkSoftAP_Result_t ap_info;
    joylink_softap_result(&ap_info);

    wifi_set_opmode(STATION_MODE);
    struct station_config sta_config;
    wifi_station_get_config(&sta_config);
    sprintf(sta_config.ssid, ap_info.ssid);
    sprintf(sta_config.password, ap_info.pass);
    log_info("connect to AP, SSID:%s, password:%s", sta_config.ssid, sta_config.password);
    wifi_station_set_config(&sta_config);
    wifi_station_connect();

    vTaskDelete(udp_send_handle);
    vTaskDelete(NULL);
}
void joylink_softap_innet(void)
{
    log_info("*********************************");
    log_info("*    ENTER SOFTAP CONFIG MODE   *");
    log_info("*********************************");
    xTaskCreate(joylink_softap_innet_task, "joylink_softap_innet_task", 512+128, NULL, 5, NULL);
}



