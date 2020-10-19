/* --------------------------------------------------
 * @brief: 
 *
 * @version: 1.0
 *
 * @date: 08/01/2018
 * 
 * @desc: Functions in this file must be implemented by the device developers when porting the Joylink SDK.
 *
 * --------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "joylink.h"
#include "joylink_extern.h"
#include "joylink_extern_json.h"

#include "joylink_memory.h"
#include "joylink_socket.h"
#include "joylink_string.h"
#include "joylink_stdio.h"
#include "joylink_stdint.h"
#include "joylink_log.h"
#include "joylink_time.h"
#include "joylink_thread.h"
#include "joylink_extern_ota.h"


jl2_d_idt_t user_idt = 
{
	.type = 0,
	.cloud_pub_key = IDT_CLOUD_PUB_KEY,

	.sig = "01234567890123456789012345678901",
	.pub_key = "01234567890123456789012345678901",

	.f_sig = "01234567890123456789012345678901",
	.f_pub_key = "01234567890123456789012345678901",
};

#ifdef JOYLINK_SDK_EXAMPLE_TEST
user_dev_status_t user_dev = 
{
    .Power = 1,
    .Mode = 0,
    .State = 0,
};
#else
user_dev_status_t user_dev;
#endif

/*E_JLDEV_TYPE_GW*/
#ifdef _SAVE_FILE_
char  *file = "joylink_info.txt";
#endif


/**
 * @brief: 用以返回一个整型随机数
 *
 * @param: 无
 *
 * @returns: 整型随机数
 */
int
joylink_dev_get_random()
{
    /**
     *FIXME:must to do
     */
    static unsigned long int next = 1;
    next = next *1103515245 + 12345;
    return (int)(next/65536) % (1134);
}

/**
 * @brief: 返回是否可以访问互联网
 *
 * @returns: E_JL_TRUE 可以访问, E_JL_FALSE 不可访问
 */
E_JLBOOL_t joylink_dev_is_net_ok()
{
    /**
     *FIXME:must to do
     */
    return E_JL_TRUE;
}

/**
 * @brief: 此函数用作通知应用层设备与云端的连接状态.
 *
 * @param: st - 当前连接状态  0-Socket init, 1-Authentication, 2-Heartbeat
 *
 * @returns: 
 */
E_JLRetCode_t
joylink_dev_set_connect_st(int st)
{
	/**
	*FIXME:must to do
	*/
	char buff[64] = {0};
	int ret = 0;

	jl_platform_sprintf(buff, "{\"conn_status\":\"%d\"}", st);
	log_info("--set_connect_st:%s\n", buff);

	return ret;
}

/**
 * @brief: 传出激活信息
 *
 * @param[in] message: 激活信息
 * 
 * @returns: 0:设置成功
 */
E_JLRetCode_t joylink_dev_active_message(char *message)
{
	log_info("message = %s", message);

}

/**
 * @brief: 存储JLP(Joylink Parameters)信息,将入参jlp结构中的信息持久化存储,如文件、设备flash等方式
 *
 * @param [in]: jlp-JLP structure pointer
 *
 * @returns: 
 */
E_JLRetCode_t
joylink_dev_set_attr_jlp(JLPInfo_t *jlp)
{
	if(NULL == jlp){
		return E_RET_ERROR;
	}
	/**
	*FIXME:must to do
	*Must save jlp info to flash or files
	*/
	int ret = E_RET_ERROR;

#ifdef _SAVE_FILE_
    //Sample code for saving JLP info in a file.
	FILE *outfile;
	outfile = jl_platform_fopen(file, "wb+" );
	jl_platform_fwrite(jlp, sizeof(JLPInfo_t), 1, outfile );
	jl_platform_fclose(outfile);
#endif
	ret = E_RET_OK;

	return ret;
}

/**
 * @brief: 设置设备认证信息
 *
 * @param[out]: pidt--设备认证信息结构体指针,需填入必要信息sig,pub_key,f_sig,f_pub_key,cloud_pub_key
 *
 * @returns: 返回设置成功或失败
 */
