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

#ifndef __SIMPLE_RINGBUFFER_H__
#define __SIMPLE_RINGBUFFER_H__

#include "esp_common.h"
#include "c_types.h"
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void*  RingbufHandle_t;

RingbufHandle_t ringbuffer_create(size_t buf_length);
int ringbuffer_get_free_size(RingbufHandle_t rb);
int ringbuffer_get_data_size(RingbufHandle_t rb);

int ringbuffer_read(RingbufHandle_t ringbuf, uint8_t *target, int length, int ticks_to_wait);
int ringbuffer_write(RingbufHandle_t ringbuf, uint8_t *buf, int length, int ticks_to_wait);
int ringbuffer_fetch(RingbufHandle_t ringbuf, uint8_t *target, int length, int ticks_to_wait);

int ringbuffer_return(RingbufHandle_t ringbuf, int length);

int ringbuffer_read_from_isr(RingbufHandle_t ringbuf, uint8_t *buf, int length, portBASE_TYPE* pHigherPriorityTaskWoken);
int ringbuffer_write_from_isr(RingbufHandle_t ringbuf, uint8_t *buf, int length, portBASE_TYPE* pHigherPriorityTaskWoken);
int ringbuffer_fetch_from_isr(RingbufHandle_t ringbuf, uint8_t *target, int length, portBASE_TYPE* pHigherPriorityTaskWoken);

#ifdef __cplusplus
}
#endif

#endif

