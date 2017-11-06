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

#include <string.h>
#include <esp_common.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "driver/uart.h"
#include "ringbuffer/simple_ringbuffer.h"
#include "driver/gpio.h"
#include "adapter/adapter.h"

#define UART_CHECK(a, str, ret_val) \
    if (!(a)) { \
        ets_printf("[uart] %s(%d): %s", __FUNCTION__, __LINE__, str); \
        return (ret_val); \
    }

#define UART_EMPTY_THRESH_DEFAULT       (20)
#define UART_FULL_THRESH_DEFAULT        (120)
#define UART_TOUT_THRESH_DEFAULT        (10)
#define UART_ENTER_CRITICAL_ISR()    portENTER_CRITICAL();
#define UART_EXIT_CRITICAL_ISR()     portEXIT_CRITICAL();
#define UART_ENTER_CRITICAL()        portENTER_CRITICAL();
#define UART_EXIT_CRITICAL()         portEXIT_CRITICAL()

typedef struct {
    uart_event_type_t type;        /*!< UART TX data type */
    struct {
        int brk_len;
        size_t size;
        uint8_t data[0];
    } tx_data;
} uart_tx_data_t;

typedef struct {
    uart_port_t uart_num;               /*!< UART port number*/
    int queue_size;                     /*!< UART event queue size*/
    QueueHandle_t xQueueUart;           /*!< UART queue handler*/
    //rx parameters
    int rx_buffered_len;                  /*!< UART cached data length */
    SemaphoreHandle_t rx_mux;           /*!< UART RX data mutex*/
    RingbufHandle_t rx_ring_buf;        /*!< RX ring buffer handler*/
    bool rx_buffer_full_flg;            /*!< RX ring buffer full flag. */
    uint8_t rx_data_buf[UART_FIFO_LEN]; /*!< Data buffer to stash FIFO data*/
    uint8_t rx_stash_len;               /*!< stashed data length.(When using flow control, after reading out FIFO data, if we fail to push to buffer, we can just stash them.) */
    //tx parameters
    SemaphoreHandle_t tx_fifo_sem;      /*!< UART TX FIFO semaphore*/
    SemaphoreHandle_t tx_mux;           /*!< UART TX mutex*/
    RingbufHandle_t tx_ring_buf;        /*!< TX ring buffer handler*/
    bool tx_waiting_fifo;               /*!< this flag indicates that some task is waiting for FIFO empty interrupt, used to send all data without any data buffer*/
    int tx_len_tot;                     /*!< Total length of current item in ring buffer*/
    int tx_len_cur;
} uart_obj_t;

static uart_obj_t* p_uart_obj[UART_NUM_MAX] = {0};

esp_err_t uart_set_word_length(uart_port_t uart_num, uart_word_length_t data_bit)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_FAIL);
    UART_CHECK((data_bit < UART_DATA_BITS_MAX), "data bit error", ESP_FAIL);
    UART_ENTER_CRITICAL();
    SET_PERI_REG_BITS(UART_CONF0(uart_num), UART_BIT_NUM, data_bit, UART_BIT_NUM_S);
    UART_EXIT_CRITICAL();
    return ESP_OK;
}

esp_err_t uart_get_word_length(uart_port_t uart_num, uart_word_length_t* data_bit)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_FAIL);
    *(data_bit) = GET_PERI_REG_BITS2(UART_CONF0(uart_num), UART_BIT_NUM, UART_BIT_NUM_S);
    return ESP_OK;
}

esp_err_t uart_set_stop_bits(uart_port_t uart_num, uart_stop_bits_t stop_bit)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_FAIL);
    UART_CHECK((stop_bit < UART_STOP_BITS_MAX), "stop bit error", ESP_FAIL);
    UART_ENTER_CRITICAL();
    SET_PERI_REG_BITS(UART_CONF0(uart_num), UART_STOP_BIT_NUM, stop_bit, UART_STOP_BIT_NUM_S);
    UART_EXIT_CRITICAL();
    return ESP_OK;
}

esp_err_t uart_get_stop_bits(uart_port_t uart_num, uart_stop_bits_t* stop_bit)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_FAIL);
    *(stop_bit) = GET_PERI_REG_BITS2(UART_CONF0(uart_num), UART_STOP_BIT_NUM, UART_STOP_BIT_NUM_S);
    return ESP_OK;
}

esp_err_t uart_set_parity(uart_port_t uart_num, uart_parity_t parity_mode)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_FAIL);
    UART_ENTER_CRITICAL();

    if (parity_mode & 0x1 > 0) {
        SET_PERI_REG_MASK(UART_CONF0(uart_num), UART_PARITY);
    } else {
        CLEAR_PERI_REG_MASK(UART_CONF0(uart_num), UART_PARITY);
    }

    if ((parity_mode >> 1) & 0x1 > 0) {
        SET_PERI_REG_MASK(UART_CONF0(uart_num), UART_PARITY_EN);
    } else {
        CLEAR_PERI_REG_MASK(UART_CONF0(uart_num), UART_PARITY_EN);
    }

    UART_EXIT_CRITICAL();
    return ESP_OK;
}