E_JLRetCode_t
joylink_dev_get_idt(jl2_d_idt_t *pidt)
{
	if(NULL == pidt){
		return E_RET_ERROR; 
	}
	pidt->type = 0;
	/**
	*FIXME:must to do
	*/
	jl_platform_strcpy(pidt->sig, user_idt.sig);
	jl_platform_strcpy(pidt->pub_key, user_idt.pub_key);
	//jl_platform_strcpy(pidt->rand, user_idt.rand);
	jl_platform_strcpy(pidt->f_sig, user_idt.f_sig);
	jl_platform_strcpy(pidt->f_pub_key, user_idt.f_pub_key);
	jl_platform_strcpy(pidt->cloud_pub_key, user_idt.cloud_pub_key);

	return E_RET_OK;
}

/**
 * @brief: 此函数需返回设备的MAC地址
 *
 * @param[out] out: 将设备MAC地址赋值给此参数
 * 
 * @returns: E_RET_OK 成功, E_RET_ERROR 失败
 */
E_JLRetCode_t
joylink_dev_get_user_mac(char *out)
{
	/**
	*FIXME:must to do
	*/
#ifdef JOYLINK_SDK_EXAMPLE_TEST
	jl_platform_strcpy(out, "AABBCCDDEE01");
	return E_RET_OK;
#else
	return -1;
#endif
}

/**
 * @brief: 此函数需返回设备私钥private key,该私钥可从小京鱼后台获取
 *
 * @param[out] out: 将私钥字符串赋值给该参数返回
 * 
 * @returns: E_RET_OK:成功, E_RET_ERROR:发生错误
 */
E_JLRetCode_t
joylink_dev_get_private_key(char *out)
{
	/**
	*FIXME:must to do
	*/
#ifdef JOYLINK_SDK_EXAMPLE_TEST
	jl_platform_strcpy(out, "B12C1BC0212DA6BB74CBD5212B3CDE1B9FB1E45AD75279814CE8299957B75176");
	return E_RET_OK;
#else
	return E_RET_ERROR;
#endif
}

/**
 * @brief: 从永久存储介质(文件或flash)中读取jlp信息,并赋值给参数jlp,其中feedid, accesskey,localkey,joylink_server,server_port必须正确赋值
 *
 * @param[out] jlp: 将JLP(Joylink Parameters)读入内存,并赋值给该参数
 *
 * @returns: E_RET_OK:成功, E_RET_ERROR:发生错误
 */
E_JLRetCode_t
joylink_dev_get_jlp_info(JLPInfo_t *jlp)
{
	if(NULL == jlp){
		return E_RET_ERROR;
	}
	/**
	*FIXME:must to do
 	*Must get jlp info from flash 
	*/
	int ret = E_RET_OK;

#ifdef _SAVE_FILE_
JLPInfo_t fjlp;
jl_platform_memset(&fjlp, 0, sizeof(JLPInfo_t));

FILE *infile;
infile = jl_platform_fopen(file, "rb+");
if(infile > 0)
{
	jl_platform_fread(&fjlp, sizeof(fjlp), 1, infile);
	jl_platform_fclose(infile);
}

	jl_platform_strcpy(jlp->feedid, fjlp.feedid);
	jl_platform_strcpy(jlp->accesskey, fjlp.accesskey);
	jl_platform_strcpy(jlp->localkey, fjlp.localkey);
	jl_platform_strcpy(jlp->joylink_server, fjlp.joylink_server);
	jlp->server_port = fjlp.server_port;
	if(joylink_dev_get_user_mac(jlp->mac) < 0){
		jl_platform_strcpy(jlp->mac, fjlp.mac);
	}

	if(joylink_dev_get_private_key(jlp->prikey) < 0){
		jl_platform_strcpy(jlp->prikey, fjlp.prikey);
	}
#endif

	jlp->is_actived = E_JL_TRUE;
	jlp->model_code_flag = E_JL_FALSE;
	jlp->version = JLP_VERSION;
	jl_platform_strcpy(jlp->uuid, JLP_UUID);

	jlp->devtype = JLP_DEV_TYPE;
	jlp->lancon = JLP_LAN_CTRL;
	jlp->cmd_tran_type = JLP_CMD_TYPE;

	jlp->noSnapshot = JLP_SNAPSHOT;

	return ret;
}

