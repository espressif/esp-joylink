#include "joylink_string.h"
#include "joylink_memory.h"
#include "joylink_softap.h"

/**
 * @brief:set wifi mode to AP mode.
 *
 * @returns:success 0, others failed
 */
int32_t jl_softap_enter_ap_mode(void)
{
    // open wifi whit AP mode
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
    char *_uuid = "CDEFC2";
    jl_platform_strcpy(uuid, _uuid);
    
    char* _dev_soft_ssid="martin";
    jl_platform_strcpy(dev_soft_ssid, _dev_soft_ssid);
    
    char *_cid = "25";
    jl_platform_strcpy(cid, _cid);

    char *_brand="45";
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

    return 0;
}
