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

#include <stdio.h>
#include <string.h>
#include "esp_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "simple_ringbuffer.h"
#include "adapter/adapter.h"

#define SR_DBG(fmt,...) //os_printf(fmt, ##__VA_ARGS__)

//The ringbuffer structure
typedef struct
{
    SemaphoreHandle_t free_space_sem;          //Binary semaphore, wakes up writing threads when there's more free space
    SemaphoreHandle_t items_buffered_sem; //Binary semaphore, indicates there are new packets in the circular buffer. See remark.
    size_t size;                                //Size of the data storage
    size_t free_size;
    uint8_t *write_ptr;                         //Pointer where the next item is written
    uint8_t *read_ptr;                          //Pointer from where the next item is read
    uint8_t *data;                              //Data storage
} ringbuf_t;

void ringbuffer_delete(RingbufHandle_t ringbuf)
{
    if (ringbuf == NULL) {
        return;
    }
    ringbuf_t* rb = (ringbuf_t*) ringbuf;
    if (rb->data) {
        free(rb->data);
        rb->data = NULL;
    }
    if (rb->free_space_sem) vSemaphoreDelete(rb->free_space_sem);
    if (rb->items_buffered_sem) vSemaphoreDelete(rb->items_buffered_sem);
}

RingbufHandle_t ringbuffer_create(size_t buf_length)
{
    ringbuf_t *rb = malloc(sizeof(ringbuf_t));
    if (rb == NULL) goto err;
    memset(rb, 0, sizeof(ringbuf_t));
    rb->data = malloc(buf_length);
    if (rb->data == NULL) goto err;
    rb->size = buf_length;
    rb->free_size = buf_length;
    rb->read_ptr = rb->data;
    rb->write_ptr = rb->data;
    rb->free_space_sem = xSemaphoreCreateBinary();
    rb->items_buffered_sem = xSemaphoreCreateBinary();
    if (rb->free_space_sem == NULL || rb->items_buffered_sem == NULL) goto err;
    return (RingbufHandle_t) rb;
    err:
    //Some error has happened. Free/destroy all allocated things and return NULL.
    if (rb) {
        free(rb->data);
        if (rb->free_space_sem) vSemaphoreDelete(rb->free_space_sem);
        if (rb->items_buffered_sem) vSemaphoreDelete(rb->items_buffered_sem);
    }
    free(rb);
    return NULL;
}

int ringbuffer_get_free_size(RingbufHandle_t ringbuf)
{
    ringbuf_t* rb = (ringbuf_t*) ringbuf;
    if (rb == NULL) return -1;
    return rb->free_size;
}

int ringbuffer_get_data_size(RingbufHandle_t ringbuf)
{
    ringbuf_t* rb = (ringbuf_t*) ringbuf;
    if (rb == NULL) return -1;
    portENTER_CRITICAL();
    int len = rb->size - rb->free_size;
    portEXIT_CRITICAL();
    return len;
}

void ringbuffer_info(RingbufHandle_t ringbuf)
{
    ringbuf_t* rb = (ringbuf_t*) ringbuf;
    SR_DBG("buf--head: %p; read:%d; write: %d; diff: %d; free: %d\n", rb->data, rb->read_ptr - rb->data, rb->write_ptr
            - rb->data, ringbuffer_get_data_size(ringbuf), rb->free_size);
}

static int ringbuffer_write_buf(RingbufHandle_t ringbuf, uint8_t *buf, int length)
{
    ringbuf_t* rb = (ringbuf_t*) ringbuf;
    if (rb == NULL) return -1;
    if (length == 0) return 0;
    if (length > rb->free_size) {
        return -2;
    }
    int tail_len = (rb->data + rb->size - rb->write_ptr);

    if (length < tail_len) {
        memcpy(rb->write_ptr, buf, length);
        rb->write_ptr += length;
        rb->free_size -= length;
    } else {
        int remain_len = length - tail_len;
        memcpy(rb->write_ptr, buf, tail_len);
        memcpy(rb->data, buf + tail_len, remain_len);
        rb->write_ptr = rb->data + remain_len;
        rb->free_size -= length;
    }
    return length;
}