esp_err_t uart_get_parity(uart_port_t uart_num, uart_parity_t* parity_mode)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_FAIL);
    int val = READ_PERI_REG(UART_CONF0(uart_num));

    if (val & UART_PARITY_EN) {
        if (val & UART_PARITY) {
            (*parity_mode) = UART_PARITY_ODD;
        } else {
            (*parity_mode) = UART_PARITY_EVEN;
        }
    } else {
        (*parity_mode) = UART_PARITY_DISABLE;
    }

    return ESP_OK;
}

esp_err_t uart_set_baudrate(uart_port_t uart_num, uint32_t baud_rate)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_FAIL);
    UART_CHECK((baud_rate < UART_BITRATE_MAX), "baud_rate error", ESP_FAIL);
    uint32_t clk_div = (((UART_CLK_FREQ) << 4) / baud_rate);
    UART_ENTER_CRITICAL();
    SET_PERI_REG_BITS(UART_CLKDIV(uart_num), UART_CLKDIV_CNT, (clk_div >> 4), UART_CLKDIV_S);
    UART_EXIT_CRITICAL();
    return ESP_OK;
}

esp_err_t uart_get_baudrate(uart_port_t uart_num, uint32_t* baudrate)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_FAIL);
    UART_ENTER_CRITICAL();
    uint32_t clk_div = GET_PERI_REG_BITS2(UART_CLKDIV(uart_num), UART_CLKDIV_CNT, UART_CLKDIV_S);
    UART_EXIT_CRITICAL();
    (*baudrate) = (UART_CLK_FREQ) / clk_div;
    return ESP_OK;
}

esp_err_t uart_set_line_inverse(uart_port_t uart_num, uint32_t inverse_mask)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_FAIL);
    UART_CHECK((((inverse_mask & UART_LINE_INV_MASK) == 0) && (inverse_mask != 0)), "inverse_mask error", ESP_FAIL);
    UART_ENTER_CRITICAL();
    CLEAR_PERI_REG_MASK(UART_CONF0(uart_num), UART_LINE_INV_MASK);
    SET_PERI_REG_MASK(UART_CONF0(uart_num), inverse_mask);
    UART_EXIT_CRITICAL();
    return ESP_OK;
}

//only when UART_HW_FLOWCTRL_RTS is set , will the rx_thresh value be set.
esp_err_t uart_set_hw_flow_ctrl(uart_port_t uart_num, uart_hw_flowcontrol_t flow_ctrl, uint8_t rx_thresh)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_FAIL);
    UART_CHECK((rx_thresh < UART_FIFO_LEN), "rx flow thresh error", ESP_FAIL);
    UART_CHECK((flow_ctrl < UART_HW_FLOWCTRL_MAX), "hw_flowctrl mode error", ESP_FAIL);
    UART_ENTER_CRITICAL();

    if (flow_ctrl & UART_HW_FLOWCTRL_RTS) {
        SET_PERI_REG_BITS(UART_CONF1(uart_num), UART_RX_FLOW_THRHD, rx_thresh, UART_RX_FLOW_THRHD_S);
        SET_PERI_REG_MASK(UART_CONF1(uart_num), UART_RX_FLOW_EN);
    } else {
        CLEAR_PERI_REG_MASK(UART_CONF1(uart_num), UART_RX_FLOW_EN);
    }

    if (flow_ctrl & UART_HW_FLOWCTRL_CTS) {
        SET_PERI_REG_MASK(UART_CONF0(uart_num), UART_TX_FLOW_EN);
    } else {
        CLEAR_PERI_REG_MASK(UART_CONF0(uart_num), UART_TX_FLOW_EN);
    }

    UART_EXIT_CRITICAL();
    return ESP_OK;
}

esp_err_t uart_get_hw_flow_ctrl(uart_port_t uart_num, uart_hw_flowcontrol_t* flow_ctrl)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_FAIL);
    uart_hw_flowcontrol_t val = UART_HW_FLOWCTRL_DISABLE;

    if (READ_PERI_REG(UART_CONF1(uart_num)) & UART_RX_FLOW_EN) {
        val |= UART_HW_FLOWCTRL_RTS;
    }

    if (READ_PERI_REG(UART_CONF0(uart_num)) & UART_TX_FLOW_EN) {
        val |= UART_HW_FLOWCTRL_CTS;
    }

    (*flow_ctrl) = val;
    return ESP_OK;
}

static esp_err_t uart_reset_fifo(uart_port_t uart_num)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_FAIL);
    UART_ENTER_CRITICAL();
    SET_PERI_REG_MASK(UART_CONF0(uart_num), UART_RXFIFO_RST);
    CLEAR_PERI_REG_MASK(UART_CONF0(uart_num), UART_RXFIFO_RST);
    SET_PERI_REG_MASK(UART_CONF0(uart_num), UART_TXFIFO_RST);
    CLEAR_PERI_REG_MASK(UART_CONF0(uart_num), UART_TXFIFO_RST);
    UART_EXIT_CRITICAL();
    return ESP_OK;
}

esp_err_t uart_clear_intr_status(uart_port_t uart_num, uint32_t clr_mask)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_FAIL);
    //intr_clr register is write-only
    WRITE_PERI_REG(UART_INT_CLR(uart_num), clr_mask);
    return ESP_OK;
}

