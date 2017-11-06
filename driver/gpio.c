// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <esp_common.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"

static const char* GPIO_TAG = "gpio";

#define GPIO_CHECK(a, str, ret_val) \
    if (!(a)) { \
        printf("[gpio]%s\n", str); \
        return (ret_val); \
    }

const uint32_t GPIO_PIN_MUX_REG[GPIO_PIN_COUNT] = {
    GPIO_PIN_REG_0,
    GPIO_PIN_REG_1,
    GPIO_PIN_REG_2,
    GPIO_PIN_REG_3,
    GPIO_PIN_REG_4,
    GPIO_PIN_REG_5,
    GPIO_PIN_REG_6,
    GPIO_PIN_REG_7,
    GPIO_PIN_REG_8,
    GPIO_PIN_REG_9,
    GPIO_PIN_REG_10,
    GPIO_PIN_REG_11,
    GPIO_PIN_REG_12,
    GPIO_PIN_REG_13,
    GPIO_PIN_REG_14,
    GPIO_PIN_REG_15,
};

typedef struct {
    gpio_isr_t fn;   /*!< isr function */
    void* args;      /*!< isr function args */
} gpio_isr_func_t;

static gpio_isr_func_t* gpio_isr_func = NULL;

esp_err_t gpio_pullup_en(gpio_num_t gpio_num)
{
    GPIO_CHECK(GPIO_IS_VALID_GPIO(gpio_num), "GPIO number error", ESP_ERR_INVALID_ARG);
    portENTER_CRITICAL();
    PIN_PULLUP_EN(GPIO_PIN_MUX_REG[gpio_num]);
    portEXIT_CRITICAL();
    return ESP_OK;
}

esp_err_t gpio_pullup_dis(gpio_num_t gpio_num)
{
    GPIO_CHECK(GPIO_IS_VALID_GPIO(gpio_num), "GPIO number error", ESP_ERR_INVALID_ARG);
    portENTER_CRITICAL();
    PIN_PULLUP_DIS(GPIO_PIN_MUX_REG[gpio_num]);
    portEXIT_CRITICAL();
    return ESP_OK;
}

esp_err_t gpio_pulldown_en(gpio_num_t gpio_num)
{
    //pull-down is not supported on esp8266
    return ESP_OK;
}

esp_err_t gpio_pulldown_dis(gpio_num_t gpio_num)
{
    //pull-down is not supported on esp8266
    return ESP_OK;
}

esp_err_t gpio_set_intr_type(gpio_num_t gpio_num, gpio_int_type_t intr_type)
{
    GPIO_CHECK(GPIO_IS_VALID_GPIO(gpio_num), "GPIO number error", ESP_ERR_INVALID_ARG);
    GPIO_CHECK(intr_type < GPIO_INTR_MAX, "GPIO interrupt type error", ESP_ERR_INVALID_ARG);
    uint32 pin_reg;
    portENTER_CRITICAL();
    SET_PERI_REG_BITS(GPIO_REG_ADDR(gpio_num), GPIO_PIN_INT_TYPE, intr_type, GPIO_PIN_INT_TYPE_S);
    portEXIT_CRITICAL();
    return ESP_OK;
}

esp_err_t gpio_intr_enable()
{
    _xt_isr_unmask(1 << ETS_GPIO_INUM);
    return ESP_OK;
}

esp_err_t gpio_intr_disable()
{
    _xt_isr_mask(1 << ETS_GPIO_INUM);
    return ESP_OK;
}

static esp_err_t gpio_output_disable(gpio_num_t gpio_num)
{
    WRITE_PERI_REG(GPIO_ENABLE_W1TC_REG, 0x1 << gpio_num);
    return ESP_OK;
}

static esp_err_t gpio_output_enable(gpio_num_t gpio_num)
{
    WRITE_PERI_REG(GPIO_ENABLE_W1TS_REG, 0x1 << gpio_num);
    return ESP_OK;
}

esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level)
{
    GPIO_CHECK(GPIO_IS_VALID_OUTPUT_GPIO(gpio_num), "GPIO output gpio_num error", ESP_ERR_INVALID_ARG);

    if (level) {
        WRITE_PERI_REG(GPIO_OUT_W1TS_REG, 1 << gpio_num);
    } else {
        WRITE_PERI_REG(GPIO_OUT_W1TC_REG, 1 << gpio_num);
    }

    return ESP_OK;
}

