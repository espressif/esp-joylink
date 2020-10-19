#ifndef _JOYLINK_EXTERN_H_
#define _JOYLINK_EXTERN_H_

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "joylink.h"
#include "sdkconfig.h"

#define JOYLINK_CLOUD_AUTH
#define JOYLINK_DEVICE_AUTH

#define JOYLINK_THUNDER_SLAVE
#define JOYLINK_SMART_CONFIG

/*
 * user set
 */
#define JLP_VERSION  1
/*
 * Create dev and get the index from developer center
 */

#define JLP_DEV_TYPE    E_JLDEV_TYPE_NORMAL
#define JLP_LAN_CTRL	E_LAN_CTRL_ENABLE
#define JLP_CMD_TYPE	E_CMD_TYPE_LUA_SCRIPT
#define JLP_SNAPSHOT    E_SNAPSHOT_NO


#define JLP_UUID          CONFIG_JOYLINK_DEVICE_UUID
#define JLP_CLOUD_PUB_KEY CONFIG_JOYLINK_PUBLIC_KEY
#define JLP_PRIVATE_KEY   CONFIG_JOYLINK_PRIVATE_KEY

#define USER_DATA_POWER   "Power"
typedef struct _user_dev_status_t {
    int Power;
} user_dev_status_t;

/**
 * @brief: 设置设备RTC时间
 *
 * @returns: 
 * 
 * @note: This function has deprecated. Instead of using it you must implement the function jl_set_UTCtime which defined in pal/src/joylink_time.c
 */
E_JLBOOL_t joylink_dev_sync_clound_timestamp(long long timestamp);

/**
 * @brief: 此函数需返回设备的MAC地址
 *
 * @param[out] out: 将设备MAC地址赋值给此参数
 * 
 * @returns: E_RET_OK 成功, E_RET_ERROR 失败
 */
E_JLRetCode_t joylink_dev_get_user_mac(char *out);

/**
 * @brief: 此函数需返回设备私钥private key,该私钥可从小京鱼后台获取
 *
 * @param[out] out: 将私钥字符串赋值给该参数返回
 * 
 * @returns: E_RET_OK:成功, E_RET_ERROR:发生错误
 */
E_JLRetCode_t joylink_dev_get_private_key(char *out);

/**
 * @brief: 根据传入的cmd值,设置对应设备属性
 *
 * @param[in] cmd: 设备属性名称
 * @param[out] user_data: 设备状态结构体
 * 
 * @returns: 0:设置成功
 */
E_JLRetCode_t joylink_dev_user_data_set(char *cmd, user_dev_status_t *user_data);

/**
 * @brief: 传出激活信息
 *
 * @param[in] message: 激活信息
 * 
 * @returns: 0:设置成功
 */
E_JLRetCode_t joylink_dev_active_message(char *message);

/**
 * @brief: clear wifi information
 * 
 * @returns: void
 */
void esp_joylink_wifi_clear_info(void);

/**
 * brief: 设置局域网待激活模式
 *
 * @param[in] on_off: 1, 开启激活模式; 0, 关闭激活模式
 */

#ifdef __cplusplus
}
#endif

#endif

