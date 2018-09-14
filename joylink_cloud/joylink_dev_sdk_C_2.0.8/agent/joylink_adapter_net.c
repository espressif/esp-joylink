/*Copyright (c) 2015-2050, JD Smart All rights reserved.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License. */

#include <stdint.h>
#if defined(__MTK_7687__)
#include "lwip/netdb.h"
#include "lwip/inet.h"
#include "lwip/netif.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#include "lwip/sockets.h"
#elif defined(__REALTEK_8711__)
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/inet.h"
#include "lwip/netif.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#elif defined(__ESP32__)
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip/inet.h"
#include "lwip/netif.h"
#include "lwip/stats.h"
#include "lwip/tcp.h"
#elif defined(__MICOKIT_3166__)
#include "lwip/netdb.h"
#include "lwip/inet.h"
#include "sockets.h"
#include "lwip/arch.h"
#include "interface.h"

#elif defined(__MW300__)
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include <stdint.h>
#elif defined(__QC_4010__)
#include "basetypes.h"
#include "socket_api.h"
#include "timetype.h"
#include "qcom_time.h"
#elif defined(__OV_788__)
#include "includes.h" 
#include "libpdk.h"
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/time.h>
#include <time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#endif

#if  defined(__OV_788__)
#include "joylink_net.h"
#else
#include <string.h>

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#endif

#include "joylink_adapter_net.h"
#include "joylink_ret_code.h"
#include "joylink_log.h"
#include "joylink.h"

#ifndef __LINUX__
uint8_t s_host_ipv4[32] = {0};
#endif

#define E_OK            E_RET_OK
#define E_ERROR         E_RET_ERROR

static void 
joylink_adapter_net_set_error(char *err, const char *fmt, ...)
{
#if defined(__QC_4010__) || defined(__OV_788__)
    printf("\njoylink_net_set_error:not support va_list on qc4010.\n");
#else
    va_list ap;

    if (!err) return;
    va_start(ap, fmt);
    vsnprintf(err, ANET_ERR_LEN, fmt, ap);
    va_end(ap);
#endif
}

int32_t 
joylink_adapter_net_non_block(char *err, int32_t fd)
{
#if defined(__MICOKIT_3166__) || defined(__QC_4010__) || defined(__OV_788__)

	printf("not support non block now\n");
	return ANET_ERR;
#else
    int32_t flags;

    /* Set the socket non-blocking.
     * Note that fcntl(2) for F_GETFL and F_SETFL can't be
     * interrupted by a signal. */
#if defined (__MTK_7687__) || defined(__MW300__) || defined(__REALTEK_8711__) || defined(__ESP32__)
	if ((flags = fcntl(fd, F_GETFL, 0)) == -1)
#else
    if ((flags = fcntl(fd, F_GETFL)) == -1)
#endif	
	{
        joylink_adapter_net_set_error(err, "fcntl(F_GETFL): %s", strerror(errno));
        return ANET_ERR;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        joylink_adapter_net_set_error(err, "fcntl(F_SETFL,O_NONBLOCK): %s", strerror(errno));
        return ANET_ERR;
    }
    return ANET_OK;

#endif
}

/* Set TCP keep alive option to detect dead peers. The interval option
 * is only used for Linux as we are using Linux-specific APIs to set
 * the probe send time, interval, and count. */
int32_t 
joylink_adapter_net_keep_alive(char *err, int32_t fd, int32_t interval)
{
#if defined(__OV_788__)
	printf("not support non block now\n");
    return ANET_OK;
#else
    int32_t val = 1;

    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, sizeof(val)) == -1){
        joylink_adapter_net_set_error(err, "setsockopt SO_KEEPALIVE: %s", strerror(errno));
        return ANET_ERR;
    }

#ifdef __linux__
    /* Default settings are more or less garbage, with the keepalive time
     * set to 7200 by default on Linux. Modify settings to make the feature
     * actually useful. */

    /* Send first probe after interval. */
    val = interval;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &val, sizeof(val)) < 0) {
        joylink_adapter_net_set_error(err, "setsockopt TCP_KEEPIDLE: %s\n", strerror(errno));
        return ANET_ERR;
    }

    /* Send next probes after the specified interval. Note that we set the
     * delay as interval / 3, as we send three probes before detecting
     * an error (see the next setsockopt call). */
    val = interval/3;
    if (val == 0) val = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &val, sizeof(val)) < 0) {
        joylink_adapter_net_set_error(err, "setsockopt TCP_KEEPINTVL: %s\n", strerror(errno));
        return ANET_ERR;
    }

    /* Consider the socket in error state after three we send three ACK
     * probes without getting a reply. */
    val = 3;
    if (setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &val, sizeof(val)) < 0) {
        joylink_adapter_net_set_error(err, "setsockopt TCP_KEEPCNT: %s\n", strerror(errno));
        return ANET_ERR;
    }
#endif

    return ANET_OK;
#endif /* __OV_788__*/
}

/**
 * brief: 
 *
 * @Param: err
 * @Param: fd
 * @Param: val
 *
 * @Returns: 
 */
static int32_t 
joylink_adapter_net_set_tcp_no_delay(char *err, int32_t fd, int32_t val)
{
#if defined(__OV_788__)
    printf("no delay is not support in ov788.");
    return ANET_ERR;
#else
#if defined(__QC_4010__)
    if (setsockopt(fd, SOL_SOCKET, TCP_NODELAY, &val, sizeof(val)) == -1) {
#else
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val)) == -1) {
#endif
        joylink_adapter_net_set_error(err, "setsockopt TCP_NODELAY: %s", strerror(errno));
        return ANET_ERR;
    }
    return ANET_OK;