int gpio_get_level(gpio_num_t gpio_num)
{
    return (READ_PERI_REG(GPIO_IN_REG) >> gpio_num) & 0x1;
}

esp_err_t gpio_set_pull_mode(gpio_num_t gpio_num, gpio_pull_mode_t pull)
{
    GPIO_CHECK(GPIO_IS_VALID_GPIO(gpio_num), "GPIO number error", ESP_ERR_INVALID_ARG);
    GPIO_CHECK(pull <= GPIO_FLOATING, "GPIO pull mode error", ESP_ERR_INVALID_ARG);
    esp_err_t ret = ESP_OK;

    switch (pull) {
        case GPIO_PULLUP_ONLY:
            gpio_pullup_en(gpio_num);
            break;

        case GPIO_FLOATING:
            gpio_pullup_dis(gpio_num);
            break;

        default:
            printf("Unknown pull up/down mode,gpio_num=%u,pull=%u\n", gpio_num, pull);
            ret = ESP_ERR_INVALID_ARG;
            break;
    }

    return ret;
}

esp_err_t gpio_set_direction(gpio_num_t gpio_num, gpio_mode_t mode)
{
    GPIO_CHECK(GPIO_IS_VALID_GPIO(gpio_num), "GPIO number error", ESP_ERR_INVALID_ARG);
    esp_err_t ret = ESP_OK;

    //input is always connected
    //set output enable
    if (mode & GPIO_MODE_DEF_OUTPUT) {
        gpio_output_enable(gpio_num);
    } else {
        gpio_output_disable(gpio_num);
    }

    if (mode & GPIO_MODE_DEF_OD) {
        SET_PERI_REG_MASK(GPIO_REG_ADDR(gpio_num), GPIO_PIN_DRIVER);
    } else {
        CLEAR_PERI_REG_MASK(GPIO_REG_ADDR(gpio_num), GPIO_PIN_DRIVER);
    }

    return ret;
}

esp_err_t gpio_config(const gpio_config_t* io_config)
{
    uint64_t gpio_pin_mask = (io_config->pin_bit_mask);
    uint32_t io_reg = 0;
    uint32_t io_num = 0;
    uint8_t input_en = 0;
    uint8_t output_en = 0;
    uint8_t od_en = 0;
    uint8_t pu_en = 0;
    uint8_t pd_en = 0;

    if (io_config->pin_bit_mask == 0 || io_config->pin_bit_mask >= (((uint64_t) 1) << GPIO_PIN_COUNT)) {
        printf("[gpio]:GPIO_PIN mask error \n");
        return ESP_ERR_INVALID_ARG;
    }

    do {
        io_reg = GPIO_PIN_MUX_REG[io_num];

        if (((gpio_pin_mask >> io_num) & BIT(0)) && io_reg) {
            gpio_set_direction(io_num, io_config->mode);

            if (io_config->pull_up_en) {
                pu_en = 1;
                gpio_pullup_en(io_num);
            } else {
                gpio_pullup_dis(io_num);
            }

            printf("GPIO[%d]| InputEn: %d| OutputEn: %d| OpenDrain: %d| Pullup: %d| Pulldown: %d| Intr:%d \n", io_num, input_en, output_en, od_en, pu_en, pd_en, io_config->intr_type);
            gpio_set_intr_type(io_num, io_config->intr_type);

            if ((0x1 << io_num) & (GPIO_SEL_0 | GPIO_SEL_2 | GPIO_SEL_4 | GPIO_SEL_5)) {
                PIN_FUNC_SELECT(io_reg, 0);
            } else {
                PIN_FUNC_SELECT(io_reg, 3);
            }
        }

        io_num++;
    } while (io_num < GPIO_PIN_COUNT);

    return ESP_OK;
}

void IRAM_ATTR gpio_intr_service(void* arg)
{
    uint32_t gpio_num = 0;
    //read status to get interrupt status for GPIO0-31
    uint32_t gpio_intr_status;
    gpio_intr_status = READ_PERI_REG(GPIO_STATUS_REG);

    if (gpio_isr_func == NULL) {
        return;
    }

    do {
        if (gpio_intr_status & BIT(gpio_num)) { //gpio0-gpio31
            WRITE_PERI_REG(GPIO_STATUS_W1TC_REG, 0x1 << gpio_num);

            if (gpio_isr_func[gpio_num].fn != NULL) {
                gpio_isr_func[gpio_num].fn(gpio_isr_func[gpio_num].args);
            }
        }
    } while (++gpio_num < GPIO_PIN_COUNT);
}

