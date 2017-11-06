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

#include "c_types.h"
#include <stdio.h>
#include <string.h>
#include "esp_common.h"
#include "status_led/led.h"
#include "driver/gpio.h"

#define QUICK_BLINK_FREQ_DEFAULT    5
#define SLOW_BLINK_FREQ_DEFAULT     2

static uint8_t g_quick_blink_freq = QUICK_BLINK_FREQ_DEFAULT;
static uint8_t g_slow_blink_freq = SLOW_BLINK_FREQ_DEFAULT;

#define STATUS_LED_DBG(fmt,...) os_printf(fmt, ##__VA_ARGS__)

#define ESP_CHECK(a, ret) if(!(a)) {    \
        os_printf("led: check error. file %s line %d\n", __FILE__, __LINE__);    \
        return (ret);   \
        }
#define POINTER_ASSERT(param, ret)  ESP_CHECK((param) != NULL, (ret))

typedef struct {
    led_status_t state;
    led_dark_level_t dark_level;
    led_mode_t mode;
    os_timer_t led_tm;
    uint8_t io_num;
    uint8_t led_level;
} led_dev_t;

static void led_stop_blink(led_handle_t led_handle)
{
    led_dev_t* led_dev = (led_dev_t*) led_handle;
    os_timer_disarm(&(led_dev->led_tm));
}

static void led_level_set(led_handle_t led_handle, uint8_t level)
{
    led_dev_t* led_dev = (led_dev_t*) led_handle;
    level = led_dev->mode==LED_NIGHT_MODE ? 0 : level;
    gpio_set_level(led_dev->io_num, (level ^ led_dev->dark_level));
    led_dev->led_level = level;
}

static bool led_blink(led_handle_t led_handle, uint32_t period_ms)
{
    led_dev_t* led_dev = (led_dev_t*) led_handle;
    POINTER_ASSERT(led_handle, false);
    ESP_CHECK(period_ms != 0, false);
    os_timer_arm(&(led_dev->led_tm), period_ms / 2, 1);
    return true;
}

static void led_blink_cb(void *arg)
{
    led_dev_t *led_dev = (led_dev_t*) arg;
    led_dev->led_level = 0x01 & (~led_dev->led_level);
    led_level_set(led_dev, led_dev->led_level);
}

bool led_setup(uint8_t quick_blink_fre, uint8_t slow_blink_fre)
{
    ESP_CHECK(quick_blink_fre != 0, false);
    ESP_CHECK(slow_blink_fre != 0, false);
    g_quick_blink_freq = quick_blink_fre;
    g_slow_blink_freq = slow_blink_fre;
    return true;
}

led_handle_t led_create(uint8_t io_num, led_dark_level_t dark_level)
{
    led_dev_t *led_p = (led_dev_t*)zalloc(sizeof(led_dev_t));
    POINTER_ASSERT(led_p, NULL);
    led_p->io_num = io_num;
    led_p->dark_level = dark_level;
    led_p->state = LED_NORMAL_OFF;
    led_p->mode = LED_NORMAL_MODE;
    gpio_config_t gpio_conf;
    gpio_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_conf.mode = GPIO_MODE_OUTPUT;
    gpio_conf.pin_bit_mask = (1 << io_num);
//    gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&gpio_conf);
    led_state_write(led_p, LED_NORMAL_OFF);
    os_timer_disarm(&(led_p->led_tm));
    os_timer_setfn(&(led_p->led_tm), led_blink_cb, (void*)led_p);
    return (led_handle_t) led_p;
}

bool led_delete(led_handle_t led_handle)
{
    POINTER_ASSERT(led_handle, false);
    led_dev_t* led_dev = (led_dev_t*) led_handle;
    os_timer_disarm(&(led_dev->led_tm));
    free(led_handle);
    return true;
}

bool led_state_write(led_handle_t led_handle, led_status_t state)
{
    led_dev_t* led_dev = (led_dev_t*) led_handle;
    POINTER_ASSERT(led_dev, false);
    switch (state) {
        case LED_NORMAL_OFF:
            led_stop_blink(led_handle);
            led_level_set(led_handle, 0);
            break;
        case LED_NORMAL_ON:
            led_stop_blink(led_handle);
            led_level_set(led_handle, 1);
            break;
        case LED_QUICK_BLINK:
            led_blink(led_handle, 1000 / g_quick_blink_freq);
            break;
        case LED_SLOW_BLINK:
            led_blink(led_handle, 1000 / g_slow_blink_freq);
            break;
        default:
            return false; 
            break;
    }
    led_dev->state = state;
    return true;
}

bool led_mode_write(led_handle_t led_handle, led_mode_t mode)
{
    POINTER_ASSERT(led_handle, false);
    led_dev_t *led_dev = (led_dev_t*) led_handle;
    led_dev->mode = mode;
    led_state_write(led_handle, led_dev->state);
    return true;
}

bool led_blink_custom_freq(led_handle_t led_handle, uint32_t period_ms)
{
    POINTER_ASSERT(led_handle, false);
    ESP_CHECK(period_ms != 0, false);
    led_dev_t *led_dev = (led_dev_t*) led_handle;
    led_blink(led_handle, period_ms);
    led_dev->state = LED_BLINK_CUSTOM_FREQ;
    return true;
}

led_status_t led_state_read(led_handle_t led_handle)
{
    led_dev_t* led_dev = (led_dev_t*) led_handle;
    return led_dev->state;
}

led_mode_t led_mode_read(led_handle_t led_handle)
{
    led_dev_t* led_dev = (led_dev_t*) led_handle;
    return led_dev->mode;
}
