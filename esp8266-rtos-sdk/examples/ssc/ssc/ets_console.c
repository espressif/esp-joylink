/*
 *   Copyright (c) 2011 Espressif System
 *
 *     Simple Serial Console for ETS
 *
 */
#include "esp_common.h"

#include "uart.h"
#include "ssc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#define SSC_TASK_PRIORITY 10
#define SSC_QUEUE_SIZE 64

/*
 *   As ETS is not a acutal RR or preemptive kernel, for async cmd, we need to
 *    run a callback, in order to return prompt state. i.e. neet to call
 *    ssc_cmd_done(int cmd_id).
 */

typedef struct sscon_s {
    os_event_t eventq[SSC_EVT_N];
    char line[MAX_LINE_N];
    int8 line_end;
    uint8 run_id;
    uint8 arg_pos;
#define CMD_FAIL_TIMEOUT 2000
    os_timer_t command_fail_timer;
    uint8 cmdnum;
    ssc_cmd_t *cmdset;
} sscon_t;

#ifdef SSC_DEBUG
struct ssc_debug_line {
    char line[32];
    uint8 pos;
};
static struct ssc_debug_line dbgLine;
#endif /* SSC_DEBUG */

static sscon_t gCon;
#define CON()  (&gCon)
#define LEND() (CON()->line_end)

xTaskHandle xSscTaskHandle;
xQueueHandle xQueueSsc;

//static void ssc_task(os_event_t *e);
static char ssc_char_pop(void);
static int ssc_char_push(char *c);
static inline void ssc_flush_line(void);
static STATUS ssc_parse_line(void);
static void ssc_run_cmd(void);
static void ssc_finish_cmd(uint32 param);
#ifndef SSC_INT_DRIVE
static void ssc_getline(void);
#else
static void ssc_rx_char(char c);
#endif //SSC_INT_DRIVE
LOCAL void ssc_cmd_fail_handler();

static void (*_ssc_help)(void);

os_timer_t uart_debug_timer;

static void vSsc_task(void *pvParameters)
{
    os_event_t e;

    for (;;) {
        if (xQueueReceive(xQueueSsc, (void *)&e, (portTickType)portMAX_DELAY)) {
            switch (e.sig) {
                case SIG_SSC_RUNCMD:
                    ssc_run_cmd();
                    break;

                case SIG_SSC_CMDDONE:
                    ssc_finish_cmd(e.par);

                //fall thru
                case SIG_SSC_RESTART:
                    ssc_flush_line();
#ifndef SSC_INT_DRIVE
                    ssc_getline();
#else
                    CON()->run_id = 0xff;
                    ssc_printf("\n%s", PROMPT);
#endif //SSC_INT_DRIVE
                    break;
#ifdef SSC_INT_DRIVE

                case SIG_SSC_UART_RX_CHAR:
//                	printf("2 %c\n", e.par);
                    ssc_rx_char((char) e.par);
#ifdef SSC_DEBUG
                    dbgLine.line[dbgLine.pos] = (char)e.par;

                    if (dbgLine.pos++ >= 32) {
                        dbgLine.pos = 0;
                    }

#endif //SSC_DEBUG
                    break;
#endif //SSC_INT_DRIVE

                default:
                    //ASSERT(0);
                    break;
            }
        }
    }

    vTaskDelete(NULL);
}

static void ssc_run_cmd(void)
{
#define CON_IS_CMD_SYNC(cmd_id) (CON()->cmdset[(cmd_id)].flag & CMD_T_SYNC)
#define CON_IS_CMD_ASYNC(cmd_id) (CON()->cmdset[(cmd_id)].flag & CMD_T_ASYNC)

    os_event_t e;
    ssc_cmd_t *cmd = NULL;

    do {
        if (CON()->cmdnum == 0) {
            break;
        }

        if (CON()->cmdset == NULL) {
            break;
        }

        cmd = CON()->cmdset + CON()->run_id;

        if (cmd != NULL) {
            if (cmd->cmd_func) {
            	//switch to new line
            	ssc_printf("\n");
                cmd->cmd_func();

                if (cmd->flag & CMD_T_SYNC) {
//    		        os_post(SSC_PRIO, SIG_SSC_CMDDONE, cmd->id);
                    e.par = cmd->id;
                    e.sig = SIG_SSC_CMDDONE;
                    xQueueSend(xQueueSsc, (void *)&e, (portTickType)0);
                } else if (cmd->flag & CMD_T_ASYNC) {
                    /*
                     *  fire a timeout for async cmd
                     */
                    os_timer_disarm(&CON()->command_fail_timer);
                    os_timer_arm(&CON()->command_fail_timer, CMD_FAIL_TIMEOUT, 0);
                }
            }
        }

    } while (0);
}

