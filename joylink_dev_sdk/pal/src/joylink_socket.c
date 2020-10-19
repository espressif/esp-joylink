#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#ifdef __LINUX_PAL__
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#endif

// joylink platform layer header files
#include "joylink_stdio.h"
#include "joylink_string.h"
#include "joylink_memory.h"
#include "joylink_socket.h"


/** @defgroup group_platform_net_api_manage
 *  @{
 */

/**
 * @brief   根据主机名获取主机的资料
 *
 * @param[in] hostname: 主机明字符串指针
 * @return 0:success, -1:failed
 * @see None.
 * @note None.
 */
int32_t jl_platform_gethostbyname(char *hostname, char *ip_buff, uint32_t buff_len)
{
#ifdef __LINUX_PAL__
    struct hostent *pGethost = NULL;
	char **pptr = NULL;
	struct in_addr addrConv, inaddr;
	jl_platform_memset(&addrConv, 0, sizeof(struct in_addr));
	pGethost = gethostbyname(hostname);
	if(pGethost != NULL)
	{
		pptr = pGethost->h_addr_list;
		for(; *pptr != NULL; pptr++){
			addrConv.s_addr = *(uint32_t *)(*pptr);
			jl_platform_printf("[%s] ip address:[%s] [%lu]\r\n", hostname, inet_ntoa(addrConv), addrConv.s_addr);
			jl_platform_memcpy(&inaddr, &addrConv, sizeof(struct in_addr));
		}

       jl_platform_memset(ip_buff,  0, buff_len);
        jl_platform_strncpy(ip_buff, inet_ntoa(inaddr), buff_len);
         jl_platform_printf("[%s] ip address\r\n", ip_buff);
        return 0;
                
        }else{
            return -1;
        }
#else
    return -1;
#endif

}

/**
 * @brief   获取地址家族类型的值
 *
 * @param[in] domain: 	地址家族
 * @return.
 	对应的类型的值
 * @see None.
 * @note None.
 */
int32_t jl_platform_get_socket_proto_domain(JL_SOCK_PROTO_DOMAIN_T domain)
{
#ifdef __LINUX_PAL__
    int32_t ret_domain = -1;
    if (domain == JL_SOCK_PROTO_DOMAIN_AF_INET) {
        ret_domain = AF_INET;
    }
    else if (domain == JL_SOCK_PROTO_DOMAIN_AF_INET6) {
        ret_domain = AF_INET6;
    } else {};
    return ret_domain;
#else
    return -1;
#endif
}

/**
 * @brief   通过指定域名，协议，类型创建一个socket
 *
 * @param[in] domain: 	地址家族，通常使用AF_INET
 * @param[in] type: 	socket种类
 * @param[in] protocol: socket使用的协议
 * @return.
 	>=0: 为socket句柄，表示创建成功
	<0:  socket创建失败
 * @see None.
 * @note None.
 */
int32_t jl_platform_socket(JL_SOCK_PROTO_DOMAIN_T domain, JL_SOCK_PROTO_TYPE_T type, JL_SOCK_PROTO_PROTO_T protocol)
{
#ifdef __LINUX_PAL__
    int32_t in_domain = 2, in_type, in_protocol = 0;
    if (domain == JL_SOCK_PROTO_DOMAIN_AF_INET) {
        in_domain = AF_INET;
    }
    else if (domain == JL_SOCK_PROTO_DOMAIN_AF_INET6) {
        in_domain = AF_INET6;
    } else {};
    
    if (type == JL_SOCK_PROTO_TYPE_SOCK_STREAM) {
        in_type = SOCK_STREAM;
    }
    else if (type == JL_SOCK_PROTO_TYPE_SOCK_DGRAM) {
        in_type = SOCK_DGRAM;
    }
    else if (type == JL_SOCK_PROTO_TYPE_SOCK_RAW) {
        in_type = SOCK_RAW;
    } else {};
    
    if (protocol == JL_SOCK_PROTO_PROTO_IPPROTO_IP) {
        protocol = IPPROTO_IP;
    }
    else if (protocol == JL_SOCK_PROTO_PROTO_IPPROTO_TCP) {
        protocol = IPPROTO_TCP;
    }
    else if (protocol == JL_SOCK_PROTO_PROTO_IPPROTO_UDP) {
        protocol = IPPROTO_UDP;
    } else {};

    return socket(in_domain, in_type, in_protocol);
#else
    return -1;
#endif
}

