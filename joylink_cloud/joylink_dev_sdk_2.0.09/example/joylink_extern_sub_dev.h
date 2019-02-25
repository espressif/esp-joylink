#ifndef _JOYLINK_EXTERN_SUB_DEV_H_
#define _JOYLINK_EXTERN_SUB_DEV_H_

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "joylink.h"
#include "../joylink/joylink_sub_dev.h"

#define SCAN_QR_CODE  0
#define FIND_SUB_DEV  1

#define ONE_EVERY_TIME   0
#define MANY_EVERY_TIME  1
/*
 * user set
 */
#define ADD_SUBDEV_TYPE  FIND_SUB_DEV
#define ADD_SUBDEV_NUMB  ONE_EVERY_TIME

/*---------------- sub dev api ---------------*/

/**
 * brief: 
 *
 * @Param: dev
 * @Param: num
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_dev_sub_add(JLSubDevData_t *dev, int num);

/**
 * brief: 
 *
 * @Param: dev
 * @Param: num
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_sub_dev_del(char *feedid);

/**
 * brief: 
 *
 * @Param: feedid
 * @Param: dev
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_dev_sub_get_by_feedid(char *feedid, JLSubDevData_t *dev);
/**
 * brief: 
 *
 * @Param: feedid
 * @Param: dev
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_dev_sub_version_update(char *feedid, int version);
/**
 * brief: 
 *
 * @Param: uuid
 * @Param: mac
 * @Param: dev
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_sub_dev_get_by_uuid_mac(char *uuid, char *mac, JLSubDevData_t *dev);

/**
 * brief: 
 *
 * @Param: uuid
 * @Param: mac
 * @Param: dev
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_dev_sub_update_keys_by_uuid_mac(char *uuid, char *mac, JLSubDevData_t *dev);

/**
 * brief: 
 *
 * @Param: count
 * @Param: scan_type
 *
 * @Returns: 
 */
JLSubDevData_t *
joylink_dev_sub_devs_get(int *count);

/**
 * brief: 
 *
 * @Param: cmd
 * @Param: cmd_len
 * @Param: feedid
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_dev_sub_ctrl(const char* cmd, int cmd_len, char* feedid);

/**
 * brief: 
 *
 * @Param: feedid
 * @Param: out_len
 *
 * @Returns: 
 */
char *
joylink_dev_sub_get_snap_shot(char *feedid, int *out_len);

/**
 * brief: 
 *
 * @Param: feedid
 *
 * @Returns: 
 */
E_JLRetCode_t
joylink_dev_sub_unbind(const char *feedid);


#ifdef __cplusplus
}
#endif

#endif



