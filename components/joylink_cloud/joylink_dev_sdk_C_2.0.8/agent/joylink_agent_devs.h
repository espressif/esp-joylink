#ifndef __JD_AGENT_DEVS_H_
#define __JD_AGENT_DEVS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "joylink.h"
#include "joylink_agent.h"

#define AGENT_DEV_MAX               (10)

/**
 * brief: 
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_agent_devs_load(void);

/**
 * brief: 
 *
 * @Param: pad
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_agent_dev_add(char feedid[JL_MAX_FEEDID_LEN], 
        char ackey[JL_MAX_ACKEY_LEN]);

/**
 * brief: 
 *
 * @Param: feedid
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_agent_dev_del(char *feedid);

/**
 * brief: 
 *
 * @Param: pad
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_agent_dev_upd(char *feedid, AgentDev_t *pad);

/**
 * brief: 
 *
 * @Param: feedid
 * @Param: o_pad
 *
 * @Returns: 
 */
AgentDev_t *
joylink_agent_dev_get(char *feedid);

/**
 * brief: 
 *
 * @Param: fds[AGENT_DEV_MAX]
 *
 * @Returns: 
 */
int
joylink_agent_devs_get_fds(int fds[AGENT_DEV_MAX]);

/**
 * brief: 
 *
 * @Param: fd
 *
 * @Returns: 
 */
AgentDev_t *
joylink_agent_get_dev_by_fd(int fd);
/**
 * brief: 
 *
 * @Param: fd
 * @Param: feedid[JL_MAX_FEEDID_LEN]
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_agent_get_feedid_by_fd(int fd, char feedid[JL_MAX_FEEDID_LEN]);

/**
 * brief: 
 *
 * @Param: fd
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_agent_dev_clear_fd_by_fd(int fd);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