int ringbuffer_write_from_isr(RingbufHandle_t ringbuf, uint8_t *buf, int length,
        portBASE_TYPE* pHigherPriorityTaskWoken)
{
    ringbuf_t* rb = (ringbuf_t*) ringbuf;
    int res = ringbuffer_write_buf(ringbuf, buf, length);
    if (rb->free_size > 0) {
        xSemaphoreGiveFromISR(rb->free_space_sem, pHigherPriorityTaskWoken);
    }
    xSemaphoreGiveFromISR(rb->items_buffered_sem, pHigherPriorityTaskWoken);
    return res;
}

static int ringbuffer_read_buf(RingbufHandle_t ringbuf, uint8_t *target, int length)
{
    ringbuf_t* rb = (ringbuf_t*) ringbuf;
    if (rb == NULL) return -1;
    int data_size = rb->size - rb->free_size;
    if (data_size < length) {
        return -2;
    }
    int tail_len = (rb->data + rb->size - rb->read_ptr);
    int remain_len = length - tail_len;
    SR_DBG("\n\n\n----READ----\n\n\n");
    SR_DBG("tail: %d; remain: %d\n", tail_len, remain_len);
    ringbuffer_info(ringbuf);
    SR_DBG("prd: %d ; pda: %d\n", *(rb->read_ptr), *(rb->data));
    SR_DBG("==================\n");
    if (rb->read_ptr < rb->write_ptr || tail_len > length) {
        memcpy(target, rb->read_ptr, length);
        rb->read_ptr += length;
        rb->free_size += length;
    } else {
        SR_DBG("**************\n");
        SR_DBG("tail_len: %d\n", tail_len);
        SR_DBG("prd: %d ; pda: %d\n", *(rb->read_ptr), *(rb->data));
        memcpy(target, rb->read_ptr, tail_len);
        memcpy(target + tail_len, rb->data, remain_len);
        SR_DBG("2nd tail_len: %d\n", tail_len);
        SR_DBG("2nd prd: %d ; pda: %d\n", *(rb->read_ptr), *(rb->data));
        SR_DBG("target: %d  +1: %d\n", *target, *(target + 1));
        rb->read_ptr = rb->data + remain_len;
        rb->free_size += length;
    }
    SR_DBG("^^^^^check read&^^6^^^^^\n");
    SR_DBG("2nd target: %d  +1: %d\n", *target, *(target + 1));
    SR_DBG("^^^^^^endof check^^^^^^\n");
    return length;
}

int ringbuffer_fetch_buf(RingbufHandle_t ringbuf, uint8_t *target, int length)
{
    ringbuf_t* rb = (ringbuf_t*) ringbuf;
    if (rb == NULL) return -1;
    int data_size = rb->size - rb->free_size;
    if (data_size < length) {
        return -2;
    }
    int tail_len = (rb->data + rb->size - rb->read_ptr);
    int remain_len = length - tail_len;
    SR_DBG("\n\n\n----READ----\n\n\n");
    SR_DBG("tail: %d; remain: %d\n", tail_len, remain_len);
    ringbuffer_info(ringbuf);
    SR_DBG("prd: %d ; pda: %d\n", *(rb->read_ptr), *(rb->data));
    SR_DBG("==================\n");
    if (rb->read_ptr < rb->write_ptr || tail_len > length) {
        memcpy(target, rb->read_ptr, length);
    } else {
        SR_DBG("**************\n");
        SR_DBG("tail_len: %d\n", tail_len);
        SR_DBG("prd: %d ; pda: %d\n", *(rb->read_ptr), *(rb->data));
        memcpy(target, rb->read_ptr, tail_len);
        memcpy(target + tail_len, rb->data, remain_len);
        SR_DBG("2nd tail_len: %d\n", tail_len);
        SR_DBG("2nd prd: %d ; pda: %d\n", *(rb->read_ptr), *(rb->data));
        SR_DBG("target: %d  +1: %d\n", *target, *(target + 1));
    }
    SR_DBG("^^^^^check read&^^6^^^^^\n");
    SR_DBG("2nd target: %d  +1: %d\n", *target, *(target + 1));
    ringbuffer_info(ringbuf);
    SR_DBG("^^^^^^endof check^^^^^^\n");
    return length;
}

int ringbuffer_read_from_isr(RingbufHandle_t ringbuf, uint8_t *target, int length,
        portBASE_TYPE* pHigherPriorityTaskWoken)
{
    ringbuf_t* rb = (ringbuf_t*) ringbuf;
    portENTER_CRITICAL();
    int len = ringbuffer_read_buf(ringbuf, target, length);
    portEXIT_CRITICAL();

    if (rb->free_size < rb->size) {
        xSemaphoreGiveFromISR(rb->items_buffered_sem, pHigherPriorityTaskWoken);
    }
    xSemaphoreGiveFromISR(rb->free_space_sem, pHigherPriorityTaskWoken);
    return len;
}