#endif /*ov_788*/
}

/**
 * brief: 
 *
 * @Param: err
 * @Param: fd
 *
 * @Returns: 
 */
int32_t 
joylink_adapter_net_enable_tcp_no_delay(char *err, int32_t fd)
{
    return joylink_adapter_net_set_tcp_no_delay(err, fd, 1);
}

/**
 * brief: 
 *
 * @Param: err
 * @Param: fd
 *
 * @Returns: 
 */
int32_t 
joylink_adapter_net_disable_tcp_no_delay(char *err, int32_t fd)
{
    return joylink_adapter_net_set_tcp_no_delay(err, fd, 0);
}

/**
 * brief: 
 *
 * @Param: err
 * @Param: fd
 * @Param: buffsize
 *
 * @Returns: 
 */
int32_t 
joylink_adapter_net_set_send_buf(char *err, int32_t fd, int32_t buffsize)
{
#if defined(__OV_788__)
    printf("not support:set_sent_buf\n");
    return ANET_ERR;
#else
    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &buffsize, sizeof(buffsize)) == -1)
    {
        joylink_adapter_net_set_error(err, "setsockopt SO_SNDBUF: %s", strerror(errno));
        return ANET_ERR;
    }
    return ANET_OK;
#endif /* ov_788 */
}

/**
 * brief: 
 *
 * @Param: err
 * @Param: fd
 *
 * @Returns: 
 */
int32_t
joylink_adapter_net_tcp_keep_alive(char *err, int32_t fd)
{
#if defined(__OV_788__)
    printf("not support:tcp_keep_alive\n");
    return ANET_ERR;
#else
    int32_t yes = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes)) == -1) {
        joylink_adapter_net_set_error(err, "setsockopt SO_KEEPALIVE: %s", strerror(errno));
        return ANET_ERR;
    }
    return ANET_OK;
#endif /* ov_788 */
}

/* joylink_adapter_net_generic_resolve() is called by joylink_adapter_net_resolve() and joylink_adapter_net_resolve_ip() to
 * do the actual work. It resolves the hostname "host" and set the string
 * representation of the IP address into the buffer pointed by "ipbuf".
 *
 * If flags is set to ANET_IP_ONLY the function only resolves hostnames
 * that are actually already IPv4 or IPv6 addresses. This turns the function
 * into a validating / normalizing function. */
int32_t
joylink_adapter_net_generic_resolve(char *err, char *host, char *ipbuf, int32_t ipbuf_len,
                       int32_t flags)
{
#if defined(__QC_4010__) || defined(__OV_788__)
    printf("__QC_4010__:struct addrinfo not support\n");
    return ANET_ERR;
#else
    struct addrinfo hints, *info;
    int32_t rv;

    memset(&hints,0,sizeof(hints));
    
    #if defined(__MICOKIT_3166__) || defined(__MW300__) || defined(__QC_4010__) || defined(__REALTEK_8711__)
    printf("__MICOKIT_3166__:ANET IP ONLY not support\n");
    #else
    if (flags & ANET_IP_ONLY) hints.ai_flags = AI_NUMERICHOST;
    #endif
	hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;  /* specify socktype to avoid dups */

    if ((rv = getaddrinfo(host, NULL, &hints, &info)) != 0) {
		joylink_adapter_net_set_error(err, "getaddrinfo:%d", rv);
        return ANET_ERR;
    }
    if (info->ai_family == AF_INET) {
        struct sockaddr_in *sa = (struct sockaddr_in *)info->ai_addr;
#if defined(__MW300__)
        inet_ntop(AF_INET, (ip_addr_t *) &(sa->sin_addr), ipbuf, ipbuf_len);
#else
        #if defined(__REALTEK_8711__)
        extern char * joylink_adapter_inet_ntoa(struct in_addr in_a);
        if (ipbuf) {
            char *temp_ip = joylink_adapter_inet_ntoa(sa->sin_addr);
            memcpy(ipbuf,  temp_ip, strlen(temp_ip));
        }
        #else
            inet_ntop(AF_INET, &sa->sin_addr, ipbuf, ipbuf_len);
        #endif
#endif
    } 
	else 
	{
		#ifndef __LINUX__
		printf("__MICOKIT_3166__:not support ipv6\n");
		#else
		struct sockaddr_in6 *sa = (struct sockaddr_in6 *)info->ai_addr;
        inet_ntop(AF_INET6, &(sa->sin6_addr), ipbuf, ipbuf_len);
		#endif
    }

    freeaddrinfo(info);
    return ANET_OK;
#endif
}

/**
 * brief: 
 *
 * @Param: err
 * @Param: host
 * @Param: ipbuf
 * @Param: ipbuf_len
 *
 * @Returns: 
 */
int32_t
joylink_adapter_net_resolve(char *err, char *host, char *ipbuf, int32_t ipbuf_len) {
    return joylink_adapter_net_generic_resolve(err,host,ipbuf,ipbuf_len,ANET_NONE);
}

/**
 * brief: 
 *
 * @Param: err
 * @Param: host
 * @Param: ipbuf
 * @Param: ipbuf_len
 *
 * @Returns: 
 */
int32_t
joylink_adapter_net_resolve_ip(char *err, char *host, char *ipbuf, int32_t ipbuf_len) {
    return joylink_adapter_net_generic_resolve(err,host,ipbuf,ipbuf_len,ANET_IP_ONLY);
}

