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

#ifndef _IOT_BUTTON_H_
#define _IOT_BUTTON_H_

#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (* button_cb)(void*);
typedef void* button_handle_t;
typedef enum {
    BUTTON_ACTIVE_HIGH = 1,    /*!<button active level: high level*/
    BUTTON_ACTIVE_LOW = 0,     /*!<button active level: low level*/
} button_active_t;

typedef enum {
    BUTTON_PUSH_CB = 0,   /*!<button push callback event */
    BUTTON_RELEASE_CB,    /*!<button release callback event */
    BUTTON_TAP_CB,        /*!<button quick tap callback event(will not trigger if there already is a "PRESS" event) */
} button_cb_type_t;

/**
 * @brief Init button functions
 *
 * @param gpio_num GPIO index of the pin that the button uses
 * @param cb_num "Press action" call back function number, users can register their own callback functions.
 * @param active_level button hardware active level.
 *        For "BUTTON_ACTIVE_LOW" it means when the button pressed, the GPIO will read low level.
 *
 * @return A button_handle_t handle to the created button object, or NULL in case of error.
 */
button_handle_t button_dev_init(gpio_num_t gpio_num, int cb_num, button_active_t active_level);

/**
 * @brief Register a callback function for a "TAP" action.
 *
 * @param type callback function type
 * @param cb callback function for "TAP" action.
 * @param arg Parameter for callback function
 * @param interval_tick a filter to bypass glitch, unit: FreeRTOS ticks.
 * @param btn_handle handle of the button object
 * @note
 *        Button callback functions execute in the context of the timer service task.
 *        It is therefore essential that button callback functions never attempt to block.
 *        For example, a button callback function must not call vTaskDelay(), vTaskDelayUntil(),
 *        or specify a non zero block time when accessing a queue or a semaphore.
 * @return
 *     - ESP_OK Success
 *     - ESP_FAIL Parameter error
 */
//esp_err_t button_dev_add_tap_cb(button_cb cb, void* arg, int interval_tick, button_handle_t btn_handle);
esp_err_t button_dev_add_tap_cb(button_cb_type_t type, button_cb cb, void* arg, int interval_tick, button_handle_t btn_handle);

/**
 * @brief
 *
 * @param cb_idx Index for "PRESS" callback function.
 * @param cb callback function for "PRESS" action.
 * @param arg Parameter for callback function
 * @param interval_tick a filter to bypass glitch, unit: FreeRTOS ticks.
 * @param btn_handle handle of the button object
 * @note
 *        Button callback functions execute in the context of the timer service task.
 *        It is therefore essential that button callback functions never attempt to block.
 *        For example, a button callback function must not call vTaskDelay(), vTaskDelayUntil(),
 *        or specify a non zero block time when accessing a queue or a semaphore.
 * @return
 *     - ESP_OK Success
 *     - ESP_FAIL Parameter error
 */
esp_err_t button_dev_add_press_cb(int cb_idx, button_cb cb, void* arg, int interval_tick, button_handle_t btn_handle);

/**
 * @brief Delete button object and free memory
 * @param btn_handle handle of the button object
 *
 * @return
 *     - ESP_OK Success
 *     - ESP_FAIL Parameter error
 */
esp_err_t button_dev_free(button_handle_t btn_handle);

/**
 * @brief Remove "TAP" callback
 * @param type callback function event type
 * @param btn_handle The handle of the button object
 * @return
 *     - ESP_OK Success
 */
esp_err_t button_dev_rm_tap_cb(button_cb_type_t type, button_handle_t btn_handle);

/**
 * @brief Remove "PRESS" callback
 * @param cb_idx Index for "PRESS" callback function.
 * @param btn_handle The handle of the button object
 * @return
 *     - ESP_OK Success
 */
esp_err_t button_dev_rm_press_cb(int cb_idx, button_handle_t btn_handle);

#ifdef __cplusplus
}
#endif

#endif
