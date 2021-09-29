#ifndef _JOYLINK_EXTERN_H_
#define _JOYLINK_EXTERN_H_

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "joylink.h"

/*
 * user set
 */
#define JLP_VERSION  0
/*
 * Create dev and get the index from developer center
 */

#define JLP_DEV_TYPE    E_JLDEV_TYPE_NORMAL
#define JLP_LAN_CTRL	E_LAN_CTRL_ENABLE
#define JLP_CMD_TYPE	E_CMD_TYPE_LUA_SCRIPT
#define JLP_SNAPSHOT    E_SNAPSHOT_NO

#ifdef JOYLINK_SDK_EXAMPLE_TEST
#define JLP_UUID "363282" 
#define IDT_CLOUD_PUB_KEY "0283866FBE020E880A0D004C7253A5FED07EBEEB618DB4149098F98ED8E5CEF244"

#define JLP_CHIP_MODEL_CODE ""

#define USER_DATA_POWER   "Power"
#define USER_DATA_MODE   "Mode"
#define USER_DATA_STATE   "State"

typedef struct _user_dev_status_t {
    int Power;
    int Mode;
    int State;
} user_dev_status_t;
#else
#define JLP_UUID "" 
#define IDT_CLOUD_PUB_KEY ""

#define JLP_CHIP_MODEL_CODE ""

typedef struct _user_dev_status_t {

} user_dev_status_t;
#endif


typedef enum _JL_OTA_UPGRADE_TYPE_E{
    JL_OTA_UPGRADE_TYPE_PROMPT      = 1,
    JL_OTA_UPGRADE_TYPE_SILENT      = 2,
    JL_OTA_UPGRADE_TYPE_ENFORCE     = 3
}JL_OTA_UPGRADE_TYPE_E;

typedef struct{
    int serial;
    char feedid[JL_MAX_FEEDID_LEN];
    char productuuid[JL_MAX_UUID_LEN];
    int version;
    char versionname[JL_MAX_VERSION_NAME_LEN];
    unsigned int crc32;
    char url[JL_MAX_URL_LEN];
    JL_OTA_UPGRADE_TYPE_E upgradetype;
}JLOtaOrder_t;

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
 * @brief: 返回是否可以访问互联网
 *
 * @returns: E_JL_TRUE 可以访问, E_JL_FALSE 不可访问
 */
E_JLBOOL_t joylink_dev_is_net_ok();

/**
 * @brief: 此函数用作通知应用层设备与云端的连接状态.
 *
 * @param: st - 当前连接状态  0-Socket init, 1-Authentication, 2-Heartbeat
 *
 * @returns: 
 */
E_JLRetCode_t joylink_dev_set_connect_st(int st);

/**
 * @brief: 存储JLP(Joylink Parameters)信息,将入参jlp结构中的信息持久化存储,如文件、设备flash等方式
 *
 * @param [in]: jlp-JLP structure pointer
 *
 * @returns: 
 */
E_JLRetCode_t joylink_dev_set_attr_jlp(JLPInfo_t *jlp);

/**
 * @brief: 从永久存储介质(文件或flash)中读取jlp信息,并赋值给参数jlp,其中feedid, accesskey,localkey,joylink_server,server_port必须正确赋值
 *
 * @param[out] jlp: 将JLP(Joylink Parameters)读入内存,并赋值给该参数
 *
 * @returns: E_RET_OK:成功, E_RET_ERROR:发生错误
 */
E_JLRetCode_t joylink_dev_get_jlp_info(JLPInfo_t *jlp);

/**
 * @brief: 返回包含model_code的json结构,该json结构的字符串形式由joylink_dev_modelcode_info（小京鱼后台自动生成代码,其中已经包含model_code）返回
 *
 * @param[out]: out_modelcode 用以返回序列化为字符串的model_code json结构
 * @param[in]: out_max json结构的最大允许长度
 *
 * @returns: 实际写入out_modelcode的长度
 */
int joylink_dev_get_modelcode(JLPInfo_t *jlp, char *out_modelcode, int32_t out_max);

/**
 * @brief: 获取设备快照json结构
 *
 * @param[out] out_snap: 序列化为字符串的设备快照json结构
 * @param[in] out_max: out_snap可写入的最大长度
 *
 * @returns: 实际写入out_snap的数据长度
 */
int joylink_dev_get_snap_shot(char *out_snap, int32_t out_max);

/**
 * @brief: 获取向App返回的设备快照json结构
 *
 * @param[out] out_snap: 序列化为字符串的设备快照json结构
 * @param[in] out_max: out_snap允许写入的最大长度
 * @param[in] code: 返回状态码
 * @param[in] feedid: 设备的feedid
 *
 * @returns: 
 */
int joylink_dev_get_json_snap_shot(char *out_snap, int32_t out_max, int code, char *feedid);