/**
 * @brief   用于与服务器建立连接,发出连接请求,必须在参数中指定服务器的IP地址和端口号
 *
 * @param[in] s: 指向用Socket函数生成的socket句柄
 * @param[in] name: 指向服务器地址的指针
 * @param[in] namelen: 该地址的长度
 * @return None.
	当函数成功调用时返回0
 * @see None.
 * @note None.
 */
int32_t jl_platform_connect(int32_t s, const jl_sockaddr *name, uint32_t namelen)
{
#ifdef __LINUX_PAL__
    struct sockaddr addr;
    addr.sa_family = name->sa_family;
    jl_platform_memcpy(addr.sa_data, name->sa_data, sizeof(addr.sa_data));
    return connect(s, &addr, namelen);
#else
    return -1;
#endif
}

/**
 * @brief   描述符控制函数接口
 *
* @param[in] s: handle.
* @param[in] cmd: command.
 * @return None.
 	0:sucess
	other:faild
 * @see None.
 * @note None.
 */
int32_t jl_platform_fcntl(int32_t s, JL_FCNTL_CMD_T cmd)
{
    int32_t ret = 0;
#ifdef __LINUX_PAL__
    int32_t flags;
    if (cmd == JL_FCNTL_CMD_SETFL_O_NONBLOCK) {
        if ((flags = fcntl(s, F_GETFL)) == -1) {
            ret = -1;
        } else {
            if (fcntl(s, F_SETFL, flags | O_NONBLOCK) == -1) {
                ret = -1;
            } else {
                ret = 1;
            }
        }
    } else
    {
        ret = -1;
    }
#endif
    return ret;
}

/**
 * @brief   在指定的socket上发送指定缓冲区的指定长度, 阻塞时间不超过指定时长, 且指定长度若发送完需提前返回
 * @param   fd 	    : socket的句柄
 * @param   buf : 被发送的缓冲区起始地址
 * @param   len:     被发送的数据长度, 单位是字节(Byte)
 * @param   flags:   flag
 * @param   timeout_ms : 可能阻塞的最大时间长度, 单位是毫秒
 *
 * @retval  < 0 : 发送过程中出现错误或异常
 * @retval  0 : 在指定的'timeout_ms'时间间隔内, 没有任何数据被成功发送
 * @retval  (0, len] : 在指定的'timeout_ms'时间间隔内, 被成功发送的数据长度, 单位是字节(Byte)
 *
 * @note none
 */
int32_t jl_platform_send( int32_t fd, const void *buf, int32_t len, int32_t flags, uint32_t timeout_ms )
{
#ifdef __LINUX_PAL__
    return send(fd, buf, len, flags);
#else
    return -1;
#endif
}

/**
 * @brief   利用socket进行接受数据.
 *
 * @param[in] fd:     指向用socket函数生成的句柄
 * @param[in] buf:   接受数据的缓冲区(数组)的指针
 * @param[in] len:   缓冲区的大小
 * @param[in] flags:   flag
 * @param[in] timeout_ms:   可能阻塞的最大时间长度, 单位是毫秒
 * @return
	成功时返回收到的字节数.
	如果连接被中断则返回0
	失败时返回 SOCKET_ERROR
 * @see None.
 * @note None.
 */
int32_t jl_platform_recv( int32_t fd, void *buf, int32_t len, int32_t flags, uint32_t timeout_ms )
{
#ifdef __LINUX_PAL__
    return recv(fd, buf, len, flags);
#else
    return -1;
#endif
}

/**
 * @brief   可以用于调查一个或多个socket的状态
 *
 * @param[in] maxfdp1:  如果是再windows 下面，这个参数可以忽略，可以为NULL
 * @param[in] readset:  用于接受的SOCKET设备的指针
 * @param[in] writeset: 用于发送数据的SOCKET设备的指针
 * @param[in] exceptset:检查错误的状态
 * @param[in] timeout:  超时设定
 * @return
	返回大于0的值时,表示与条件相符的SOCKET数
	返回0表示超时
	失败时返回SOCKET_ERROR
 * @see None.
 * @note None.
 */
