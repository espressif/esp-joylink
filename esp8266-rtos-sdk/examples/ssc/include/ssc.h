/*
 *   Copyright (c) 2011 Espressif System
 * 
 *     Simple Serial Console for ETS
 *       
 */

#ifndef _SSC_H_
#define _SSC_H_

typedef uint32_t ETSSignal;
typedef uint32_t ETSParam;

typedef struct ETSEventTag ETSEvent;

struct ETSEventTag {
    ETSSignal sig;
    ETSParam  par;
};

#define os_signal_t ETSSignal
#define os_param_t  ETSParam
#define os_event_t ETSEvent

#if 0
#define CMD_T_ASYNC   0x01
#define CMD_T_SYNC    0x02

typedef struct cmd_s {
    char * cmd_str;
    uint8   flag;
    uint8   id;
    void (* cmd_func)(void);
    void (* cmd_callback)(void *arg);
} ssc_cmd_t;
#endif

#define ssc_printf printf
#define MAX_LINE_N  127
#define PROMPT  ":>"
#define SSC_EVT_N   16//4
#define SSC_PRIO   27

enum {
    SIG_SSC_RUNCMD,
    SIG_SSC_CMDDONE,
    SIG_SSC_RESTART,
    SIG_SSC_UART_RX_CHAR,
};

#if 0
void ssc_attach(UartBautRate bandrate);
void ssc_cmd_done(int cmd_id, STATUS status);
int ssc_param_len(void);
char *ssc_param_str(void);
int ssc_parse_param(char *pLine, char *argv[]);
void ssc_register(ssc_cmd_t *cmdset, uint8 cmdnum, void (* help)(void));
#endif

#endif /* _SSC_H_ */
