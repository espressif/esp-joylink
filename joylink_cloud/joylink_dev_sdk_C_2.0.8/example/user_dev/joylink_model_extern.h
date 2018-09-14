#ifndef _JOYLINK_EXTERN_H_
#define _JOYLINK_EXTERN_H_

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "joylink.h"

/*
 * user set
 */
#define JLP_VERSION  1
#define JLP_MAC "60:50:40:30:20:10"
/*
 * Create dev and get the index from developer center
 */

#define JLP_DEV_TYPE	E_JLDEV_TYPE_NORMAL
#define JLP_LAN_CTRL	E_LAN_CTRL_ENABLE
#define JLP_CMD_TYPE	E_CMD_TYPE_LUA_SCRIPT

#define JLP_UUID "87B580" 
#define IDT_CLOUD_PUB_KEY "0241A5170B0299294D39B0676D3D85BE732EE3EC664A0DCFA43C871A0D85192371"

#ifdef __cplusplus
}
#endif

#endif

