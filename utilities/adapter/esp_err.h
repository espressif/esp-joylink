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

#ifndef __ESP_ERR_H__
#define __ESP_ERR_H__

#include <stdint.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t esp_err_t;

/* Definitions for error constants. */

#define ESP_OK          0
#define ESP_FAIL        -1

#define ESP_ERR_NO_MEM          0x101
#define ESP_ERR_INVALID_ARG     0x102
#define ESP_ERR_INVALID_STATE   0x103
#define ESP_ERR_INVALID_SIZE    0x104
#define ESP_ERR_NOT_FOUND       0x105
#define ESP_ERR_NOT_SUPPORTED   0x106
#define ESP_ERR_TIMEOUT         0x107
#define ESP_ERR_INVALID_RESPONSE    0x108
#define ESP_ERR_INVALID_CRC     0x109

#define ESP_ERR_WIFI_BASE       0x3000 /*!< Starting number of WiFi error codes */

/**
 * Macro which can be used to check the error code,
 * and terminate the program in case the code is not ESP_OK.
 * Prints the failed statement to serial output.
 */
#define ESP_ERROR_CHECK(x)   do { esp_err_t rc = (x); if (rc != ESP_OK) { assert(0 && #x);} } while(0);

#ifdef __cplusplus
}
#endif

#endif /* __ESP_ERR_H__ */