/**
 * brief: 
 *
 * @Param: err
 * @Param: fd
 *
 * @Returns: 
 */
static int32_t 
joylink_adapter_net_set_reuse_addr(char *err, int32_t fd) {
#if defined(__OV_788__)
    printf("net_set_reuse_addr is not support\n"); 
        return ANET_ERR;
#else
    int32_t yes = 1;
    /* Make sure connection-intensive things like the redis benckmark
     * will be able to close/open sockets a zillion of times */
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        joylink_adapter_net_set_error(err, "setsockopt SO_REUSEADDR: %s", strerror(errno));
        return ANET_ERR;
    }
    return ANET_OK;
#endif /*  ov_788 */
}

/**
 * brief: 
 *
 * @Param: s
 *
 * @Returns: 
 */
static int32_t
joylink_adapter_net_check_connect(int32_t s)
{
#if defined(__OV_788__)
    return 0;
#else
    fd_set r_set;
    struct timeval tm;

    FD_ZERO(&r_set);
    FD_SET(s, &r_set);

    tm.tv_sec = 3;
    tm.tv_usec = 0;

    if (select(s + 1, NULL, &r_set, NULL, &tm) < 0) {
        log_error("select error");
        close(s);
        return -1;
    }

    if (FD_ISSET(s, &r_set)) {
        int32_t err = -1;
        socklen_t len = sizeof(int32_t);
        if (getsockopt(s, SOL_SOCKET, SO_ERROR ,&err, &len) < 0 ) {
            log_error("getsockopt error");
            close(s);
            return -2;
        }
        if (err) {
            log_error("err:%d, errno:%d", err, errno);
            errno = err;    //ENETUNREACH(101):Network is unreachable
            close(s);
            return -3;
        }
    }
    return 0;
#endif
}

/**
 *FIXME zhongxuan
 */
#define ANET_CONNECT_NONE 0
#define ANET_CONNECT_NONBLOCK 1

#if defined(__MTK_7687__)
static int32_t 
joylink_adapter_net_tcp_generic_connect(char *err, const char *addr, int32_t port, int32_t flags)
{
    int32_t s = ANET_ERR;   
    
    /* Try to create the socket and to connect it.
     * If we fail in the socket() call, or on connect(), we retry with
     * the next entry in servinfo. */
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1){
            log_error("socket:%d", s);
            goto error;
        }
    if (joylink_adapter_net_set_reuse_addr(err,s) == ANET_ERR) {
        log_error("socket:%d", s);
        goto error;
    }
    if (flags & ANET_CONNECT_NONBLOCK && joylink_adapter_net_non_block(err,s) != ANET_OK) {
        log_error("ANET_CONNECT_NONBLOCK:%d", s);
        goto error;
     }

    struct sockaddr_in in;
    memset(&in, 0, sizeof(in));
    in.sin_family = AF_INET;

    in.sin_port = htons(port);
    in.sin_addr.s_addr = joylink_adapter_inet_addr(addr);
    
    if (connect(s, (struct sockaddr*)&in, sizeof(struct sockaddr_in)) == -1) {
        /* If the socket is non-blocking, it is ok for connect() to
         * return an EINPROGRESS error here. */
        if (errno == EINPROGRESS && flags & ANET_CONNECT_NONBLOCK) {            
            if (0 == joylink_adapter_net_check_connect(s)){
                log_error("connect:%d", s);
                goto end;    
            }
            
        }
        close(s);
        s = ANET_ERR; 

        joylink_adapter_net_set_error(err, "connect: %s", strerror(errno));
        log_error("socket:%d, errno:%d, %s", s, errno, err);
    }    

    /* If we ended an iteration of the for loop without errors, we
     * have a connected socket. Let's return to the caller. */
    goto end;   
    
error:
    if (s != ANET_ERR) {
        close(s);
        s = ANET_ERR;
    }
