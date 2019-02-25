#ifndef _JOYLINK_EXTERN_H_
#define _JOYLINK_EXTERN_H_

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "joylink.h"

#define JOYLINK_CLOUD_AUTH

#define JOYLINK_THUNDER_SLAVE
#define JOYLINK_SMART_CONFIG

/*
 * user set
 */
#define JLP_VERSION  1

/*
 * Create dev and get the index from developer center
 */
#define JLP_DEV_TYPE	E_JLDEV_TYPE_NORMAL
#define JLP_LAN_CTRL	E_LAN_CTRL_ENABLE
#define JLP_CMD_TYPE	E_CMD_TYPE_LUA_SCRIPT
#define JLP_SNAP_SHORT  E_SNAP_SHORT_N

#define JLP_UUID  "A8560A"
#define IDT_CLOUD_PUB_KEY  "0397B7B46C78B43E72591AAE516857A9B299C8F9291D68061FEC01B0A05DCB95A4"


#define USER_DATA_POWER   "Power"
#define USER_DATA_MODE   "Mode"

typedef struct _user_dev_status_t {
    int Power;
    int Mode;
} user_dev_status_t;


/**
 * brief:
 *
 * @Returns:
 */
int joylink_dev_get_user_mac(char *out);

/**
 * brief:
 *
 * @Returns:
 */
int joylink_dev_get_private_key(char *out);

/**
 * brief:
 *
 * @Returns:
 */
int joylink_dev_user_data_set(char *cmd, user_dev_status_t *user_data);



#ifdef __cplusplus
}
#endif

#endif