static void ssc_finish_cmd(uint32 param)
{
    ssc_cmd_t *cmd = NULL;
    //we got only one run cmd
//    ASSERT((param & 0xffff) == CON()->run_id);
    os_timer_disarm(&CON()->command_fail_timer);

    do {
        if (CON()->cmdnum == 0) {
            break;
        }

        if (CON()->cmdset == NULL) {
            break;
        }

        cmd = CON()->cmdset + CON()->run_id;

        if (cmd == NULL) {
            break;
        }

        if (!(cmd->flag & CMD_T_ASYNC)) {
            break;
        }

        if (CON()->cmdset[CON()->run_id].cmd_callback) {
            CON()->cmdset[CON()->run_id].cmd_callback(&param);
        }

    } while (0);

#undef CON_IS_CMD_SYNC
#undef CON_IS_CMD_ASYNC
}

static char ssc_char_pop(void)
{
    char c;

    if (!LEND()) {
        return -1;
    }

    if ((--LEND()) >= 0) {
        c = CON()->line[LEND()];
        CON()->line[LEND()] = 0;
        return c;
    } else {
        return -1;
    }
}

static int ssc_char_push(char *c)
{
    if (LEND() == MAX_LINE_N) {
        return -1;
    }

    CON()->line[LEND()] = *c;
    return ++LEND();

}

static inline void ssc_flush_line(void)
{
    bzero(CON()->line, MAX_LINE_N);
    LEND() = 0;
    CON()->arg_pos = 0;
}

static STATUS ssc_parse_line(void)
{
	#define MAX_CMD_LEN  8
    char cmd_str[MAX_CMD_LEN];
    uint8 i = 0, j = 0;
    ssc_cmd_t *cmd = NULL;

    if (!LEND()) {
        return FAIL;
    }

    while( i < MAX_CMD_LEN && i < LEND()) {
        if(CON()->line[i] == ' ')
        	break;
    	cmd_str[i] = CON()->line[i];
    	i++;
    }

    for (j = 0; j < CON()->cmdnum; j++) {
        cmd = CON()->cmdset + j;

        if (cmd && i == strlen(cmd->cmd_str) && memcmp(cmd_str, cmd->cmd_str,i) == 0) {
            break;
        }
    }

    if (j == CON()->cmdnum) {
        return FAIL;
    }

    CON()->run_id = j;
    CON()->arg_pos = i + 1;

    return OK;
}