end:    
    return s;
}
#elif defined(__OV_788__)
static int32_t 
joylink_adapter_net_tcp_generic_connect(char *err, const char *addr, int32_t port, int32_t flags)
{
    int32_t s = ANET_ERR;   
    
    if ((s = tsock_init(TSOCK_TCP, 0, 0)) == -1){
            log_error("socket:%d", s);
            goto error;
    }

    struct sockaddr_in in;
    memset(&in, 0, sizeof(in));
    in.sin_family = AF_INET;

    in.sin_port = htons(port);
    in.sin_addr.s_addr = joylink_adapter_inet_addr(addr);
    
    if (tsock_connect(s, (uip_ipaddr_t*)&(in.sin_addr), in.sin_port) == -1) {
        if (0 == joylink_adapter_net_check_connect(s)){
            log_error("connect:%d", s);
            goto end;    
        }
            
        tsock_close(s);
        s = ANET_ERR; 

        log_error("socket:%d", s);
    }    

    /* If we ended an iteration of the for loop without errors, we
     * have a connected socket. Let's return to the caller. */
    goto end;   
    
error:
    if (s != ANET_ERR) {
        tsock_close(s);
        s = ANET_ERR;
    }
end:    
    return s;
}
#else
static int32_t 
joylink_adapter_net_tcp_generic_connect(char *err, const char *addr, int32_t port, int32_t flags)
{
#if defined(__QC_4010__)
    printf("__QC_4010__:struct addrinfo not support\n");
    return ANET_ERR;
#else
    int32_t s = ANET_ERR, rv;
    char portstr[6];  /* strlen("65535") + 1; */
    struct addrinfo hints, *servinfo, *p;

    snprintf(portstr,sizeof(portstr),"%d",port);
    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(addr,portstr,&hints,&servinfo)) != 0) {
	    joylink_adapter_net_set_error(err, "getaddrinfo:%d", rv);
        return ANET_ERR;
    }
    for (p = servinfo; p != NULL; p = p->ai_next) {
        /* Try to create the socket and to connect it.
         * If we fail in the socket() call, or on connect(), we retry with
         * the next entry in servinfo. */
        if ((s = socket(p->ai_family,p->ai_socktype,p->ai_protocol)) == -1)
            continue;
        if (joylink_adapter_net_set_reuse_addr(err,s) == ANET_ERR) goto error;
        if (flags & ANET_CONNECT_NONBLOCK && joylink_adapter_net_non_block(err,s) != ANET_OK)
            goto error;
        if (connect(s, p->ai_addr, p->ai_addrlen) == -1) {
            /* If the socket is non-blocking, it is ok for connect() to
             * return an EINPROGRESS error here. */
            if (errno == EINPROGRESS && flags & ANET_CONNECT_NONBLOCK) {
                if (0 == joylink_adapter_net_check_connect(s)){
                    goto end;    
                }
                
            }
            close(s);
            s = ANET_ERR;
            continue;
        }

        /* If we ended an iteration of the for loop without errors, we
         * have a connected socket. Let's return to the caller. */
        goto end;
    }
    if (p == NULL) {
        joylink_adapter_net_set_error(err, "creating socket: %s", strerror(errno));
        log_error("errno:%d, %s", errno, err);
    }

error:
    if (s != ANET_ERR) {
        close(s);
        s = ANET_ERR;
    }
end:
    freeaddrinfo(servinfo);
    return s;
#endif
}
#endif

/**
 * brief: 
 *
 * @Param: err
 * @Param: addr
 * @Param: port
 *
 * @Returns: 
 */
int32_t
joylink_adapter_net_tcp_connect(char *err, const char *addr, int32_t port)
{
    return joylink_adapter_net_tcp_generic_connect(err, addr,port,ANET_CONNECT_NONE);
}

/**
 * brief: 
 *
 * @Param: err
 * @Param: addr
 * @Param: port
 *
 * @Returns: 
 */
int32_t
joylink_adapter_net_tcp_non_block_connect(char *err, char *addr, int32_t port)
{
    return joylink_adapter_net_tcp_generic_connect(err,addr,port,ANET_CONNECT_NONBLOCK);
}

int32_t
joylink_adapter_net_read_no_check_out(int32_t fd, char *buf, int32_t count)
{
    return read(fd,buf,count);
}

/* Like read(2) but make sure 'count' is read before to return
 * (unless error or EOF condition is encountered) */
int32_t
joylink_adapter_net_read(int32_t fd, char *buf, int32_t count)
{
    int32_t nread, totlen = 0;
    while(totlen != count) {
#if defined(__OV_788__)
	nread = tsock_recv(fd, buf, count-totlen); 
#else
        nread = read(fd,buf,count-totlen);
#endif
        if (nread == 0) return totlen;
        if (nread == -1) return -1;
        totlen += nread;
        buf += nread;
    }
    return totlen;
}

extern void joylink_cloud_set_st(int32_t value);

/* Like write(2) but make sure 'count' is read before to return
 * (unless error is encountered) */
int32_t
joylink_adapter_net_write(int32_t fd, char *buf, int32_t count)
{
    int32_t nwritten, totlen = 0;
    while(totlen != count) {
#if defined(__OV_788__)
        nwritten = tsock_send(fd, buf, count-totlen);
#else
        nwritten = write(fd,buf,count-totlen);
#endif
        if (nwritten == 0) return totlen;
        if (nwritten == -1) return -1;
        totlen += nwritten;
        buf += nwritten;
    }
    return totlen;
}

/**
 * brief: 
 *
 * @Param: fd
 * @Param: buf
 * @Param: size
 *
 * @Returns: 
 */
int32_t
joylink_adapter_net_send(int32_t fd, char *buf, int32_t size)
{
    if(fd < 0){
        printf("------------- XXXXX fd is -1 XXXXXXXX\n");
        return E_ERROR;
    }
    int32_t count = 0;
    int32_t ret = E_ERROR;
SEND:
    log_info("fd:%d, size:%d", fd, size);
#if defined(__OV_788__)
    ret  = tsock_send(fd, buf, size);
#else
    ret = send(fd, buf, size, 0);
#endif
    count++;
    if (ret >= 0) {
        if (ret == size) {
            log_info("net send OK : %d", size);
            return size;
        }
        if (ret < size) {
            log_info("net send OK : %d", ret);
            int32_t ret_l = joylink_adapter_net_send(fd, buf + ret, size - ret);
            if (ret_l == (size - ret)) {
                log_info("net send OK : %d", size);
                return size;
            } else {
#if defined(__OV_788__)
                tsock_close(fd);
#else
                close(fd);
#endif
                return ret_l;
            }
        }
    } else {
#if defined(__QC_4010__) || defined(__OV_788__)
         log_error("unknown error! errno:%d", ret);
#else
        switch(errno){
            case EINTR:         //Interrupted system call
                log_error("interrupted system call");
                close(fd);
                break;
            case EAGAIN:        //Try again
                log_info("count:%d", count);
                if (count <= 3) {
#if defined(__MICOKIT_3166__) || defined(__MW300__) || defined(__ESP32__)
#elif defined(__MTK_7687__)
                    os_sleep(0, 10);
#else
                    usleep(10);
#endif
                    goto SEND;    
                } else {
                    /*close fd, set st*/
                    log_error("retry send 10 times, net send error");
                    close(fd);
                }
                break;
            case EBADF:         //Bad file number
                log_error("bad file number");
                close(fd);
                break;
            default:
                log_error("unknown error! errno:%d, %s", errno, strerror(errno));
                close(fd);
        }    
#endif
    }
    return ret;
}

