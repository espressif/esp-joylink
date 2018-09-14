#ifndef __JD_AGENT_JSON_H_
#define __JD_AGENT_JSON_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */


#include "joylink.h"
/**
 * brief: 
 *
 * @Param: sdev
 * @Param: count
 *
 * @Returns: 
 */
char * 
joylink_agent_create_cloud_auth_json_req();

/**
 * brief: 
 *
 * @Returns: 
 */
char * 
joylink_agent_create_cloud_hb_json_req();

/**
 * brief: 
 *
 * @Returns: 
 */
char * 
joylink_agent_create_cloud_snap_json_req();

/**
 * brief: 
 *
 * @Param: pMsg
 * @Param: out_num
 *
 * @Returns: 
 */
JLAddAgentDev_t*
joylink_agent_parse_dev_del(const uint8_t* pMsg, int* out_num);

/**
 * brief: 
 *
 * @Param: pMsg
 * @Param: out_num
 *
 * @Returns: 
 */
JLAddAgentDev_t*
joylink_agent_parse_dev_add(const uint8_t* pMsg, int* out_num);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