esp_err_t uart_enable_intr_mask(uart_port_t uart_num, uint32_t enable_mask)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_FAIL);
    UART_ENTER_CRITICAL();
    SET_PERI_REG_MASK(UART_INT_CLR(uart_num), enable_mask);
    SET_PERI_REG_MASK(UART_INT_ENA(uart_num), enable_mask);
    UART_EXIT_CRITICAL();
    return ESP_OK;
}

esp_err_t uart_disable_intr_mask(uart_port_t uart_num, uint32_t disable_mask)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_FAIL);
    UART_ENTER_CRITICAL();
    CLEAR_PERI_REG_MASK(UART_INT_ENA(uart_num), disable_mask);
    UART_EXIT_CRITICAL();
    return ESP_OK;
}

esp_err_t uart_enable_rx_intr(uart_port_t uart_num)
{
    return uart_enable_intr_mask(uart_num, UART_RXFIFO_FULL_INT_ENA | UART_RXFIFO_TOUT_INT_ENA);
}

esp_err_t uart_disable_rx_intr(uart_port_t uart_num)
{
    return uart_disable_intr_mask(uart_num, UART_RXFIFO_FULL_INT_ENA | UART_RXFIFO_TOUT_INT_ENA);
}

esp_err_t uart_disable_tx_intr(uart_port_t uart_num)
{
    return uart_disable_intr_mask(uart_num, UART_TXFIFO_EMPTY_INT_ENA);
}

esp_err_t uart_enable_tx_intr(uart_port_t uart_num, int enable, int thresh)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_FAIL);
    UART_CHECK((thresh < UART_FIFO_LEN), "empty intr threshold error", ESP_FAIL);
    UART_ENTER_CRITICAL();
    WRITE_PERI_REG(UART_INT_CLR(uart_num), UART_TXFIFO_EMPTY_INT_CLR);
    SET_PERI_REG_BITS(UART_CONF1(uart_num), UART_TXFIFO_EMPTY_THRHD, thresh, UART_TXFIFO_EMPTY_THRHD_S);

    if (enable & 0x1 > 0) {
        SET_PERI_REG_MASK(UART_INT_ENA(uart_num), UART_TXFIFO_EMPTY_INT_ENA);
    } else {
        CLEAR_PERI_REG_MASK(UART_INT_ENA(uart_num), UART_TXFIFO_EMPTY_INT_ENA);
    }

    UART_EXIT_CRITICAL();
    return ESP_OK;
}

esp_err_t uart_intr_enable()
{
    _xt_isr_unmask(1 << ETS_UART_INUM);
    return ESP_OK;
}

esp_err_t uart_intr_disable()
{
    _xt_isr_mask(1 << ETS_UART_INUM);
    return ESP_OK;
}

esp_err_t uart_isr_register(uart_port_t uart_num, void (*fn)(void*), void* arg)
{
    int ret;
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_FAIL);
    UART_ENTER_CRITICAL();
    _xt_isr_attach(ETS_UART_INUM, fn, arg);
    uart_intr_enable();
    UART_EXIT_CRITICAL();
    return ret;
}


esp_err_t uart_isr_free(uart_port_t uart_num)
{
    esp_err_t ret = ESP_OK;
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_FAIL);

    UART_EXIT_CRITICAL();
    return ret;
}

esp_err_t uart_set_rts(uart_port_t uart_num, int level)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_FAIL);
    UART_CHECK(((READ_PERI_REG(UART_CONF1(uart_num)) & UART_RX_FLOW_EN) == 0), "disable hw flowctrl before using sw control", ESP_FAIL);
    UART_ENTER_CRITICAL();

    if (level & 0x1 > 0) {
        SET_PERI_REG_MASK(UART_CONF0(uart_num), UART_SW_RTS);
    } else {
        CLEAR_PERI_REG_MASK(UART_CONF0(uart_num), UART_SW_RTS);
    }

    UART_ENTER_CRITICAL();
    return ESP_OK;
}

esp_err_t uart_set_dtr(uart_port_t uart_num, int level)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_FAIL);
    UART_ENTER_CRITICAL();

    if ((level & 0x1) > 0) {
        SET_PERI_REG_MASK(UART_CONF0(uart_num), UART_SW_DTR);
    } else {
        CLEAR_PERI_REG_MASK(UART_CONF0(uart_num), UART_SW_DTR);
    }

    UART_ENTER_CRITICAL();
    return ESP_OK;
}

esp_err_t uart_param_config(uart_port_t uart_num, const uart_config_t* uart_config)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_FAIL);
    UART_CHECK((uart_config), "param null", ESP_FAIL);
    uart_set_hw_flow_ctrl(uart_num, uart_config->flow_ctrl, uart_config->rx_flow_ctrl_thresh);
    uart_set_baudrate(uart_num, uart_config->baud_rate);
    WRITE_PERI_REG(UART_CONF0(uart_num), (
                       (uart_config->parity << UART_PARITY_S)
                       | (uart_config->stop_bits << UART_STOP_BIT_NUM_S)
                       | (uart_config->data_bits << UART_BIT_NUM_S)
                       | ((uart_config->flow_ctrl & UART_HW_FLOWCTRL_CTS) ? UART_TX_FLOW_EN : 0x0)));
    return ESP_OK;
}

