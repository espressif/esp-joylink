#ifndef __JD_AGENT_H_
#define __JD_AGENT_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "joylink.h"
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>


#pragma pack(1)

/************** APP -> AGENT GW ************/
typedef struct {
	char feedid[JL_MAX_FEEDID_LEN];
	char accesskey[33];
}AgentDevAdd_t;

typedef struct {
	char feedid[JL_MAX_FEEDID_LEN];
}AgentDevDel_t;

/************** AGENT GW -> Cloud ************/
typedef struct {
	int devs_len;
	unsigned int timestamp;
	char* devs;
}AgentAuthReq_t;

typedef struct {
	unsigned int timestamp;
	int code;
	char error_devs[];
}AgentAuthRsp_t;

typedef struct {
	int devs_len;
	unsigned int timestamp;
	char* devs;
}AgentHBReq_t;

typedef struct {
	unsigned int timestamp;
	int code;
}AgentHBRsp_t;

typedef struct {
	int snaps_len;
	unsigned int timestamp;
	char* snaps;
}AgentSnapsReq_t;

typedef struct {
	unsigned int timestamp;
	int code;
}AgentSnapsRsp_t;

/**************  Cloud -> Agent GW ************/

#pragma pack()

typedef struct {
    int32_t fd;
    int32_t st;
	char accesskey[JL_MAX_ACKEY_LEN];
	char sessionkey[JL_MAX_SESSION_KEY_LEN];
	char feedid[JL_MAX_FEEDID_LEN];

    JLHearBeat_t hb;
    char *snap;
    int32_t snap_len;
    char auth[16];
    int32_t random;
    int agent_st;
}AgentDev_t;

#define JL_AGENT_GW_PORT                (33000)
#define JL_AGENT_GW_CLIENT_MAX          (20)

#define IS_AGENT_AUTH_SET(agent_st)     (agent_st & 0x01)
#define IS_AGENT_HB_SET(agent_st)       (agent_st & 0x02)
#define IS_AGENT_SNAP_SET(agent_st)     (agent_st & 0x04)

#define ST_AGENT_AUTH_SET(agent_st)     (agent_st = agent_st | 0x00000001)
#define ST_AGENT_HB_SET(agent_st)       (agent_st = agent_st | 0x02)
#define ST_AGENT_SNAP_SET(agent_st)     (agent_st = agent_st | 0x04)

#define ST_AGENT_AUTH_CLR(agent_st)     (agent_st = agent_st & (~0x01))
#define ST_AGENT_HB_CLR(agent_st)       (agent_st = agent_st & (~0x02))
#define ST_AGENT_SNAP_CLR(agent_st)     (agent_st = agent_st & (~0x04))

/********* funs ***********/

/**
 * brief: 
 *
 * @Param: fd
 * @Param: src
 * @Param: len
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_agent_cloud_auth_hand(int32_t fd, uint8_t *src, int32_t len);

/**
 * brief: 
 *
 * @Param: src
 * @Param: sin_recv
 * @Param: addrlen
 *
 * @Returns: 
 */
E_JLRetCode_t 
joylink_agent_proc_dev_add(uint8_t *src, struct sockaddr_in *sin_recv, socklen_t addrlen);

/**
 * brief: 
 *
 * @Param: src
 * @Param: sin_recv
 * @Param: addrlen
 *
 * @Returns: 
 */
E_JLRetCode_t 
joylink_agent_proc_dev_del(uint8_t *src, struct sockaddr_in *sin_recv, socklen_t addrlen);

/**
 * brief: 
 *
 * @Param: src
 * @Param: sin_recv
 * @Param: addrlen
 *
 * @Returns: 
 */
E_JLRetCode_t 
joylink_agent_proc_get_dev_list(uint8_t *src, struct sockaddr_in *sin_recv, socklen_t addrlen);

/**
 * brief: 
 *
 * @Param: fd
 * @Param: payload
 * @Param: s_len
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_agent_dev_cloud_ota_up_hand(int32_t fd, 
        uint8_t *payload, 
        int32_t s_len);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