int32_t jl_platform_select(int32_t maxfdp1, jl_fd_set *readset, jl_fd_set *writeset, jl_fd_set *exceptset,  jl_timeval *timeout)
{
#ifdef __LINUX_PAL__
    struct timeval tmp_timeval;
    tmp_timeval.tv_usec = timeout->tv_usec;
    tmp_timeval.tv_sec = timeout->tv_sec;
    return select(maxfdp1, (fd_set *)readset, (fd_set *)writeset, (fd_set *)exceptset, &tmp_timeval);
#else
    return -1;
#endif
}

/**
 * @brief   销毁socket连接
 *
* @param[in] s: 销毁的socket句柄
 * @return None.
 * @see None.
 * @note None.
 */
int32_t jl_platform_close(int32_t s)
{
#ifdef __LINUX_PAL__
    return close(s);
#else
    return 0;
#endif
}

/**
 * @brief   利用Socket进行发送数据.
 *
 * @param[in] s: 指向用Socket函数生成的句柄
 * @param[in] data: 接受数据的缓冲区(数组)的指针
 * @param[in] size: 缓冲区的大小
 * @param[in] flags: 调用方式(MSG_DONTROUTE , MSG_OOB)
 * @param[in] to: 指向发送方SOCKET地址的指针
 * @param[in] tolen: 发送方SOCKET地址的大小
 * @return None.
	成功时返回已经发送的字节数.
	失败时返回SOCKET_ERROR
 * @see None.
 * @note None.
 */
int32_t jl_platform_sendto( int32_t fd, const void *buf, int32_t len, int32_t flags, const jl_sockaddr *to, uint32_t tolen, uint32_t timeout_ms )
{
#ifdef __LINUX_PAL__
    struct sockaddr addr;
    addr.sa_family = to->sa_family;
    jl_platform_memcpy(addr.sa_data, to->sa_data, sizeof(addr.sa_data));
    return sendto(fd, buf, len, flags, &addr, tolen);
#else
    return -1;
#endif
}

/**
 * @brief   从指定的UDP句柄接收指定长度数据到缓冲区, 阻塞时间不超过指定时长, 且指定长度若接收完需提前返回, 源地址保存在出参中
 *
 * @param   fd : socket的句柄
 * @param   buf :  存放被接收数据的缓冲区起始地址
 * @param   len : 接收并存放到缓冲区中数据的最大长度, 单位是字节(Byte)
 * @param   flags:   flag
 * @param   from :    存放源网络地址的结构体首地址
 * @param   from_len : 地址大小
 * @param   timeout_ms : 可能阻塞的最大时间长度, 单位是毫秒
 *
 * @retval  < 0 : 接收过程中出现错误或异常
 * @retval  0 : 在指定的'timeout_ms'时间间隔内, 没有任何数据被成功接收
 * @retval  (0, len] : 在指定的'timeout_ms'时间间隔内, 被成功接收的数据长度, 单位是字节(Byte)
 */
int32_t jl_platform_recvfrom( int32_t fd, void *buf, int32_t len, int32_t flags, jl_sockaddr *from, uint32_t *from_len, uint32_t timeout_ms )
{
#ifdef __LINUX_PAL__
    int32_t ret;
    struct sockaddr addr;
    ret = recvfrom(fd, buf, len, flags, &addr, (socklen_t *)from_len);
    from->sa_family = addr.sa_family;
    jl_platform_memcpy(from->sa_data, addr.sa_data, sizeof(addr.sa_data));
    return ret;
#else
    return -1;
#endif
}

/**
 * @brief   指定本地IP地址所使用的端口号时候使用
 *
 * @param[in] s: 指向用socket函数生成的句柄
 * @param[in] s: 指向socket地址的指针
 * @param[in] s: 指向用socket函数生成的句柄

 * @return
	当函数成功调用时返回0
	调用失败时返回 SOCKET_ERROR

 * @see None.
 * @note None.
 */