/**
 * brief: 
 *
 * @Param: fd
 * @Param: buf
 * @Param: size
 *
 * @Returns: 
 */
int32_t
joylink_adapter_net_recv(int32_t fd, char *buf, int32_t size)
{
    int32_t count  = 0;
    int32_t ret = E_ERROR;

SEND:
#if defined(__OV_788__)
    ret = tsock_recv(fd, buf, size);
#else
    ret = recv(fd, buf, size, 0);
#endif
    count++;
    if (ret > 0) {
        if (ret == size) {
            /*log_info("net recv OK : %d", size);*/
            return size;
        }
        if (ret < size) {
            /*log_info("net recv OK : %d", ret);*/
            int32_t ret_l = joylink_adapter_net_recv(fd, buf + ret, size - ret);
            if (ret_l == (size - ret)) {
                /*log_info("net recv OK : %d", size);*/
                return size;
            } else {
#if defined(__OV_788__)
                tsock_close(fd);
#else
                close(fd);
#endif
                return ret_l;
            }
        }
    } else if (ret == 0) {
#if defined(__OV_788__)
                tsock_close(fd);
#else
                close(fd);
#endif
        log_error("tcp is disconnected");
        return E_ERROR;
    } else if (ret < 0) {
#if defined(__QC_4010__) || defined(__OV_788__)
         log_error("unknown error! errno:%d", ret);
#else
        switch(errno){
            case EINTR:         //Interrupted system call
                break;
            case EAGAIN:        //Try again
                if (count <= 10) {
                    sleep(1);
                    goto SEND;    
                } else {
                    /*close fd*/
                    log_error("timeout 10s, net recv error");
                    close(fd);
                }
                break;
            case EBADF:         //Bad file number
                close(fd);
                break;
            default:
                log_error("unknown error! errno:%d, %s", errno, strerror(errno));
                close(fd);
        }
#endif
    }
    return ret;
}

/**
 * [joylink_adapter_net_recv_with_time] 
 *
 * @param: [fd]
 * @param: [buf]
 * @param: [size]
 * @param: [usec]
 * @param: [sec]
 *
 * @returns: 
 */
int32_t
joylink_adapter_net_recv_with_time(int32_t fd, char *buf, int32_t size, int32_t usec, int32_t sec)
{
    struct timeval select_time_out;
    fd_set  read_fds;
    int32_t ret;

    FD_ZERO(&read_fds);
    select_time_out.tv_usec = usec;
    select_time_out.tv_sec = (long)sec;

#if defined(__OV_788__)
    ret = select(&read_fds, NULL, NULL, &select_time_out);
#else
    ret = select(fd + 1, &read_fds, NULL, NULL, &select_time_out);
#endif
    if (ret < 0){
        log_error("select err: %d!\r\n", ret);
    }else if (ret == 0){
        log_error("select err timeout: %d!\r\n", ret);
    }else if (ret > 0){
        return joylink_adapter_net_recv(fd, buf, size);
    }

    return E_ERROR;
}

/**
 * brief: 
 *
 * @Param: err
 * @Param: s
 * @Param: sa
 * @Param: len
 * @Param: backlog
 *
 * @Returns: 
 */
