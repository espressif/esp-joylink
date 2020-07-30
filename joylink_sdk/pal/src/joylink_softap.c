#include "joylink_string.h"
#include "joylink_memory.h"
#include "joylink_softap.h"

#ifdef __ESP_PAL__
#include "esp_wifi.h"
#include "joylink_log.h"
#include "joylink_extern.h"
#endif

/**
 * @brief:set wifi mode to AP mode.
 *
 * @returns:success 0, others failed
 */
int32_t jl_softap_enter_ap_mode(void)
{
    // open wifi whit AP mode
#ifdef __ESP_PAL__
    log_debug("--  set wifi AP mode");
    esp_wifi_set_mode(WIFI_MODE_AP);
#endif
    return 0;
}


/**
 * @brief:System is expected to get product information that user regiested in Cloud platform.
 *
 * @param[out]: UUID , max length: 8
 * @param[out]: BRAND, max length: 8 
 * @param[out]: CID,   max length: 8
 * @param[out]: dev_soft_ssid max length: 32
 *
 * @returns:success 0, others failed
 */
int32_t jl_softap_get_product_info(char *uuid, char *brand, char *cid, char *dev_soft_ssid)
{
    char *_uuid = CONFIG_JOYLINK_DEVICE_UUID;
    jl_platform_strcpy(uuid, _uuid);
    
    char* _dev_soft_ssid = CONFIG_JOYLINK_SOFTAP_SSID;
    jl_platform_strcpy(dev_soft_ssid, _dev_soft_ssid);
    
    char *_cid = CONFIG_JOYLINK_DEVICE_CID;
    jl_platform_strcpy(cid, _cid);

    char *_brand = CONFIG_JOYLINK_DEVICE_BRAND;
    jl_platform_strcpy(brand, _brand);
    
    return 0;
}

/**
 * @brief:System is expected to connect wifi router with the in parameters.
 *
 * @param[in]: ssid of wifi router
 * @param[in]: passwd  of wifi router
 *
 * @returns:success 0, others failed
 */
int32_t jl_softap_connect_wifi_router(char *ssid, char *passwd)
{
    // step 1 close AP mode

    // step 2 connect to the router
#ifdef __ESP_PAL__
    wifi_config_t config;

    log_debug("--  ready to connect to AP");
    esp_wifi_set_mode(WIFI_MODE_STA);

    memset(&config, 0x0, sizeof(config));
    esp_wifi_get_config(ESP_IF_WIFI_STA, &config);
    memcpy(config.sta.ssid, ssid, strlen(ssid));
    memcpy(config.sta.password, passwd, strlen(passwd));

    esp_wifi_set_config(ESP_IF_WIFI_STA, &config);
    esp_joylink_wifi_save_info(config.sta.ssid, config.sta.password);
    esp_wifi_connect();
#endif
    return 0;
}
