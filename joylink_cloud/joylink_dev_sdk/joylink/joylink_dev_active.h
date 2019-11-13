#ifndef _JOYLINK_DEV_ACTIVE_H_
#define _JOYLINK_DEV_ACTIVE_H_
#include "joylink.h"

#define LEN_URL_MAX			32
#define LEN_TOKEN_MAX		16

int joylink_dev_active_param_set(char *urlstr,char *tokenstr);


int joylink_dev_active_req(void);


#endif