#ifndef SSC_INT_DRIVE
static void ssc_getline(void)
{
    char c;
    os_event_t e;
_restart:
    ssc_printf("\n%s", PROMPT);

    while (1) {
        if (FAIL == uart_rx_one_char(&c)) {
            continue;
        }

#ifdef SSC_DEBUG
        dbgLine.line[dbgLine.pos] = c;

        if (dbgLine.pos++ >= 32) {
            dbgLine.pos = 0;
        }

#endif //SSC_DEBUG

        if (c == '\r') {
            //minicom sent '\r'
            break;
        } else if (c == 0x08 || c == 0x7f) {
            //del key
            if (ssc_char_pop() != (char) - 1) {
                uart_tx_one_char(c);
                uart_tx_one_char(' ');
                uart_tx_one_char(c);
            } else {
                ssc_flush_line();
                goto _restart;
            }
        } else if (c == '?') {
            //help
            ssc_flush_line();
            _ssc_help();
            goto _restart;
        } else if (c >= 0x20 && c < 0x7f) {
            if (ssc_char_push(&c) == -1) {

            }

            ssc_printf("%c", c);
        } else {
            ssc_printf("what?\n");
        }
    }
    if (ssc_parse_line() == OK) {
        e.par = 0;
        e.sig = SIG_SSC_RUNCMD;
        xQueueSend(xQueueSsc, (void *)&e, (portTickType)0);
    } else {
        ssc_flush_line();
        goto _restart;
    }
}
#else
static void ssc_rx_char(char c)
{
    os_event_t e;

    do {
        if (CON()->run_id != 0xff) {
            //discard recieved char when we are in process of running some cmd
            return;
        }

        if (c == '?' && (LEND() == 0)) {
            _ssc_help();
            ssc_printf("\n%s", PROMPT);
            return;
        }

        if (c == '\r') {
            //minicom sent '\r'
            if (ssc_parse_line() == OK) {
                e.par = 0;
                e.sig = SIG_SSC_RUNCMD;
                xQueueSend(xQueueSsc, (void *)&e, (portTickType)0);
            } else {
                ssc_flush_line();
                ssc_printf("\n%s", PROMPT);
            }
        } else if (c == 0x08 || c == 0x7f) {
            //del key
            if (ssc_char_pop() != (char) - 1) {
                uart_tx_one_char(c);
                uart_tx_one_char(' ');
                uart_tx_one_char(c);
            } else {
                ssc_flush_line();
            }
        } else if (c >= 0x20 && c < 0x7f) {
            if (ssc_char_push(&c) == -1) {
                ssc_flush_line();
                ssc_printf("\n%s", PROMPT);
            } else {
                ssc_printf("%c", c);
            }
        } else {
            ssc_printf("what?\n");
        }
    } while (0);
}
#endif //SSC_INT_DRIVE

void ssc_attach(SscBaudRate bandrate)
{
    ssc_uart_init(bandrate);

    CON()->run_id = 0xff;

    xQueueSsc = xQueueCreate(SSC_QUEUE_SIZE, sizeof(os_event_t));

    xTaskCreate(vSsc_task, (uint8_t const *)"sscTask", 1024, NULL, SSC_TASK_PRIORITY, &xSscTaskHandle);
    os_timer_setfn(&CON()->command_fail_timer, ssc_cmd_fail_handler, NULL);

#ifndef SSC_INT_DRIVE
    ssc_getline();
#else
    printf("\n%s", PROMPT);
#endif //SSC_INT_DRIVE
}

void ssc_cmd_done(int cmd_id, STATUS status)
{
    os_event_t e;

    e.par = status << 16 | cmd_id;
    e.sig = SIG_SSC_CMDDONE;

    xQueueSend(xQueueSsc, (void *)&e, (portTickType)0);
}

void ssc_register(ssc_cmd_t *cmdset, uint8 cmdnum, void (*helper)(void))
{
    if (cmdnum) {
        CON()->cmdnum = cmdnum;
        CON()->cmdset = cmdset;
    }

    _ssc_help = helper;
}

int ssc_param_len(void)
{
    if (LEND() >= CON()->arg_pos) {
        return LEND() - CON()->arg_pos;
    }

    return 0;
}

char *ssc_param_str(void)
{
    return &CON()->line[CON()->arg_pos];
}

int ssc_parse_param(char *pLine, char *argv[])
{
    int nargs = 0;

    while (nargs < MAX_LINE_N) {
        while ((*pLine == ' ') || (*pLine == '\t')) {
            ++pLine;
        }

        if (*pLine == '\0') {
            argv[nargs] = NULL;
            return (nargs);
        }

        argv[nargs++] = pLine;

        while (*pLine && (*pLine != ' ') && (*pLine != '\t')) {
            ++pLine;
        }

        if (*pLine == '\0') {
            argv[nargs] = NULL;
            return (nargs);
        }

        *pLine++ = '\0';
    }

    return (nargs);
}

LOCAL void ssc_cmd_fail_handler(void *arg)
{
    os_event_t e;

    e.par = FAIL << 16 | CON()->run_id;
    e.sig = SIG_SSC_CMDDONE;

    xQueueSend(xQueueSsc, (void *)&e, (portTickType)0);
}