int32_t jl_platform_bind(int32_t s, const jl_sockaddr *my_addr, uint32_t addrlen)
{
#ifdef __LINUX_PAL__
    struct sockaddr addr;
    addr.sa_family = my_addr->sa_family;
    jl_platform_memcpy(addr.sa_data, my_addr->sa_data, sizeof(addr.sa_data));
    return bind(s, &addr, addrlen);
#else
    return -1;
#endif
}

/**
 * @brief   用来监听客户端的链接
 *
 * @param[in] s: 服务端套接字的句柄
 * @param[in] s: 套接字的未决连接队列的大小

 * @return
	当函数成功调用时返回0
	调用失败时返回 -1

 * @see None.
 * @note None.
 */
int32_t jl_platform_listen(int32_t s, int32_t backlog)
{
#ifdef __LINUX_PAL__
    return listen(s, backlog);
#else
    return -1;
#endif
}

/**
 * @brief   接受一个套接字中已经建立的连接
 *
 * @param[in] s: 服务端套接字的句柄
 * @param[in] s: 指向socket地址的指针
 * @param[in] s: 指向socket地址结构的大小

 * @return
	当函数成功调用时返回0
	调用失败时返回 -1

 * @see None.
 * @note None.
 */
int32_t jl_platform_accept(int32_t s, jl_sockaddr *my_addr, uint32_t *addrlen)
{
#ifdef __LINUX_PAL__
    int32_t ret;
    struct sockaddr addr;
    ret = accept(s, &addr, (socklen_t *)addrlen);
    my_addr->sa_family = addr.sa_family;
    jl_platform_memcpy(my_addr->sa_data, addr.sa_data, sizeof(addr.sa_data));
    return ret;
#else
    return -1;
#endif
}

/**
 * @brief    获取或者设置与某个套接字关联的选项。选项可能存在于多层协议中，它们总会出现在最上面的套接字层。当操作套接字选项时，
			 选项位于的层和选项的名称必须给出。为了操作套接字层的选项，应该将层的值指定为SOL_SOCKET。为了操作其它层的选项，控制选
			 项的合适协议号必须给出。例如，为了表示一个选项由TCP协议解析，层应该设定为协议号TCP。
 *
 * @param[in] socket: 将要被设置或者获取选项的套接字
 * @param[in] level: 将要被设置或者获取选项的套接字
 * @param[in] option_name: 需要访问的选项名
 * @param[in] option_value: 对于getsockopt()，指向返回选项值的缓冲。对于setsockopt()，指向包含新选项值的缓冲
 * @param[in] option_len: 对于getsockopt()，作为入口参数时，选项值的最大长度。作为出口参数时，选项值的实际长度。对于setsockopt()，现选项的长度
 * @return 
	成功执行时返回0
	失败返回-1
 * @see None.
 * @note None.
 */
int32_t jl_platform_setsockopt(int32_t socket, JL_SOCK_OPT_LEVEL_T level, JL_SOCK_OPT_NAME_T option_name, const void *option_value, int32_t option_len)
{
#ifdef __LINUX_PAL__
    int in_level = 1, in_option_name;
    if (level == JL_SOCK_OPT_LEVEL_SOL_SOCKET) {
        in_level = SOL_SOCKET;
    }
    if (option_name == JL_SOCK_OPT_NAME_SO_BROADCAST) {
        in_option_name = SO_BROADCAST;
    }
    if (option_name == JL_SOCK_OPT_NAME_SO_REUSEADDR) {
        in_option_name = SO_REUSEADDR;
    }
    if (option_name == JL_SOCK_OPT_NAME_SO_RCVTIMEO) {
        in_option_name = SO_RCVTIMEO;
    }
    return setsockopt(socket, in_level, in_option_name, option_value, option_len);
#else
    return -1;
#endif
}

/**
 * @brief    将32位主机字符顺序转换成网络字符顺序
 *
 * @param[in] hostlong: 32位主机字符值
 * @return 
 	返回对应的网络字符顺序
 * @see None.
 * @note None.
 */
