#include "joylink_sdk.h"
#include "joylink_platform.h"
#include "joylink_adapter.h"

#include <sys/time.h>
#include <fcntl.h>

#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_partition.h"
#include "tcpip_adapter.h"
#include "esp_ota_ops.h"
#include "sdkconfig.h"

wifi_config_t  jd_ble_config;
bool get_rst = false;
jl_net_config_data_t ble_net_cfg_data;

extern int esp_ble_gatts_send_data(uint8_t*frame, int32_t frame_len);

/*************************************************
Function: jl_adapter_send_data
Description: SDK适配接口，GATTS Characteristic发送数据
Calls: GATTS Characteristic发送数据接口
Called By: @jl_send_frame：SDK内部函数
Input: @data：发送的数据
       @data_len：发送的数据长度
Output: None
Return: 0：成功
       -1：失败
Others:
*************************************************/
int32_t jl_adapter_send_data(uint8_t* data, uint32_t data_len)
{
    int ret = 0;
    ret = esp_ble_gatts_send_data(data, data_len);
    return ret;
}

/*************************************************
Function: jl_adapter_set_config_data
Description: SDK适配接口，获取配网与绑定参数
Calls: 配网与绑定接口
Called By: @jl_process_user_data：SDK内部函数
Input: @data->ssid：配网数据，WiFi SSID
       @data->ssid_len：WiFi SSID长度
       @data->password：配网数据，WiFi密码
       @data->password_len：WiFi密码长度
       @data->url：绑定云端链接
       @data->url_len：绑定云端链接长度
       @data->token：绑定参数
       @data->token_len：绑定参数长度
Output: None
Return: 0：成功
       -1：失败
Others: 设置激活绑定接口：joylink_dev_active_start(data->url, data->token);
*************************************************/
int32_t jl_adapter_set_config_data(jl_net_config_data_t *data)
{
    int status = 0;
    ble_net_cfg_data.url = (uint8_t *) malloc(data->url_len + 1);
    ble_net_cfg_data.token = (uint8_t *) malloc(data->token_len + 1);

    strncpy(ble_net_cfg_data.url, data->url, data->url_len + 1);
    strncpy(ble_net_cfg_data.token, data->token, data->token_len + 1);

    memcpy(jd_ble_config.sta.ssid, data->ssid, strlen((char *)data->ssid));
    memcpy(jd_ble_config.sta.password, data->password, strlen((char *)data->password));

    printf("ble-url: %s\n", ble_net_cfg_data.url);
    printf("ble-token: %s\n", ble_net_cfg_data.token);
    //to do
    //joylink_dev_active_start(data->url, data->token);
    return status;
}

/*************************************************
Function: jl_adapter_set_system_time
Description: SDK适配接口，设置系统时间
Calls: 设置系统时间接口
Called By: SDK内部函数
Input: @time->second：秒
       @time->usecond：微妙
Output: None
Return: 0：成功
       -1：失败
Others: None
*************************************************/
int32_t jl_adapter_set_system_time(jl_timestamp_t *time)
{
    struct timeval now = { .tv_sec = time->second , .tv_usec = time->usecond};
    settimeofday(&now, NULL);

    struct tm *lt;
    lt = localtime(&lt);
    printf("%d/%d/%d %d:%d:%d\n", lt->tm_year+1900, lt->tm_mon, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);
    
    return 0;
}