/**
 * @brief: 返回包含model_code的json结构,该json结构的字符串形式由joylink_dev_modelcode_info（小京鱼后台自动生成代码,其中已经包含model_code）返回
 *
 * @param[out]: out_modelcode 用以返回序列化为字符串的model_code json结构
 * @param[in]: out_max json结构的最大允许长度
 *
 * @returns: 实际写入out_modelcode的长度
 */
int
joylink_dev_get_modelcode(JLPInfo_t *jlp, char *out_modelcode, int32_t out_max)
{
	if(NULL == out_modelcode || out_max < 0){
		return 0;
	}
	/**
	*FIXME:must to do
	*/
	int len = 0;

	char *packet_data =  joylink_dev_modelcode_info(0, jlp);
	if(NULL !=  packet_data){
		len = jl_platform_strlen(packet_data);
		log_info("------>%s:len:%d\n", packet_data, len);
		if(len < out_max){
			jl_platform_memcpy(out_modelcode, packet_data, len); 
		}else{
			len = 0;
		}
	}

	if(NULL !=  packet_data){
		jl_platform_free(packet_data);
	}

	return len;
}

/**
 * @brief: 返回设备状态,通过填充user_data参数,返回设备当前状态
 *
 * @param[out] user_data: 设备状态结构体指针
 * 
 * @returns: 0
 */
int
joylink_dev_user_data_get(user_dev_status_t *user_data)
{
	/**
	*FIXME:must to do
	*/
	return 0;
}

/**
 * @brief: 获取设备快照json结构,结构中包含返回状态码
 *
 * @param[in] ret_code: 返回状态码
 * @param[out] out_snap: 序列化为字符串的设备快照json结构
 * @param[in] out_max: out_snap可写入的最大长度
 *
 * @returns: 实际写入out_snap的数据长度
 */
int
joylink_dev_get_snap_shot_with_retcode(int32_t ret_code, char *out_snap, int32_t out_max)
{
	if(NULL == out_snap || out_max < 0){
		return 0;
	}
	/**
	*FIXME:must to do
	*/
	int len = 0;

	joylink_dev_user_data_get(&user_dev);

	char *packet_data =  joylink_dev_package_info(ret_code, &user_dev);
	if(NULL !=  packet_data){
		len = jl_platform_strlen(packet_data);
		log_info("------>%s:len:%d\n", packet_data, len);
		if(len < out_max){
			jl_platform_memcpy(out_snap, packet_data, len); 
		}else{
			len = 0;
		}
	}

	if(NULL !=  packet_data){
		jl_platform_free(packet_data);
	}
	return len;
}

/**
 * @brief: 获取设备快照json结构
 *
 * @param[out] out_snap: 序列化为字符串的设备快照json结构
 * @param[in] out_max: out_snap可写入的最大长度
 *
 * @returns: 实际写入out_snap的数据长度
 */
int
joylink_dev_get_snap_shot(char *out_snap, int32_t out_max)
{
	return joylink_dev_get_snap_shot_with_retcode(0, out_snap, out_max); 
}

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
int
joylink_dev_get_json_snap_shot(char *out_snap, int32_t out_max, int code, char *feedid)
{
    /**
     *FIXME:must to do
     */
    jl_platform_sprintf(out_snap, "{\"code\":%d, \"feedid\":\"%s\"}", code, feedid);

    return jl_platform_strlen(out_snap);
}

/**
 * @brief: 通过App控制设备,需要实现此函数,根据传入的json_cmd对设备进行控制
 *
 * @param[in] json_cmd: 设备控制命令
 *
 * @returns: E_RET_OK:控制成功, E_RET_ERROR:发生错误 
 */
E_JLRetCode_t 
joylink_dev_lan_json_ctrl(const char *json_cmd)
{
    /**
     *FIXME:must to do
     */
    log_debug("json ctrl:%s", json_cmd);

    return E_RET_OK;
}

/**
 * @brief: 根据传入的cmd值,设置对应设备属性
 *
 * @param[in] cmd: 设备属性名称
 * @param[out] user_data: 设备状态结构体
 * 
 * @returns: E_RET_OK 设置成功
 */
E_JLRetCode_t
joylink_dev_user_data_set(char *cmd, user_dev_status_t *user_data)
{
	/**
	*FIXME:must to do
	*/
	return E_RET_OK;
}

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

