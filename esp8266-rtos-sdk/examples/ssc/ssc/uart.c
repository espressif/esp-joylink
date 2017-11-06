#include "esp_common.h"
#include "uart.h"
#include "ssc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

extern xQueueHandle xQueueSsc;

STATUS uart_tx_one_char(uint8 TxChar)
{
    while (true) {
        uint32 fifo_cnt = READ_PERI_REG(UART_STATUS(0)) & (UART_TXFIFO_CNT << UART_TXFIFO_CNT_S);

        if ((fifo_cnt >> UART_TXFIFO_CNT_S & UART_TXFIFO_CNT) < 126) {
            break;
        }
    }

    WRITE_PERI_REG(UART_FIFO(0) , TxChar);
    return OK;
}

LOCAL void uart_rx_intr_handler_ssc(void* arg)
{
    /* uart0 and uart1 intr combine togther, when interrupt occur, see reg 0x3ff20020, bit2, bit0 represents
      * uart1 and uart0 respectively
      */
    os_event_t e;
    portBASE_TYPE xHigherPriorityTaskWoken;

    uint8 RcvChar;
    uint8 uart_no = 0;

    if (UART_RXFIFO_FULL_INT_ST != (READ_PERI_REG(UART_INT_ST(uart_no)) & UART_RXFIFO_FULL_INT_ST)) {
        return;
    }

    RcvChar = READ_PERI_REG(UART_FIFO(uart_no)) & 0xFF;

    WRITE_PERI_REG(UART_INT_CLR(uart_no), UART_RXFIFO_FULL_INT_CLR);

    e.par = RcvChar;
    e.sig = SIG_SSC_UART_RX_CHAR;

    xQueueSendFromISR(xQueueSsc, (void *)&e, &xHigherPriorityTaskWoken);
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

LOCAL void uart_config(uint8 uart_no, UartDevice *uart)
{
    if (uart_no == UART1) {
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_U1TXD_BK);
    } else {
        /* rcv_buff size if 0x100 */
        _xt_isr_attach(ETS_UART_INUM, uart_rx_intr_handler_ssc, NULL);
        PIN_PULLUP_DIS(PERIPHS_IO_MUX_U0TXD_U);
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD);
//    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_U0RTS);
    }

    uart_div_modify(uart_no, UART_CLK_FREQ / (uart->baut_rate));

    WRITE_PERI_REG(UART_CONF0(uart_no), uart->exist_parity
                   | uart->parity
                   | (uart->stop_bits << UART_STOP_BIT_NUM_S)
                   | (uart->data_bits << UART_BIT_NUM_S));

    //clear rx and tx fifo,not ready
    SET_PERI_REG_MASK(UART_CONF0(uart_no), UART_RXFIFO_RST | UART_TXFIFO_RST);
    CLEAR_PERI_REG_MASK(UART_CONF0(uart_no), UART_RXFIFO_RST | UART_TXFIFO_RST);

    if (uart_no == UART0) {
        //set rx fifo trigger
        WRITE_PERI_REG(UART_CONF1(uart_no),
                       ((0x01 & UART_RXFIFO_FULL_THRHD) << UART_RXFIFO_FULL_THRHD_S));
    } else {
        WRITE_PERI_REG(UART_CONF1(uart_no),
                       ((0x01 & UART_RXFIFO_FULL_THRHD) << UART_RXFIFO_FULL_THRHD_S));
    }

    //clear all interrupt
    WRITE_PERI_REG(UART_INT_CLR(uart_no), 0xffff);
    //enable rx_interrupt
    SET_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_RXFIFO_FULL_INT_ENA);
}

void ssc_uart_init(UartBautRate bandrate)
{
    while (READ_PERI_REG(UART_STATUS(0)) & (UART_TXFIFO_CNT << UART_TXFIFO_CNT_S));
    while (READ_PERI_REG(UART_STATUS(1)) & (UART_TXFIFO_CNT << UART_TXFIFO_CNT_S));

    UartDevice uart;

	uart.baut_rate    = bandrate;
	uart.data_bits    = EIGHT_BITS;
	uart.flow_ctrl    = NONE_CTRL;
	uart.exist_parity = STICK_PARITY_DIS;
	uart.parity       = NONE_BITS;
	uart.stop_bits    = ONE_STOP_BIT;

	uart_config(UART0, &uart);
	uart_config(UART1, &uart);

	_xt_isr_unmask(1 << ETS_UART_INUM);
}

