#ifndef _JOYLINK_DEV_H_
#define _JOYLINK_DEV_H_

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include "joylink.h"

#define LIGHT_CMD_NONE			(-1)
#define LIGHT_CMD_POWER			(1)

#define LIGHT_CTRL_NONE         (-1)
#define LIGHT_CTRL_ON           (1)
#define LIGHT_CTRL_OFF          (0)

typedef struct __light_ctrl{
    char cmd;
    int para_power;
    int para_state;
    int para_look;
    int para_move;
}LightCtrl_t;
typedef struct _light_manage_{
	int conn_st;	
    JLPInfo_t jlp;
    jl2_d_idt_t idt;
	LightCtrl_t lightCtrl;
}LightManage_t;

/**
 * @brief: 返回是否可以访问互联网
 *
 * @returns: E_RET_TRUE 可以访问, E_RET_FALSE 不可访问
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
 * @name:实现HTTPS的POST请求,请求响应填入revbuf参数 
 *
 * @param[in]: host POST请求的目标地址
 * @param[in]: query POST请求的路径、HEADER和Payload
 * @param[out]: revbuf 填入请求的响应信息
 * @param[in]: buflen  revbuf最大长度
 *
 * @returns:   
 *
 * @note:此函数必须正确实现,否则设备无法激活绑定
 * @note:小京鱼平台HTTPS使用的证书每年都会更新. 
 * @note:因为Joylink协议层面有双向认证的安全机制,所以此HTTPS请求设备无需校验server的证书. 
 * @note:如果设备必须校验server的证书,那么需要开发者实现时间同步等相关机制.
 */
int joylink_dev_https_post( char* host, char* query ,char *revbuf,int buflen);

/**
 * @brief 实现HTTP的POST请求,请求响应填入revbuf参数.
 * 
 * @param[in]  host: POST请求的目标主机
 * @param[in]  query: POST请求的路径、HEADER和Payload
 * @param[out] revbuf: 填入请求的响应信息的Body
 * @param[in]  buflen: revbuf的最大长度
 * 
 * @returns: 0 - 请求成功, -1 - 请求失败
 * 
 * @note: 此函数必须正确实现,否者设备无法校时,无法正常激活绑定
 *
 * */
int joylink_dev_http_post( char* host, char* query ,char *revbuf,int buflen);

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

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