esp_err_t uart_intr_config(uart_port_t uart_num, const uart_intr_config_t* intr_conf)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_FAIL);
    UART_CHECK((intr_conf), "param null", ESP_FAIL);
    UART_ENTER_CRITICAL();
    WRITE_PERI_REG(UART_INT_CLR(uart_num), UART_INTR_MASK);

    if (intr_conf->intr_enable_mask & UART_RXFIFO_TOUT_INT_ENA) {
        SET_PERI_REG_BITS(UART_CONF1(uart_num), UART_RX_TOUT_THRHD, intr_conf->rx_timeout_thresh, UART_RX_TOUT_THRHD_S);
        SET_PERI_REG_MASK(UART_CONF1(uart_num), UART_RX_TOUT_EN);
    } else {
        CLEAR_PERI_REG_MASK(UART_CONF1(uart_num), UART_RX_TOUT_EN);
    }

    if (intr_conf->intr_enable_mask & UART_RXFIFO_FULL_INT_ENA) {
        SET_PERI_REG_BITS(UART_CONF1(uart_num), UART_RXFIFO_FULL_THRHD, intr_conf->rxfifo_full_thresh, UART_RXFIFO_FULL_THRHD_S);
    }

    if (intr_conf->intr_enable_mask & UART_TXFIFO_EMPTY_INT_ENA) {
        SET_PERI_REG_BITS(UART_CONF1(uart_num), UART_TXFIFO_EMPTY_THRHD, intr_conf->txfifo_empty_intr_thresh, UART_TXFIFO_EMPTY_THRHD_S);
    }

    WRITE_PERI_REG(UART_INT_ENA(uart_num), intr_conf->intr_enable_mask);
    UART_EXIT_CRITICAL();
    return ESP_OK;
}