/**
 * @brief: 通过App控制设备,需要实现此函数,根据传入的json_cmd对设备进行控制
 *
 * @param[in] json_cmd: 设备控制命令
 *
 * @returns: E_RET_OK 控制成功, E_RET_ERROR 发生错误 
 */
E_JLRetCode_t joylink_dev_lan_json_ctrl(const char *json_cmd);

/**
 * @brief:根据src参数传入的控制命令数据包对设备进行控制.调用joylink_dev_parse_ctrl进行控制命令解析,并更改设备属性值
 *
 * @param[in] src: 控制指令数据包
 * @param[in] src_len: src长度
 * @param[in] ctr: 控制码
 * @param[in] from_server: 是否来自server控制 0-App,2-Server
 *
 * @returns: E_RET_OK 成功, E_RET_ERROR 失败
 */
E_JLRetCode_t joylink_dev_script_ctrl(const char *src, int src_len, JLContrl_t *ctr, int from_server);

/**
 * @brief: 实现接收到ota命令和相关参数后的动作,可使用otaOrder提供的参数进行具体的OTA操作
 *
 * @param[in] otaOrder: OTA命令结构体
 *
 * @returns: E_RET_OK 成功, E_RET_ERROR 发生错误
 */
E_JLRetCode_t joylink_dev_ota(JLOtaOrder_t *otaOrder);

/**
 * @brief: OTA执行状态上报,无需返回值
 */
void joylink_dev_ota_status_upload();


/**
 * @brief: 设置设备认证信息
 *
 * @param[out]: pidt--设备认证信息结构体指针,需填入必要信息sig,pub_key,f_sig,f_pub_key,cloud_pub_key
 *
 * @returns: 返回设置成功或失败
 */
E_JLRetCode_t joylink_dev_get_idt(jl2_d_idt_t *pidt);

/**
 * @brief: 用以返回一个整型随机数
 *
 * @param: 无
 *
 * @returns: 整型随机数
 */
int joylink_dev_get_random();

/**
 * @brief: SDK main loop 运行状态报告,正常情况下此函数每5秒会被调用一次,可以用来判断SDK主任务的运行状态.
 * 
 * @param[in] status: SDK main loop运行状态 0正常, -1异常
 * 
 * @return: 
 */
int joylink_dev_run_status(JLRunStatus_t status);

/**
 * @brief: 每间隔1个main loop周期此函数将在SDK main loop中被调用,让用户有机会将代码逻辑运行在核心任务中.
 * 
 * @note:  正常情况下一个main loop周期为1s(取决于socket等待接收数据的timeout时间),但不保证精度,请勿用作定时器
 * @note:  仅用作关键的非阻塞任务执行,例如OTA状态上报或设备状态上报.
 * @note:  执行阻塞或耗时较多的代码,将会妨碍主task运行.
 */
void joylink_dev_run_user_code();

/**
 * @brief: 传出激活信息
 *
 * @param[in] message: 激活信息
 * 
 * @returns: 0:设置成功
 */
E_JLRetCode_t joylink_dev_active_message(char *message);

/**
 * brief: 设置局域网待激活模式
 *
 * @param[in] on_off: 1, 开启激活模式; 0, 关闭激活模式
 */
void joylink_dev_lan_active_switch(uint8_t on_off);

/* 记录错误类型的统计，每种错误类型单独统计错误次数,最多纪录MAX_SIZE种错误类型的数据。*/
#define MAX_SIZE    10

typedef struct failedinfo{
	int 	times;	//错误次数
	int 	code;	//错误码
	char 	des[32];	//错误描述
}failedinfo_t;

/******************************接口用于模组连接ap失败 begin*********************************/
/**
 * @brief: 存储模组连接ap失败信息.
 *
 * @note: 在连接ap失败后调用，将错误码code与错误描述des写入flash保存。
 *
 * @param[in] code: 错误码
 *
 * @param[in] des: 错误描述
 *
 * @returns: 0 存储成功, -1 存储失败
 */
int joylink_set_fac_ap_con_failinfo(int code, char* des);

/**
 * @brief: 读取模组连接ap失败信息.
 *
 * @note: 上报错误日志前调用，读取flash中的错误信息。
 *
 * @param[out] info: 错误信息结构体指针
 *
 * @returns: 读取成功，返回size, 读取失败，返回-1.
 */
int joylink_get_fac_ap_con_failinfo(failedinfo_t* info);


/**
 * brief: 设置认证方式。
 *
 * @param[in] mode: 0, 单向认证; 1, 双向认证。   // 默认为双向认证。
 */
extern void joylink_set_device_auth_mode(uint8_t mode);

/**
 * @brief: 检测设备是否绑定，不能检测子设备是否绑定
 *
 *
 * @returns: 1, 已绑定；0, 未绑定；-1, 检测出错。
 */
int joylink_dev_check_bind_status(void);

#ifdef __cplusplus
}
#endif

#endif