uint32_t jl_platform_htonl(uint32_t hostlong)
{
#ifdef __LINUX_PAL__
    return htonl(hostlong);
#else
    return -1;
#endif
}

/**
 * @brief   将网络字符顺序转换为32位主机字符顺序
 *
* @param[in] netlong: 网络字符值
 * @return 
	返回对应的32位主机字符顺序
 * @see None.
 * @note None.
 */
uint32_t jl_platform_ntohl(uint32_t netlong)
{
#ifdef __LINUX_PAL__
    return ntohl(netlong);
#endif
    return 0;
}

/**
 * @brief   将16位主机字符顺序转换成网络字符顺序
 *
* @param[in] hostshort: 16位主机字符值
 * @return
	返回对应的网络字符顺序
 * @see None.
 * @note None.
 */
uint16_t jl_platform_htons(uint16_t hostshort)
{
#ifdef __LINUX_PAL__
    return htons(hostshort);
#else
    return -1;
#endif
}

/**
 * @brief   将16位网络字符顺序转换成主机字符顺序
 *
 * @param[in] netshort: 16位网络字符值
 * @return 
	返回对应的主机顺序
 * @see None.
 * @note None.
 */
uint16_t jl_platform_ntohs(uint16_t netshort)
{
#ifdef __LINUX_PAL__
    return ntohs(netshort);
#endif
}

/**
 * @brief   将网络地址转成网络二进制的数字
 *
* @param[in] addr: 网络地址
 * @return
	成功则返回非0值，失败则返回0
 * @see None.
 * @note None.
 */
int32_t jl_platform_inet_aton( const char *strptr, jl_in_addr *addr )
{
    #ifdef __LINUX_PAL__
    int32_t ret;
    struct in_addr inaddr;
    // inaddr.s_addr = addr->s_addr;
    ret = inet_aton(strptr, &inaddr);
    addr->s_addr = inaddr.s_addr;
    return ret;
    #else
     return -1;
    #endif
}

/**
 * @brief   将网络二进制的数字转换成网络地址
 *
* @param[in] addr: 网络地址
 * @return
	成功则返回字符串指针，失败则返回NULL
 * @see None.
 * @note None.
 */
char* jl_platform_inet_ntoa(jl_in_addr addr)
{
#ifdef __LINUX_PAL__
    struct in_addr tmp_addr;
    tmp_addr.s_addr = addr.s_addr;
    return inet_ntoa(tmp_addr);
#else
    return NULL;
#endif
}

/**
 * @brief   将一个十点分十进制的IP转换成一个长整数型数
 *
* @param[in] strptr: 网络地址
 * @return
	成功则返回in_addr_t，失败返回0
 * @see None.
 * @note None.
 */
jl_in_addr_t jl_platform_inet_addr(const char *strptr)
{
#ifdef __LINUX_PAL__
    return inet_addr(strptr);
#else
    jl_in_addr_t jl_addr = 0;
    return jl_addr;
#endif
}

jl_fd_set jl_platform_fd_set_allocate(void)
{
#if defined (__LINUX_PAL__)
    return (jl_fd_set)jl_platform_malloc( sizeof(fd_set) );
#else
    return NULL;
#endif
}

void jl_platform_fd_set_free(jl_fd_set fds)
{
#if defined (__LINUX_PAL__)
    if(fds)
        jl_platform_free(fds);
#endif
}

void JL_FD_CLR(int32_t fd, jl_fd_set set) 
{
#if defined (__LINUX_PAL__)
    FD_CLR(fd, (fd_set *)set);
#endif
}

int32_t JL_FD_ISSET(int32_t fd, jl_fd_set set) 
{
#if defined (__LINUX_PAL__)
    return FD_ISSET(fd, (fd_set *)set);
#else
    return 0;
#endif
}

void JL_FD_SET(int32_t fd, jl_fd_set set) 
{
#if defined (__LINUX_PAL__)
    FD_SET(fd, (fd_set *)set);
#endif
}

void JL_FD_ZERO(jl_fd_set set) 
{
#if defined (__LINUX_PAL__)
    FD_ZERO((fd_set *)set);
#endif
}

/** @} */ /* end of platform_net_api_manage */