int ringbuffer_fetch_from_isr(RingbufHandle_t ringbuf, uint8_t *target, int length,
        portBASE_TYPE* pHigherPriorityTaskWoken)
{
    ringbuf_t* rb = (ringbuf_t*) ringbuf;
    portENTER_CRITICAL();
    int len = ringbuffer_fetch_buf(ringbuf, target, length);
    portEXIT_CRITICAL();

    if (rb->free_size < rb->size) {
        xSemaphoreGiveFromISR(rb->items_buffered_sem, pHigherPriorityTaskWoken);
    }
    xSemaphoreGiveFromISR(rb->free_space_sem, pHigherPriorityTaskWoken);
    return len;
}

//length can't be 0
int ringbuffer_read(RingbufHandle_t ringbuf, uint8_t *target, int length, int ticks_to_wait)
{
    ringbuf_t* rb = (ringbuf_t*) ringbuf;
    bool done = false;
    int rd_len = 0;
    if (length <= 0) {
        return 0;
    }
    while (!done) {
        portBASE_TYPE res = xSemaphoreTake(rb->items_buffered_sem, ticks_to_wait);
        if (res == pdFALSE) {
            return -1;
        }
        portENTER_CRITICAL();
        int read_len = (rb->size - rb->free_size) > length ? length : (rb->size - rb->free_size);
        rd_len = ringbuffer_read_buf(ringbuf, target, read_len);
        portEXIT_CRITICAL();
        if (rd_len > 0) {
            done = true;
        }
    }
    if (rb->free_size < rb->size) {
        xSemaphoreGive(rb->items_buffered_sem);
    }
    xSemaphoreGive(rb->free_space_sem);
    return rd_len;
}

int ringbuffer_fetch(RingbufHandle_t ringbuf, uint8_t *target, int length, int ticks_to_wait)
{
    ringbuf_t* rb = (ringbuf_t*) ringbuf;
    bool done = false;
    int rd_len;
    while (!done) {
        while (rb->size - rb->free_size < length) {
            portBASE_TYPE res = xSemaphoreTake(rb->items_buffered_sem, ticks_to_wait);
            if (res == pdFALSE) {
                return -1;
            }
        }
        portENTER_CRITICAL();
        rd_len = ringbuffer_fetch_buf(ringbuf, target, length);
        portEXIT_CRITICAL();
        if (rd_len > 0) {
            done = true;
        }
    }
    if (rb->free_size < rb->size) {
        xSemaphoreGive(rb->items_buffered_sem);
    }
    xSemaphoreGive(rb->free_space_sem);
    return rd_len;
}

int ringbuffer_return(RingbufHandle_t ringbuf, int length)
{
    ringbuf_t* rb = (ringbuf_t*) ringbuf;
    //make sure length < occupied size
    if (length > (rb->size - rb->free_size)) {
        return -1;
    }
    portENTER_CRITICAL();
    int tail_len = (rb->data + rb->size - rb->read_ptr);
    int remain_len = length - tail_len;
    if (rb->read_ptr < rb->write_ptr || tail_len > length) {
        rb->read_ptr += length;
        rb->free_size += length;
    } else {
        rb->read_ptr = rb->data + remain_len;
        rb->free_size += length;
    }
    portEXIT_CRITICAL();
    return length;
}

int ringbuffer_write(RingbufHandle_t ringbuf, uint8_t *buf, int length, int ticks_to_wait)
{
    ringbuf_t* rb = (ringbuf_t*) ringbuf;
    bool done = false;
    int wr_len;
    while (!done) {
        while ((rb->free_size) <= length) {
            portBASE_TYPE res = xSemaphoreTake(rb->free_space_sem, ticks_to_wait);
            if (res == pdFALSE) {
                return -1;
            }
        }
        portENTER_CRITICAL();
        wr_len = ringbuffer_write_buf(ringbuf, buf, length);
        portEXIT_CRITICAL();
        if (wr_len > 0) {
            done = true;
        }
    }
    if (rb->free_size > 0) {
        xSemaphoreGive(rb->free_space_sem);
    }
    xSemaphoreGive(rb->items_buffered_sem);
    return wr_len;
}