//internal isr handler for default driver code.
static void IRAM_ATTR uart_rx_intr_handler_default(void* param)
{
    uart_obj_t* p_uart = (uart_obj_t*) param;
    uint8_t uart_num = p_uart->uart_num;
    uint8_t buf_idx = 0;
    uint32_t uart_intr_status = READ_PERI_REG(UART_INT_ST(uart_num));

    int rx_fifo_len = 0;
    uart_event_t uart_event;
    portBASE_TYPE HPTaskAwoken = 0;

    while (uart_intr_status != 0x0) {
        buf_idx = 0;
        uart_event.type = UART_EVENT_MAX;

        if (uart_intr_status & UART_TXFIFO_EMPTY_INT_ST) {
            UART_ENTER_CRITICAL_ISR();
            CLEAR_PERI_REG_MASK(UART_INT_ENA(uart_num), UART_TXFIFO_EMPTY_INT_ENA);
            SET_PERI_REG_MASK(UART_INT_CLR(uart_num), UART_TXFIFO_EMPTY_INT_CLR);
            UART_EXIT_CRITICAL_ISR();

            //TX semaphore will only be used when tx_buf_size is zero.
            if (p_uart->tx_waiting_fifo == true && p_uart->tx_ring_buf == NULL) {
                p_uart->tx_waiting_fifo = false;
                xSemaphoreGiveFromISR(p_uart->tx_fifo_sem, &HPTaskAwoken);

                if (HPTaskAwoken == pdTRUE) {
                    portYIELD() ;
                }
            } else {
                //We don't use TX ring buffer, because the size is zero.
                if (p_uart->tx_ring_buf == NULL) {
                    continue;
                }

                int tx_fifo_rem = UART_FIFO_LEN - 1 - GET_PERI_REG_BITS2(UART_STATUS(uart_num), UART_TXFIFO_CNT, UART_TXFIFO_CNT_S);
                bool en_tx_flg = false;

                //We need to put a loop here, in case all the buffer items are very short.
                //That would cause a watch_dog reset because empty interrupt happens so often.
                //Although this is a loop in ISR, this loop will execute at most 128 turns.
                while (tx_fifo_rem) {
                    //get head and parse data length.
                    if (p_uart->tx_len_tot == 0) {
                        uart_tx_data_t tx_data_struct;
                        int buffered_size = ringbuffer_get_data_size(p_uart->tx_ring_buf);

                        if (buffered_size >= sizeof(tx_data_struct)) {
                            ringbuffer_read_from_isr(p_uart->tx_ring_buf, (void*) &tx_data_struct,
                                                     sizeof(tx_data_struct), &HPTaskAwoken);
                            p_uart->tx_len_tot = tx_data_struct.tx_data.size;

                            if (p_uart->tx_len_tot <= 0) {
                                break;
                            }
                        } else {
                            break;
                        }
                    }

                    if (p_uart->tx_len_cur == 0) {
                        p_uart->tx_len_cur = ringbuffer_get_data_size(p_uart->tx_ring_buf);

                        if (p_uart->tx_len_cur <= 0) {
                            break;
                        }
                    }

                    int send_len = p_uart->tx_len_cur > tx_fifo_rem ? tx_fifo_rem : p_uart->tx_len_cur;
                    send_len = p_uart->tx_len_tot > send_len ? send_len : p_uart->tx_len_tot;
                    uint8_t data_buf[UART_FIFO_LEN];
                    ringbuffer_read_from_isr(p_uart->tx_ring_buf, data_buf, send_len, &HPTaskAwoken);

                    for (buf_idx = 0; buf_idx < send_len; buf_idx++) {
                        WRITE_PERI_REG(UART_FIFO(uart_num), data_buf[buf_idx] & 0xff);
                    }

                    p_uart->tx_len_tot -= send_len;
                    p_uart->tx_len_cur -= send_len;
                    en_tx_flg = true;
                    break;
                }

                if (en_tx_flg) {
                    UART_ENTER_CRITICAL_ISR();
                    WRITE_PERI_REG(UART_INT_CLR(uart_num), UART_TXFIFO_EMPTY_INT_CLR);
                    SET_PERI_REG_MASK(UART_INT_ENA(uart_num), UART_TXFIFO_EMPTY_INT_ENA);
                    UART_EXIT_CRITICAL_ISR();
                }
            }
        } else if ((uart_intr_status & UART_RXFIFO_TOUT_INT_ST) || (uart_intr_status & UART_RXFIFO_FULL_INT_ST)) {
            if (p_uart->rx_buffer_full_flg == false) {
                //Get the buffer from the FIFO
                rx_fifo_len = GET_PERI_REG_BITS2(UART_STATUS(uart_num), UART_RXFIFO_CNT, UART_RXFIFO_CNT_S);
                p_uart->rx_stash_len = rx_fifo_len;

                //We have to read out all data in RX FIFO to clear the interrupt signal
                while (buf_idx < rx_fifo_len) {
                    p_uart->rx_data_buf[buf_idx++] = READ_PERI_REG(UART_FIFO(uart_num)) & 0xff;
                }

                //After Copying the Data From FIFO ,Clear intr_status
                UART_ENTER_CRITICAL_ISR();
                WRITE_PERI_REG(UART_INT_CLR(uart_num), UART_RXFIFO_TOUT_INT_CLR);
                WRITE_PERI_REG(UART_INT_CLR(uart_num), UART_RXFIFO_FULL_INT_CLR);
                UART_EXIT_CRITICAL_ISR();
                uart_event.type = UART_DATA;
                uart_event.size = rx_fifo_len;
                //If we fail to push data to ring buffer, we will have to stash the data, and send next time.
                //Mainly for applications that uses flow control or small ring buffer.
                int stash_len = ringbuffer_write_from_isr(p_uart->rx_ring_buf, p_uart->rx_data_buf, p_uart->rx_stash_len, &HPTaskAwoken);

                if (stash_len <= 0) {
                    UART_ENTER_CRITICAL_ISR();
                    CLEAR_PERI_REG_MASK(UART_INT_ENA(uart_num), UART_RXFIFO_FULL_INT_ENA);
                    CLEAR_PERI_REG_MASK(UART_INT_ENA(uart_num), UART_RXFIFO_TOUT_INT_ENA);
                    UART_EXIT_CRITICAL_ISR();
                    p_uart->rx_buffer_full_flg = true;
                    uart_event.type = UART_BUFFER_FULL;
                } else {
                    UART_ENTER_CRITICAL_ISR();
                    p_uart->rx_buffered_len += p_uart->rx_stash_len;
                    UART_EXIT_CRITICAL_ISR();
                    uart_event.type = UART_DATA;
                }

                if (HPTaskAwoken == pdTRUE) {
                    portYIELD() ;
                }
            } else {
                UART_ENTER_CRITICAL_ISR();
                CLEAR_PERI_REG_MASK(UART_INT_ENA(uart_num), UART_RXFIFO_FULL_INT_ENA);
                CLEAR_PERI_REG_MASK(UART_INT_ENA(uart_num), UART_RXFIFO_TOUT_INT_ENA);
                WRITE_PERI_REG(UART_INT_CLR(uart_num), UART_RXFIFO_FULL_INT_CLR | UART_RXFIFO_TOUT_INT_CLR);
                UART_EXIT_CRITICAL_ISR();
                uart_event.type = UART_BUFFER_FULL;
            }
        } else if (uart_intr_status & UART_RXFIFO_OVF_INT_ST) {
            UART_ENTER_CRITICAL_ISR();
            SET_PERI_REG_MASK(UART_CONF0(uart_num), UART_RXFIFO_RST);
            CLEAR_PERI_REG_MASK(UART_CONF0(uart_num), UART_RXFIFO_RST);
            WRITE_PERI_REG(UART_INT_CLR(uart_num), UART_RXFIFO_OVF_INT_CLR);
            UART_EXIT_CRITICAL_ISR();
            uart_event.type = UART_FIFO_OVF;
        } else if (uart_intr_status & UART_BRK_DET_INT_ST) {
            WRITE_PERI_REG(UART_INT_CLR(uart_num), UART_BRK_DET_INT_CLR);
            uart_event.type = UART_BREAK;
        } else if (uart_intr_status & UART_FRM_ERR_INT_ST) {
            WRITE_PERI_REG(UART_INT_CLR(uart_num), UART_FRM_ERR_INT_CLR);
            uart_event.type = UART_FRAME_ERR;
        } else if (uart_intr_status & UART_PARITY_ERR_INT_ST) {
            WRITE_PERI_REG(UART_INT_CLR(uart_num), UART_PARITY_ERR_INT_CLR);
            uart_event.type = UART_PARITY_ERR;
        } else {
            WRITE_PERI_REG(UART_INT_CLR(uart_num), uart_intr_status);
            uart_event.type = UART_EVENT_MAX;
        }

        if (uart_event.type != UART_EVENT_MAX && p_uart->xQueueUart) {
            xQueueSendFromISR(p_uart->xQueueUart, (void*)&uart_event, &HPTaskAwoken);

            if (HPTaskAwoken == pdTRUE) {
                portYIELD() ;
            }
        }

        uart_intr_status = READ_PERI_REG(UART_INT_ST(uart_num));
    }
}