esp_err_t gpio_isr_handler_add(gpio_num_t gpio_num, gpio_isr_t isr_handler, void* args)
{
    GPIO_CHECK(gpio_isr_func != NULL, "GPIO isr service is not installed, call gpio_install_isr_service() first", ESP_ERR_INVALID_STATE);
    GPIO_CHECK(GPIO_IS_VALID_GPIO(gpio_num), "GPIO number error", ESP_ERR_INVALID_ARG);
    portENTER_CRITICAL();
    gpio_intr_disable();

    if (gpio_isr_func) {
        gpio_isr_func[gpio_num].fn = isr_handler;
        gpio_isr_func[gpio_num].args = args;
    }

    gpio_intr_enable();
    portEXIT_CRITICAL();
    return ESP_OK;
}

esp_err_t gpio_isr_handler_remove(gpio_num_t gpio_num)
{
    GPIO_CHECK(gpio_isr_func != NULL, "GPIO isr service is not installed, call gpio_install_isr_service() first", ESP_ERR_INVALID_STATE);
    GPIO_CHECK(GPIO_IS_VALID_GPIO(gpio_num), "GPIO number error", ESP_ERR_INVALID_ARG);
    portENTER_CRITICAL();
    gpio_intr_disable();

    if (gpio_isr_func) {
        gpio_isr_func[gpio_num].fn = NULL;
        gpio_isr_func[gpio_num].args = NULL;
    }

    portEXIT_CRITICAL();
    return ESP_OK;
}

esp_err_t gpio_install_isr_service()
{
    GPIO_CHECK(gpio_isr_func == NULL, "GPIO isr service already installed", ESP_FAIL);
    esp_err_t ret;
    portENTER_CRITICAL();
    gpio_isr_func = (gpio_isr_func_t*) calloc(GPIO_NUM_MAX, sizeof(gpio_isr_func_t));

    if (gpio_isr_func == NULL) {
        ret = ESP_ERR_NO_MEM;
    } else {
        ret = gpio_isr_register(gpio_intr_service, NULL);
    }

    portEXIT_CRITICAL();
    return ret;
}

void gpio_uninstall_isr_service()
{
    if (gpio_isr_func == NULL) {
        return;
    }

    portENTER_CRITICAL();
    gpio_intr_disable();
    free(gpio_isr_func);
    gpio_isr_func = NULL;
    portEXIT_CRITICAL();
    return;
}

esp_err_t gpio_isr_register(void* fn, void* arg)
{
    portENTER_CRITICAL();
    gpio_intr_disable();
    _xt_isr_attach(ETS_GPIO_INUM, fn, arg);
    gpio_intr_enable();
    portEXIT_CRITICAL();
    return ESP_OK;
}

/*only level interrupt can be used for wake-up function*/
esp_err_t gpio_wakeup_enable(gpio_num_t gpio_num, gpio_int_type_t intr_type)
{
    GPIO_CHECK(GPIO_IS_VALID_GPIO(gpio_num), "GPIO number error", ESP_ERR_INVALID_ARG);
    esp_err_t ret = ESP_OK;

    if ((intr_type == GPIO_INTR_LOW_LEVEL) || (intr_type == GPIO_INTR_HIGH_LEVEL)) {
        SET_PERI_REG_MASK(GPIO_REG_ADDR(gpio_num), GPIO_PIN0_WAKEUP_ENABLE);
        gpio_set_intr_type(gpio_num, intr_type);
    } else {
        printf("[gpio] wakeup only support Level mode,but edge mode set. gpio_num:%u", gpio_num);
        ret = ESP_ERR_INVALID_ARG;
    }

    return ret;
}

esp_err_t gpio_wakeup_disable(gpio_num_t gpio_num)
{
    GPIO_CHECK(GPIO_IS_VALID_GPIO(gpio_num), "GPIO number error", ESP_ERR_INVALID_ARG);
    CLEAR_PERI_REG_MASK(GPIO_REG_ADDR(gpio_num), GPIO_PIN0_WAKEUP_ENABLE);
    return ESP_OK;
}
