/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2017 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
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

#ifndef _ADAPTER_H_
#define _ADAPTER_H_

#include "esp_common.h"
#include <stdint.h>
#include <sys/time.h>
#include <time.h>

#include "esp_err.h"


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

//ESP8266 RTOS VERSION
typedef xSemaphoreHandle SemaphoreHandle_t;
typedef xQueueHandle QueueHandle_t;

typedef uint32_t TickType_t;


#define TimerHandle_t xTimerHandle
#define BaseType_t BaseType

#define portTICK_PERIOD_MS          ( ( TickType_t ) 1000 / configTICK_RATE_HZ )

#define GET_PERI_REG_BITS2(reg, bit_map, shift)   (   ( (READ_PERI_REG(reg)) >> (shift)) & bit_map   )


#define esp_get_free_heap_size  system_get_free_heap_size
#define sdk_station_config station_config
#define sdk_wifi_get_macaddr wifi_get_macaddr
#define sdk_wifi_set_opmode wifi_set_opmode
#define sdk_wifi_station_set_config wifi_station_set_config
#define sdk_wifi_station_get_connect_status wifi_station_get_connect_status
#define sdk_wifi_station_disconnect wifi_station_disconnect
#define sdk_system_get_sdk_version system_get_sdk_version
#define uart_set_baud UART_SetBaudrate
#define sdk_system_rtc_clock_cali_proc system_rtc_clock_cali_proc
#define sdk_system_get_free_heap_size system_get_free_heap_size
#define sdk_wifi_station_connect wifi_station_connect
#define sdk_wifi_get_macaddr  wifi_get_macaddr
xSemaphoreHandle xSemaphoreCreateBinary();

#ifdef __cplusplus
}
#endif

#endif /* _ADAPTER_H_ */