/**************************************************************/
esp_err_t uart_wait_tx_done(uart_port_t uart_num, TickType_t ticks_to_wait)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_FAIL);
    UART_CHECK((p_uart_obj[uart_num]), "uart driver error", ESP_FAIL);

    return ESP_OK;
}

static esp_err_t uart_set_break(uart_port_t uart_num, bool break_en)
{
    UART_ENTER_CRITICAL();

    if (break_en) {
        SET_PERI_REG_MASK(UART_CONF0(uart_num), UART_TXD_BRK);
    } else {
        CLEAR_PERI_REG_MASK(UART_CONF0(uart_num), UART_TXD_BRK);
    }

    UART_EXIT_CRITICAL();
    return ESP_OK;
}

//Fill UART tx_fifo and return a number,
//This function by itself is not thread-safe, always call from within a muxed section.
static int uart_fill_fifo(uart_port_t uart_num, const char* buffer, uint32_t len)
{
    uint8_t i = 0;
    uint8_t tx_fifo_cnt = GET_PERI_REG_BITS2(UART_STATUS(uart_num), UART_TXFIFO_CNT, UART_TXFIFO_CNT_S);
    uint8_t tx_remain_fifo_cnt = (UART_FIFO_LEN - 1 - tx_fifo_cnt);
    uint8_t copy_cnt = (len >= tx_remain_fifo_cnt ? tx_remain_fifo_cnt : len);

    for (i = 0; i < copy_cnt; i++) {
        WRITE_PERI_REG(UART_FIFO(uart_num), buffer[i]);
    }

    return copy_cnt;
}

int uart_tx_chars(uart_port_t uart_num, const char* buffer, uint32_t len)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", (-1));
    UART_CHECK((p_uart_obj[uart_num]), "uart driver error", (-1));
    UART_CHECK(buffer, "buffer null", (-1));

    if (len == 0) {
        return 0;
    }

    xSemaphoreTake(p_uart_obj[uart_num]->tx_mux, (portTickType)portMAX_DELAY);
    int tx_len = uart_fill_fifo(uart_num, (const char*) buffer, len);
    xSemaphoreGive(p_uart_obj[uart_num]->tx_mux);
    return tx_len;
}

static int uart_tx_all(uart_port_t uart_num, const char* src, size_t size, bool brk_en, int brk_len)
{
    if (size == 0) {
        return 0;
    }

    size_t original_size = size;
    //lock for uart_tx
    xSemaphoreTake(p_uart_obj[uart_num]->tx_mux, (portTickType)portMAX_DELAY);

    if (p_uart_obj[uart_num]->tx_ring_buf) {
        int max_size = ringbuffer_get_free_size(p_uart_obj[uart_num]->tx_ring_buf);
        int offset = 0;
        uart_tx_data_t evt;
        evt.tx_data.size = size;
        evt.tx_data.brk_len = brk_len;

        if (brk_en) {
            evt.type = UART_DATA_BREAK;
        } else {
            evt.type = UART_DATA;
        }

        int stmp  = ringbuffer_write(p_uart_obj[uart_num]->tx_ring_buf, (void*) &evt, sizeof(uart_tx_data_t), portMAX_DELAY);

        while (size > 0) {
            int send_size = size > max_size / 2 ? max_size / 2 : size;
            stmp = ringbuffer_write(p_uart_obj[uart_num]->tx_ring_buf, (void*)(src + offset), send_size, portMAX_DELAY);
            size -= stmp;
            offset += stmp;
        }

        xSemaphoreGive(p_uart_obj[uart_num]->tx_mux);
        uart_enable_tx_intr(uart_num, 1, UART_EMPTY_THRESH_DEFAULT);
    } else {
        while (size) {
            //semaphore for tx_fifo available
            if (pdTRUE == xSemaphoreTake(p_uart_obj[uart_num]->tx_fifo_sem, (portTickType)portMAX_DELAY)) {
                size_t sent = uart_fill_fifo(uart_num, (char*) src, size);

                if (sent < size) {
                    p_uart_obj[uart_num]->tx_waiting_fifo = true;
                    uart_enable_tx_intr(uart_num, 1, UART_EMPTY_THRESH_DEFAULT);
                }

                size -= sent;
                src += sent;
            }
        }

//        if(brk_en) {
//            uart_set_break(uart_num, brk_len);
//            xSemaphoreTake(p_uart_obj[uart_num]->tx_brk_sem, (portTickType)portMAX_DELAY);
//        }
        xSemaphoreGive(p_uart_obj[uart_num]->tx_fifo_sem);
    }

    xSemaphoreGive(p_uart_obj[uart_num]->tx_mux);
    return original_size;
}

int uart_write_bytes(uart_port_t uart_num, const char* src, size_t size)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", (-1));
    UART_CHECK((p_uart_obj[uart_num] != NULL), "uart driver error", (-1));
    UART_CHECK(src, "buffer null", (-1));
    return uart_tx_all(uart_num, src, size, 0, 0);
}

