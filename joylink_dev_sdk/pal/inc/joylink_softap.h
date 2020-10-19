#ifndef _JOYLINK_SOFTAP_H_
#define _JOYLINK_SOFTAP_H_

#include "joylink_stdint.h"

/**
 * @brief:set wifi mode to AP mode.
 *
 * @returns:success 0, others failed
 */
int32_t jl_softap_enter_ap_mode(void);


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
int32_t jl_softap_get_product_info(char *uuid, char *brand, char *cid, char *dev_soft_ssid);

/**
 * @brief:System is expected to connect wifi router with the in parameters.
 *
 * @param[in]: ssid of wifi router
 * @param[in]: passwd  of wifi router
 *
 * @returns:success 0, others failed
 */
int32_t jl_softap_connect_wifi_router(char *ssid, char *passwd);

/**
 * @brief: save wifi information
 *
 * @param[in] ssid
 * @param[in] password
 * 
 * @returns: void
 */
void esp_joylink_wifi_save_info(uint8_t*ssid,uint8_t*password);
#endif
