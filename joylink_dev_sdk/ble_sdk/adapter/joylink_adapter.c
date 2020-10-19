#include "joylink_log.h"
#include "joylink_sdk.h"
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

extern int esp_ble_gatts_send_data(uint8_t*frame, int32_t frame_len);
int jl_adapter_send_data(uint8_t* data, uint32_t data_len)
{
    int ret = 0;
    ret = esp_ble_gatts_send_data(data, data_len);
    return ret;
}

int32_t jl_adapter_set_response(uint16_t data_tag, int32_t send_state)
{
    int ret = 0;
    //to do
    return ret;
}

jl_net_config_data_t ble_net_cfg_data;

int jl_adapter_set_config_data(jl_net_config_data_t *data)
{
    int status = 0;
    memset(&jd_ble_config, 0x0, sizeof(wifi_config_t));
    memset(&ble_net_cfg_data, 0x0, sizeof(jl_net_config_data_t));
    memcpy(&ble_net_cfg_data, data, sizeof(jl_net_config_data_t));
    printf("ssid: %s\n", data->ssid);
    memcpy(jd_ble_config.sta.ssid, data->ssid, strlen((char *)data->ssid));
    printf("password: %s\n", data->password);
    memcpy(jd_ble_config.sta.password, data->password, strlen((char *)data->password));
    printf("token: %s\n", data->token);
    printf("url: %s\n", data->url);

    return status;
}

int jl_adapter_set_system_time(jl_timestamp_t *time)
{
    struct timeval now = { .tv_sec = time->second , .tv_usec = time->usecond};
    settimeofday(&now, NULL);
	
    struct tm *lt;
    lt = localtime(&lt);
    printf("%d/%d/%d %d:%d:%d\n", lt->tm_year+1900, lt->tm_mon, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);
    
    return 0;
}