int uart_read_bytes(uart_port_t uart_num, uint8_t* buf, uint32_t length, TickType_t ticks_to_wait)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", (-1));
    UART_CHECK((buf), "uart_num error", (-1));
    UART_CHECK((p_uart_obj[uart_num]), "uart driver error", (-1));
    uint8_t* data = NULL;
    size_t size;
    size_t copy_len = 0;
    int len_tmp;
    portTickType ticks_end = xTaskGetTickCount() + ticks_to_wait;

    if (xSemaphoreTake(p_uart_obj[uart_num]->rx_mux, (portTickType)ticks_to_wait) != pdTRUE) {
        return -1;
    }

    while (length) {
        ticks_to_wait = ticks_end - xTaskGetTickCount();

        if (ticks_to_wait < 0) {
            goto exit;
        }

        int len_tmp = ringbuffer_read(p_uart_obj[uart_num]->rx_ring_buf, buf + copy_len, length - copy_len,
                                      (portTickType) ticks_to_wait);

        if (len_tmp > 0) {
            copy_len += len_tmp;
            length -= len_tmp;
        } else {
            goto exit;
        }

        if (p_uart_obj[uart_num]->rx_buffer_full_flg) {
            ticks_to_wait = ticks_end - xTaskGetTickCount();

            if (ticks_to_wait < 0) {
                ticks_to_wait = 0;
            }

            int stash_len = ringbuffer_write(p_uart_obj[uart_num]->rx_ring_buf, p_uart_obj[uart_num]->rx_data_buf,
                                             p_uart_obj[uart_num]->rx_stash_len, ticks_to_wait);

            if (stash_len == p_uart_obj[uart_num]->rx_stash_len) {
                UART_ENTER_CRITICAL()
                ;
                p_uart_obj[uart_num]->rx_buffered_len += p_uart_obj[uart_num]->rx_stash_len;
                UART_EXIT_CRITICAL();
                p_uart_obj[uart_num]->rx_buffer_full_flg = false;
                uart_enable_rx_intr(p_uart_obj[uart_num]->uart_num);
            } else {

            }
        }
    }

exit:
    xSemaphoreGive(p_uart_obj[uart_num]->rx_mux);
    UART_ENTER_CRITICAL();
    p_uart_obj[uart_num]->rx_buffered_len -= copy_len;
    UART_EXIT_CRITICAL();
    return copy_len;
}

esp_err_t uart_get_buffered_data_len(uart_port_t uart_num, size_t* size)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_FAIL);
    UART_CHECK((p_uart_obj[uart_num]), "uart driver error", ESP_FAIL);
    *size = p_uart_obj[uart_num]->rx_buffered_len;
    return ESP_OK;
}

esp_err_t uart_flush(uart_port_t uart_num)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_FAIL);
    UART_CHECK((p_uart_obj[uart_num]), "uart driver error", ESP_FAIL);
    uart_obj_t* p_uart = p_uart_obj[uart_num];
    uint8_t* data;
    size_t size;

    //rx sem protect the ring buffer read related functions
    xSemaphoreTake(p_uart->rx_mux, (portTickType)portMAX_DELAY);
    uart_disable_rx_intr(p_uart_obj[uart_num]->uart_num);

    int data_len = ringbuffer_get_data_size(p_uart->rx_ring_buf);
    ringbuffer_return(p_uart->rx_ring_buf, data_len);
    uart_reset_fifo(uart_num);
    uart_enable_rx_intr(p_uart_obj[uart_num]->uart_num);
    xSemaphoreGive(p_uart->rx_mux);
    return ESP_OK;
}

esp_err_t uart_driver_install(uart_port_t uart_num, int rx_buffer_size, int tx_buffer_size, int queue_size, QueueHandle_t* uart_queue, int intr_alloc_flags)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_FAIL);
    UART_CHECK((rx_buffer_size > UART_FIFO_LEN), "uart rx buffer length error(>128)", ESP_FAIL);

    if (p_uart_obj[uart_num] == NULL) {
        p_uart_obj[uart_num] = (uart_obj_t*) calloc(1, sizeof(uart_obj_t));

        if (p_uart_obj[uart_num] == NULL) {
            ets_printf("[uart] UART driver malloc error");
            return ESP_FAIL;
        }

        p_uart_obj[uart_num]->uart_num = uart_num;
        p_uart_obj[uart_num]->tx_fifo_sem = xSemaphoreCreateBinary();
        xSemaphoreGive(p_uart_obj[uart_num]->tx_fifo_sem);
        p_uart_obj[uart_num]->tx_mux = xSemaphoreCreateMutex();
        p_uart_obj[uart_num]->rx_mux = xSemaphoreCreateMutex();
        p_uart_obj[uart_num]->queue_size = queue_size;
        p_uart_obj[uart_num]->tx_len_tot = 0;
        p_uart_obj[uart_num]->rx_buffered_len = 0;

        if (uart_queue) {
            p_uart_obj[uart_num]->xQueueUart = xQueueCreate(queue_size, sizeof(uart_event_t));
            *uart_queue = p_uart_obj[uart_num]->xQueueUart;
        } else {
            p_uart_obj[uart_num]->xQueueUart = NULL;
        }

        p_uart_obj[uart_num]->rx_buffer_full_flg = false;
        p_uart_obj[uart_num]->tx_waiting_fifo = false;
        p_uart_obj[uart_num]->rx_ring_buf = ringbuffer_create(rx_buffer_size);

        if (tx_buffer_size > 0) {
            p_uart_obj[uart_num]->tx_ring_buf = ringbuffer_create(tx_buffer_size);
        } else {
            p_uart_obj[uart_num]->tx_ring_buf = NULL;
        }
    } else {
        ets_printf("[uart] UART driver already installed");
        return ESP_FAIL;
    }

    uart_isr_register(uart_num, uart_rx_intr_handler_default, p_uart_obj[uart_num]);
    uart_intr_config_t uart_intr = {
        .intr_enable_mask = UART_RXFIFO_FULL_INT_ENA
        | UART_RXFIFO_TOUT_INT_ENA
        | UART_FRM_ERR_INT_ENA
        | UART_RXFIFO_OVF_INT_ENA
        | UART_BRK_DET_INT_ENA
        | UART_PARITY_ERR_INT_ENA,
        .rxfifo_full_thresh = UART_FULL_THRESH_DEFAULT,
        .rx_timeout_thresh = UART_TOUT_THRESH_DEFAULT,
        .txfifo_empty_intr_thresh = UART_EMPTY_THRESH_DEFAULT
    };
    uart_intr_config(uart_num, &uart_intr);
    return ESP_OK;
}

