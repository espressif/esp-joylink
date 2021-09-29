#ifndef _JOYLINK_EXTERN_SUB_DEV_H_
#define _JOYLINK_EXTERN_SUB_DEV_H_

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "joylink.h"
#include "joylink_sub_dev.h"

/*
 * user set
 */

#define DEV_AUTH_VALUE    0          // 0: disable; 1: enable.
#define DEV_BATCH_BIND    0          // 0: disable; 1; enable.

#define SUBDEV_UUID       "4E4638"
#define SUBDEV_MAC        "AA0011223366"
#define SUBDEV_LICENSE    "d91fa7fc6ba19811747d9d6ddc0971f2"

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
 * brief: 根据子设备mac查询子设备其他信息。
 *
 * @Param: macstr
 *
 * @Returns: 
 */
int joylink_sub_dev_get_by_deviceid(char *macstr, JLSubDevData_t *info_out);

/**
 * brief: 更新子设备信息，并根据标识进行保存。
 *
 * @Param: macstr
 *
 * @Returns: 
 */
int joylink_sub_dev_update_device_info(JLSubDevData_t *info_in, int save_flag);


/**
 * brief: 子设备上报数据或心跳时调用此函数，设置设备在线标识。
 *
 * @Param: macstr
 *
 * @Returns: 
 */

int joylink_sub_dev_hb_event(char *macstr);

/**
 * brief: 子设备上报设备快照，只上报macstr这个的快照。
 *
 * @Returns: 
 */
int joylink_server_sub_dev_event_req(char *macstr, char *event_payload, int length);

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

/**
 * brief: 上报子设备快照
 *
 * @Returns: 
 */
extern int joylink_server_subdev_event_req(char *macstr, char *event_payload, int length);

/**
 * brief: 立刻上报子设备快照
 *
 * @Returns: 
 */

int joylink_sub_dev_report_snapshot_immediately(char *macstr, char *data, int len);

#define SUBDEV_ACTIVE_STATUS_OPEN      0
#define SUBDEV_ACTIVE_STATUS_OK        1
#define SUBDEV_ACTIVE_STATUS_FAILED    2

/**
 * @brief: SDK subdev active 子设备激活状态报告
 * 
 * @param[in] status: 状态 0打开, 1成功，2失败, 3解邦
 * 
 * @return: reserved 当前此函数仅做通知,调用方不关心返回值.
 */
void joylink_sub_dev_active_status(char status);

/**
 * @brief: delete subdev 删除子设备
 * 
 * @param[in] mac: 删除子设备的 mac
 * 
 * @return: reserved 当前此函数仅做通知,调用方不关心返回值.
 */
void joylink_sub_dev_delete_msg(char *mac);

/**
 * @brief: 添加子设备到全局数组_g_sub_dev中
 *
 * @param[in]: dev 设备结构
 * @param[in]: message 例如：
 * {
* 	"uuid": "D23707",                                               // 小京鱼开放平台创建的虚拟红外被控设备的UUID
* 	"brandKey": "mm",                                           // 虚拟被控红外设备品牌标识，对接厂商自行定义
* 	"modelKey": "xx",                                              // 虚拟被控红外设备型号标识，对接厂商自行定义
* 	"controllerId": "948001623745658402"    // 主控红外遥控器的feedid，表示虚拟红外被控设备可以被哪一个遥控器控制。
* }
 *
 * @returns:  E_RET_OK 成功, E_RET_ERROR 发生错误
 */
E_JLRetCode_t joylink_dev_sub_add_infrared_receiver(JLSubDevData_t *dev, char *message);

#ifdef __cplusplus
}
#endif

#endif