E_JLRetCode_t 
joylink_dev_script_ctrl(const char *src, int src_len, JLContrl_t *ctr, int from_server)
{
	if(NULL == src || NULL == ctr){
		return E_RET_ERROR;
	}
	/**
	*FIXME:must to do
	*/
	int ret = -1;
	ctr->biz_code = (int)(*((int *)(src + 4)));
	ctr->serial = (int)(*((int *)(src +8)));

	uint32_t tt = jl_time_get_timestamp(NULL);
	log_info("bcode:%d:server:%d:time:%ld", ctr->biz_code, from_server,(long)tt);

	if(ctr->biz_code == JL_BZCODE_GET_SNAPSHOT){
		/*
		*Nothing to do!
		*/
		ret = E_RET_OK;
	}else if(ctr->biz_code == JL_BZCODE_CTRL){
		joylink_dev_parse_ctrl(src + 12, &user_dev);
		return E_RET_OK;
	}else if(ctr->biz_code == JL_BZCODE_MENU){
		joylink_dev_parse_ctrl(src + 12, &user_dev);
		return E_RET_OK;
	}else{
		char buf[50];
		jl_platform_sprintf(buf, "Unknown biz_code:%d", ctr->biz_code);
		log_error("%s", buf);
	}
	return ret;
}

/**
 * @brief: 实现接收到ota命令和相关参数后的动作,可使用otaOrder提供的参数进行具体的OTA操作
 *
 * @param[in] otaOrder: OTA命令结构体
 *
 * @returns: E_RET_OK 成功, E_RET_ERROR 发生错误
 */
E_JLRetCode_t
joylink_dev_ota(JLOtaOrder_t *otaOrder)
{
	jl_thread_t task_id;

	if(NULL == otaOrder){
		return E_RET_ERROR;
	}
	joylink_set_ota_info(otaOrder);
	task_id.thread_task = (threadtask) joylink_ota_task;
	task_id.stackSize = 15*1024;
    task_id.priority = JL_THREAD_PRI_DEFAULT;
    task_id.parameter = (void *)otaOrder;
    jl_platform_thread_start(&task_id);

	return E_RET_OK;
}


/**
 * @brief: OTA执行状态上报,无需返回值
 */
void
joylink_dev_ota_status_upload()
{
	return;	
}

static int joylink_dev_http_parse_content(
	char *response, 
	int response_len, 
	char *content,
	int content_len)
{
	int length = 0;
	content[0] = 0;
    char* p = jl_platform_strstr(response, "\r\n\r\n");
    if (p == NULL) 
	{
		return -1;
	}
	p += 4;
	length = response_len - (p - response);
	length = length > content_len ? content_len - 1 : length;
	jl_platform_strncpy(content, p, length);
	content[length] = 0;
	// log_info("content = \r\n%s", content);
    
	return length;
}

/**
 * @name:实现HTTPS的POST请求,请求响应填入revbuf参数 
 *
 * @param[in]: host POST请求的目标地址
 * @param[in]: query POST请求的路径、HEADER和Payload
 * @param[out]: revbuf 填入请求的响应信息
 * @param[in]: buflen  revbuf最大长度
 *
 * @returns:   负数 - 发生错误, 非负数 - 实际填充revbuf的长度 
 *
 * @note:此函数必须正确实现,否则设备无法激活绑定
 * @note:小京鱼平台HTTPS使用的证书每年都会更新. 
 * @note:因为Joylink协议层面有双向认证的安全机制,所以此HTTPS请求设备无需校验server的证书. 
 * @note:如果设备必须校验server的证书,那么需要开发者实现时间同步等相关机制.
 */
int joylink_dev_https_post( char* host, char* query ,char *revbuf,int buflen)
{
	int ret = -1;
	/**
		 *FIXME:must to do
	*/
	return ret;
}

/**
 * @brief 实现HTTP的POST请求,请求响应填入revbuf参数.
 * 
 * @param[in]  host: POST请求的目标主机
 * @param[in]  query: POST请求的路径、HEADER和Payload
 * @param[out] revbuf: 填入请求的响应信息的Body
 * @param[in]  buflen: revbuf的最大长度
 * 
 * @returns: 负值 - 发生错误, 非负 - 实际填充revbuf的长度
 * 
 * @note: 此函数必须正确实现,否者设备无法校时,无法正常激活绑定
 *
 * */