//Make sure no other tasks are still using UART before you call this function
esp_err_t uart_driver_delete(uart_port_t uart_num)
{
    UART_CHECK((uart_num < UART_NUM_MAX), "uart_num error", ESP_FAIL);

    if (p_uart_obj[uart_num] == NULL) {
        ets_printf("[uart] ALREADY NULL");
        return ESP_OK;
    }

    uart_intr_disable();
    uart_disable_rx_intr(uart_num);
    uart_disable_tx_intr(uart_num);

    if (p_uart_obj[uart_num]->tx_fifo_sem) {
        vSemaphoreDelete(p_uart_obj[uart_num]->tx_fifo_sem);
        p_uart_obj[uart_num]->tx_fifo_sem = NULL;
    }

    if (p_uart_obj[uart_num]->tx_mux) {
        vSemaphoreDelete(p_uart_obj[uart_num]->tx_mux);
        p_uart_obj[uart_num]->tx_mux = NULL;
    }

    if (p_uart_obj[uart_num]->rx_mux) {
        vSemaphoreDelete(p_uart_obj[uart_num]->rx_mux);
        p_uart_obj[uart_num]->rx_mux = NULL;
    }

    if (p_uart_obj[uart_num]->xQueueUart) {
        vQueueDelete(p_uart_obj[uart_num]->xQueueUart);
        p_uart_obj[uart_num]->xQueueUart = NULL;
    }

    if (p_uart_obj[uart_num]->rx_ring_buf) {
        ringbuffer_delete(p_uart_obj[uart_num]->rx_ring_buf);
        p_uart_obj[uart_num]->rx_ring_buf = NULL;
    }

    if (p_uart_obj[uart_num]->tx_ring_buf) {
        ringbuffer_delete(p_uart_obj[uart_num]->tx_ring_buf);
        p_uart_obj[uart_num]->tx_ring_buf = NULL;
    }

    free(p_uart_obj[uart_num]);
    p_uart_obj[uart_num] = NULL;
    return ESP_OK;
}

void uart_set_pin(uart_port_t uart_num, bool swap_en, bool cts_en, bool rts_en)
{
    if (uart_num == UART_NUM_0) {
        if (cts_en) {
            printf("set uart0 cts\n");
            PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_UART0_CTS);
        }

        if (rts_en) {
            printf("set uart0 rts\n");
            PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_UART0_RTS);
        }

        if (swap_en) {
            printf("set uart0 swap\n");
            system_uart_swap();
        }
    } else {
        printf("set uart1 pin\n");
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_UART1_TXD_BK);
    }
}

static void uart_tx_one_char(uart_port_t uart_num, uint8 TxChar)
{
    while (true) {
        uint32 fifo_cnt = READ_PERI_REG(UART_STATUS(uart_num)) & (UART_TXFIFO_CNT << UART_TXFIFO_CNT_S);

        if ((fifo_cnt >> UART_TXFIFO_CNT_S & UART_TXFIFO_CNT) < 126) {
            break;
        }
    }

    WRITE_PERI_REG(UART_FIFO(uart_num) , TxChar);
}

void uart0_write_byte(char c)
{
    if (c == '\n') {
        uart_tx_one_char(UART_NUM_0, '\r');
        uart_tx_one_char(UART_NUM_0, '\n');
    } else if (c == '\r') {
    } else {
        uart_tx_one_char(UART_NUM_0, c);
    }
}

void uart1_write_byte(char c)
{
    if (c == '\n') {
        uart_tx_one_char(UART_NUM_1, '\r');
        uart_tx_one_char(UART_NUM_1, '\n');
    } else if (c == '\r') {
    } else {
        uart_tx_one_char(UART_NUM_1, c);
    }
}

void uart_set_print_port(uart_port_t uart_no)
{
    if (uart_no == 1) {
        os_install_putc1(uart1_write_byte);
    } else {
        os_install_putc1(uart0_write_byte);
    }
}