static int32_t 
joylink_net_listen(char *err, int32_t s, struct sockaddr *sa, socklen_t len, int32_t backlog) {
	
#if defined(__OV_788__)
    struct sockaddr_in *in = (struct sockaddr_in *)sa;
    if (tsock_bind(s, in->sin_port) == -1) {
        log_error(err, "bind: %d", s);
        tsock_close(s);
#else
    if (bind(s,sa,len) == -1) {
        joylink_adapter_net_set_error(err, "bind: %s", strerror(errno));
        close(s);
#endif
        return ANET_ERR;
    }

#if defined(__OV_788__)
    if (tsock_listen(s, backlog) == -1) {
        log_error(err, "listen: %d", s);
        tsock_close(s);
        return ANET_ERR;
    }
#else
if (listen(s, backlog) == -1) {
        joylink_adapter_net_set_error(err, "listen: %s", strerror(errno));
        close(s);
        return ANET_ERR;
    }
#endif
    return ANET_OK;
}

/**
 * brief: 
 *
 * @Param: err
 * @Param: s
 *
 * @Returns: 
 */
static int32_t 
joylink_adapter_net_v6_only(char *err, int32_t s) 
{
#ifndef __LINUX__
	printf("__MICOKIT_3166__:not support IPV6\n");
	return ANET_ERR;
#else
	int32_t yes = 1;
    if (setsockopt(s,IPPROTO_IPV6,IPV6_V6ONLY,&yes,sizeof(yes)) == -1) {
        joylink_adapter_net_set_error(err, "setsockopt: %s", strerror(errno));
        close(s);
        return ANET_ERR;
    }
    return ANET_OK;
#endif
}

#if defined(__LINUX__)
static int32_t 
_joylink_net_tcp_server(char *err, int32_t port, char *bindaddr, int32_t af, int32_t backlog)
{
#if defined(__QC_4010__)
    printf("__QC_4010__:struct addrinfo not support\n");
    return ANET_ERR;
#else
    int32_t s, rv;
    char _port[6];  /* strlen("65535") */
    struct addrinfo hints, *servinfo, *p;

    snprintf(_port,6,"%d",port);

    memset(&hints,0,sizeof(hints));
    hints.ai_family = af;
    hints.ai_socktype = SOCK_STREAM;
    /*AI_PASSIVE == 0x0001*/
	hints.ai_flags = 0x0001;

    if ((rv = getaddrinfo(bindaddr, _port, &hints, &servinfo)) != 0) {
        joylink_adapter_net_set_error(err, "getaddrinfo:%d", rv);
        return ANET_ERR;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((s = socket(p->ai_family,p->ai_socktype,p->ai_protocol)) == -1)
            continue;
#if defined(__LINUX__) || defined(__LINUX_UB2__) 
        if (af == AF_INET6 && joylink_adapter_net_v6_only(err,s) == ANET_ERR) goto error;
#endif
		if (joylink_adapter_net_set_reuse_addr(err,s) == ANET_ERR) goto error;
        if (joylink_net_listen(err,s,p->ai_addr,p->ai_addrlen,backlog) == ANET_ERR) goto error;

        goto end;
    }
    if (p == NULL) {
        joylink_adapter_net_set_error(err, "unable to bind socket");
        goto error;
    }

error:
    s = ANET_ERR;
end:
    freeaddrinfo(servinfo);
    return s;
#endif
}
#else
static int32_t 
_joylink_net_tcp_server(char *err, int32_t port, char *bindaddr, int32_t af, int32_t backlog)
{
	int32_t fd = -1;
	struct sockaddr_in sin;

    memset(&sin, 0, sizeof(sin));                
    sin.sin_family      = AF_INET;
    sin.sin_port        = htons(port);
#if !defined(__OV_788__)
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
#endif

#if defined(__OV_788__)
	fd = tsock_init(TSOCK_TCP_LISTEN, 0, 0);
#else
	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#endif
	if(fd < 0){
		LOG_ERROR_RET("tcp socket failed");
	}

#if defined(__OV_788__)
	if(tsock_bind(fd, sin.sin_port) < 0){
		tsock_close(fd);
#else
	if(bind(fd, (struct sockaddr *)&sin, sizeof(struct sockaddr_in)) < 0){
		close(fd);
#endif
		fd = -1;
		LOG_ERROR_RET("tcp bind failed");
	}
	
#if defined(__OV_788__)
	if(tsock_listen(fd, 6) < 0){
		tsock_close(fd);
#else
	if(listen(fd, 6) < 0){
		close(fd);
#endif
		fd = -1;
		LOG_ERROR_RET("tcp listen failed");
	}
RET:
	return fd;
}
#endif

/**
 * brief: 
 *
 * @Param: err
 * @Param: port
 * @Param: bindaddr
 * @Param: backlog
 *
 * @Returns: 
 */
int32_t
joylink_adapter_net_tcp_server(char *err, int32_t port, char *bindaddr, int32_t backlog)
{
    return _joylink_net_tcp_server(err, port, bindaddr, AF_INET, backlog);
}

/**
 * brief: 
 *
 * @Param: err
 * @Param: port
 * @Param: bindaddr
 * @Param: backlog
 *
 * @Returns: 
 */
int32_t
joylink_adapter_net_tcp6_server(char *err, int32_t port, char *bindaddr, int32_t backlog)
{
    #ifndef __LINUX__
	printf("__MICOKIT_3166__:not support IPV6\n");
	return ANET_ERR;
	#else
	return _joylink_net_tcp_server(err, port, bindaddr, AF_INET6, backlog);
	#endif
}
static int32_t joylink_net_generic_accept(char *err, int32_t s, struct sockaddr *sa, socklen_t *len) {
    int32_t fd;
    while(1) {
#if defined(__OV_788__)
        struct sockaddr_in *in = (struct sockaddr_in*)sa;
        fd = tsock_accept(s);        
        memset(in, 0, sizeof(struct sockaddr_in));
        memcpy(&in->sin_addr, ((struct tsock*)s)->ripaddr, sizeof(in->sin_addr));
        in->sin_port = ((struct tsock*)s)->rport;
#else
        fd = accept(s,sa,len);        
#endif
        if (fd == -1) {
#if defined(__QC_4010__) || defined(__OV_788__)
#else
            if (errno == EINTR)
                continue;
            else {
                joylink_adapter_net_set_error(err, "accept: %s", strerror(errno));
                return ANET_ERR;
            }
#endif
        }
        break;
    }
    return fd;
}

/**
 * brief: 
 *
 * @Param: err
 * @Param: s
 * @Param: ip
 * @Param: ip_len
 * @Param: port
 *
 * @Returns: 
 */
int32_t
joylink_net_tcp_accept(char *err, int32_t s, char *ip, int32_t ip_len, int32_t *port) {
    int32_t fd;
#if defined(__REALTEK_8711__)
    struct sockaddr sa;
#elif defined(__OV_788__)
    struct sockaddr_in sa;
#else
    struct sockaddr_storage sa;
#endif
    socklen_t salen = sizeof(sa);
    if ((fd = joylink_net_generic_accept(err,s,(struct sockaddr*)&sa,&salen)) == -1){
        log_error("joylink_net_generic_accept fd:%d, %d", fd, salen);
        return ANET_ERR;
    }

#if defined(__REALTEK_8711__) 
    if (sa.sa_family == AF_INET) {
#elif defined(__OV_788__)
    if (sa.sin_family == AF_INET) {
#else
    if (sa.ss_family == AF_INET) {
#endif
        struct sockaddr_in *si = (struct sockaddr_in *)&sa;
#if defined(__QC_4010__) || defined(__REALTEK_8711__) || defined(__OV_788__) | defined(__MTK_7687__)
        extern char * joylink_adapter_inet_ntoa(struct in_addr in_a);
        if (ip) {
            char *temp_ip = joylink_adapter_inet_ntoa(si->sin_addr);
            memcpy(ip,  temp_ip, strlen(temp_ip));
        }
#else
        if (ip) inet_ntop(AF_INET,(void*)&(si->sin_addr),ip,ip_len);
#endif
        if (port) *port = ntohs(si->sin_port);
    } else {
    	#ifndef __LINUX__
		#else
        struct sockaddr_in6 *s6 = (struct sockaddr_in6 *)&sa;
        if (ip) inet_ntop(AF_INET6,(void*)&(s6->sin6_addr),ip,ip_len);
        if (port) *port = ntohs(s6->sin6_port);
		#endif
    }
    return fd;
}

/**
 * brief: 
 *
 * @Param: fd
 * @Param: ip
 * @Param: ip_len
 * @Param: port
 *
 * @Returns: 
 */
int32_t
joylink_net_peer_to_string(int32_t fd, char *ip, int32_t ip_len, int32_t *port) {
#if defined(__REALTEK_8711__)
    struct sockaddr sa;
#elif defined(__OV_788__)
    struct sockaddr_in sa;
#else
    struct sockaddr_storage sa;
#endif
    socklen_t salen = sizeof(sa);

#if defined(__OV_788__)
    memset(&sa, 0, sizeof(struct sockaddr_in));
    memcpy(&sa.sin_addr, ((struct tsock*)fd)->ripaddr, sizeof(sa.sin_addr));
    sa.sin_port = ((struct tsock*)fd)->rport;
#else
    if (getpeername(fd,(struct sockaddr*)&sa,&salen) == -1) {
        if (port) *port = 0;
        ip[0] = '?';
        ip[1] = '\0';
        return -1;
    }
#endif

#if defined(__REALTEK_8711__)
    if (sa.sa_family == AF_INET) {
#elif defined(__OV_788__)
    if (sa.sin_family == AF_INET) {
#else
    if (sa.ss_family == AF_INET) {
#endif
        struct sockaddr_in *s = (struct sockaddr_in *)&sa;
#if defined(__REALTEK_8711__)|| defined(__OV_788__)
        extern char * joylink_adapter_inet_ntoa(struct in_addr in_a);
        if (ip) {
            char *temp_ip = joylink_adapter_inet_ntoa(s->sin_addr);
            memcpy(ip,  temp_ip, strlen(temp_ip));
        }
#else
        if (ip) inet_ntop(AF_INET,(void*)&(s->sin_addr),ip,ip_len);
#endif
        if (port) *port = ntohs(s->sin_port);
    } 
    else 
    {
        #ifndef __LINUX__
        #else
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)&sa;
        if (ip) inet_ntop(AF_INET6,(void*)&(s->sin6_addr),ip,ip_len);
        if (port) *port = ntohs(s->sin6_port);
        #endif
    }
    return 0;
}

/**
 * brief: 
 *
 * @Param: fd
 * @Param: ip
 * @Param: ip_len
 * @Param: port
 *
 * @Returns: 
 */
int32_t
joylink_net_sock_name(int32_t fd, char *ip, int32_t ip_len, int32_t *port) {
#if defined(__REALTEK_8711__)
    struct sockaddr sa;
#elif defined(__OV_788__)
    struct sockaddr_in sa;
#else
    struct sockaddr_storage sa;
#endif
    socklen_t salen = sizeof(sa);

#if defined(__OV_788__)
    memset(&sa, 0, sizeof(struct sockaddr_in));
    memcpy(&sa.sin_addr, ((struct tsock*)fd)->ripaddr, sizeof(sa.sin_addr));
    sa.sin_port = ((struct tsock*)fd)->rport;
#else
    if (getsockname(fd,(struct sockaddr*)&sa,&salen) == -1) {
        if (port) *port = 0;
        ip[0] = '?';
        ip[1] = '\0';
        return -1;
    }
#endif

#if defined(__REALTEK_8711__)
    if (sa.sa_family == AF_INET) {
#elif defined(__OV_788__)
    if (sa.sin_family == AF_INET) {
#else
    if (sa.ss_family == AF_INET) {
#endif
        struct sockaddr_in *s = (struct sockaddr_in *)&sa;
#if defined(__REALTEK_8711__) || defined(__OV_788__)
        extern char * joylink_adapter_inet_ntoa(struct in_addr in_a);
        if (ip) {
            char *temp_ip = joylink_adapter_inet_ntoa(s->sin_addr);
            memcpy(ip,  temp_ip, strlen(temp_ip));
        }
#else
        if (ip) inet_ntop(AF_INET,(void*)&(s->sin_addr),ip,ip_len);
#endif
        if (port) *port = ntohs(s->sin_port);
    } 
    else 
    {
#ifndef __LINUX__
#else
      struct sockaddr_in6 *s = (struct sockaddr_in6 *)&sa;
        if (ip) inet_ntop(AF_INET6,(void*)&(s->sin6_addr),ip,ip_len);
        if (port) *port = ntohs(s->sin6_port);
#endif
    }
    return 0;
}

/**
 * brief: 
 *
 * @Param: cp
 * @Param: ap
 *
 * @Returns: 
 */
int  
joylink_adapter_inet_aton(const char *cp, struct in_addr *ap)
{
    int dots = 0;
	#if defined(__MICOKIT_3166__) || defined(__REALTEK_8711__) || defined(__OV_788__)
	register unsigned long acc = 0, addr = 0;

	#else
    register u_long acc = 0, addr = 0;
	#endif
    do {
        register char cc = *cp;

        switch (cc) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                acc = acc * 10 + (cc - '0');
                break;

            case '.':
                if (++dots > 3) {
                    return 0;
                }
                /* Fall through */

            case '\0':
                if (acc > 255) {
                    return 0;
                }
                addr = addr << 8 | acc;
                acc = 0;
                break;

            default:
                return 0;
        }
    } while (*cp++) ;

    /* Normalize the address */
    if (dots < 3) {
        addr <<= 8 * (3 - dots) ;
    }

    /* Store it if requested */
    if (ap) {
        ap->s_addr = htonl(addr);
    }

    return 1;    
}

/*int inet_aton(const char *cp, struct in_addr *inp);*/

         /*in_addr_t joylink_adapter_inet_addr(const char *cp);*/
/* 
  * Ascii internet address interpretation routine. 
  * The value returned is in network order. 
  */ 
/*  */ 
unsigned long 
joylink_adapter_inet_addr(const char *cp)
{ 
#if defined(__LINUX__) || defined(__MICOKIT_3166__)
    return inet_addr(cp);
#else
    struct in_addr val;

    if (joylink_adapter_inet_aton(cp, &val))
        return (val.s_addr);
    return (INADDR_NONE);
#endif
} 
  
/**
 * brief: 
 *
 * @Param: in_a
 *
 * @Returns: 
 */
char *  
joylink_adapter_inet_ntoa(struct in_addr in_a)
{
#if defined(__LINUX__) || defined(__MICOKIT_3166__)
    return inet_ntoa(in_a);
#else
    static char buffer[16];
    memset(buffer, 0, sizeof(buffer));
    
    uint32_t tm = 0;
    memcpy(&tm, &in_a, 4);
    unsigned char *bytes = (unsigned char *) &tm;
    snprintf( buffer, sizeof (buffer), "%d.%d.%d.%d", bytes[0], bytes[1], bytes[2], bytes[3] );
    return buffer;
#endif
}

/**
 * brief: 
 *
 * @Param: nfds
 * @Param: readfds
 * @Param: writefds
 * @Param: exceptfds
 * @Param: timeout
 *
 * @Returns: 
 */
int32_t 
joylink_adapter_select(int nfds, fd_set *readfds, fd_set *writefds,
                fd_set *exceptfds, struct timeval *timeout)
{
#if defined(__OV_788__)
    return select(readfds, writefds, exceptfds, timeout);
#else
    return select(nfds, readfds, writefds, exceptfds, timeout);
#endif
}

/**
 * brief: 
 *
 * @Param: fd
 *
 * @Returns: 
 */
int32_t 
joylink_adapter_net_close(int32_t fd)
{
#if defined(__OV_788__)
    return tsock_close(fd);
#else
    return close(fd);
#endif
}

/**
 * brief: 
 *
 * @Returns: 
 */
int32_t 
joylink_adapter_udp_socket()
{
#if defined(__MICOKIT_3166__) || defined(__MW300__)
	return socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#elif defined(__QC_4010__) || defined(__REALTEK_8711__)
	return socket(AF_INET,SOCK_DGRAM, 0);
#elif defined(__OV_788__)
	return tsock_init(TSOCK_UDP, 0, 0);
#else
	return socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
#endif  
}

#if defined(__ANDROID__) || defined(__IOS__)
E_JL_RETCODE 
joylink_adapter_multi_net_get_ip(char* ipv4, const uint32_t max_size)
{
    int i = 0, buf_size = 512;
    int sock_fd = -1;
    struct ifreq *ifreq;
    struct ifconf ifconf;
    char *buf = (char *)malloc(buf_size);
    char *sin_addr = NULL;
    E_JL_RETCODE ret = E_ERROR;

    ifconf.ifc_len = buf_size;
    ifconf.ifc_buf = buf;
    if((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        log_error("socket create failed");
        goto RET;
    }
    ioctl(sock_fd, SIOCGIFCONF, &ifconf);

    ifreq = (struct ifreq *)buf;
    i = ifconf.ifc_len / sizeof(struct ifreq);
    int count = 0;
    for(count = 0; (count < 5 && i > 0); i--){
        sin_addr = inet_ntoa(((struct sockaddr_in*)&(ifreq->ifr_addr))->sin_addr);
        log_debug("ip addr:%s", sin_addr);
        if(strcmp(sin_addr, "127.0.0.1") && (strcmp(sin_addr, ""))){
            strncpy(ipv4, sin_addr, max_size);
            ret = E_OK;
            break;
        }else{
            count++;
            ifreq++;
            continue;
        }
    }
RET:
    if(NULL != buf){
        free(buf);
    }
    if(sock_fd > 0){
        close(sock_fd);
    }

    return ret;
}
#endif