int joylink_dev_http_post( char* host, char* query, char *revbuf, int buflen)
{
#ifdef __LINUX_PAL__
	int log_socket = -1;
	int len = 0;
    int ret = -1;
    char *recv_buf = NULL;
    jl_sockaddr_in saServer; 
	char ip[20] = {0};

    if(host == NULL || query == NULL || revbuf == NULL) {
        log_error("DNS lookup failed");
        goto RET;
    }

    jl_platform_memset(ip,0,sizeof(ip)); 
	ret = jl_platform_gethostbyname(host, ip, SOCKET_IP_ADDR_LEN);
	if(ret < 0){
		log_error("get ip error");
		ret = -1;
		goto RET;
	}
	
	jl_platform_memset(&saServer, 0, sizeof(saServer));
    saServer.sin_family = jl_platform_get_socket_proto_domain(JL_SOCK_PROTO_DOMAIN_AF_INET);
    saServer.sin_port = jl_platform_htons(80);
	saServer.sin_addr.s_addr = jl_platform_inet_addr(ip);

    log_socket = jl_platform_socket(JL_SOCK_PROTO_DOMAIN_AF_INET, JL_SOCK_PROTO_TYPE_SOCK_STREAM, JL_SOCK_PROTO_PROTO_IPPROTO_TCP);
    if(log_socket < 0) {
        log_error("... Failed to allocate socket.");
        goto RET;
    }
	int reuseaddrEnable = 1;
	if (jl_platform_setsockopt(log_socket, 
                JL_SOCK_OPT_LEVEL_SOL_SOCKET,
                JL_SOCK_OPT_NAME_SO_REUSEADDR, 
                (uint8_t *)&reuseaddrEnable, 
                sizeof(reuseaddrEnable)) < 0){
		log_error("set SO_REUSEADDR error");
	}
	
    /*fcntl(log_socket,F_SETFL,fcntl(log_socket,F_GETFL,0)|O_NONBLOCK);*/

    if(jl_platform_connect(log_socket, (jl_sockaddr *)&saServer, sizeof(saServer)) != 0)
	{
        log_error("... socket connect failed");
        goto RET;
    }

    if (jl_platform_send(log_socket, query, jl_platform_strlen(query), 0, 5000) < 0) {
        log_error("... socket send failed");
        goto RET;
    }

	int recv_buf_len = 1024; //2048;
	recv_buf = (char *)jl_platform_malloc(recv_buf_len);
	if(recv_buf == NULL)
	{
		goto RET;
	}
	jl_platform_memset(recv_buf, 0, recv_buf_len);
	len = jl_platform_recv(log_socket, recv_buf, recv_buf_len, 0, 5000);
	if(len <= 0)
	{
		ret = -1;
		goto RET;
	}
	log_info("... read data length = %d, response data = \r\n%s", len, recv_buf);
	ret = joylink_dev_http_parse_content(recv_buf, len, revbuf, buflen);


RET:
	if(-1 != log_socket){
		jl_platform_close(log_socket);
	}
	if (recv_buf)
	{
		/* code */
		jl_platform_free(recv_buf);
	}
	
	return ret;
#else
    return -1;
#endif

}

/**
 * @brief: SDK main loop 运行状态报告,正常情况下此函数每5秒会被调用一次,可以用来判断SDK主任务的运行状态.
 * 
 * @param[in] status: SDK main loop运行状态 0正常, -1异常
 * 
 * @return: reserved 当前此函数仅做通知,调用方不关心返回值.
 */

int joylink_dev_run_status(JLRunStatus_t status)
{
	int ret = -1;
	/**
		 *FIXME:must to do
	*/
	return ret;
}

/**
 * @brief: 每间隔1个main loop周期此函数将在SDK main loop中被调用,让用户有机会将代码逻辑运行在核心任务中.
 * 
 * @note:  正常情况下一个main loop周期为1s(取决于socket等待接收数据的timeout时间),但不保证精度,请勿用作定时器
 * @note:  仅用作关键的非阻塞任务执行,例如OTA状态上报或设备状态上报.
 * @note:  执行阻塞或耗时较多的代码,将会妨碍主task运行.
 */
void joylink_dev_run_user_code()
{
    //You may add some code run in the main loop if necessary.
}

