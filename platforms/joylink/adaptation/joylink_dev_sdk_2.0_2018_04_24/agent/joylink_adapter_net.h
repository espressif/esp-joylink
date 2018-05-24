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

#ifndef JOYLINK_ADAPTER_NET_TCP_H
#define JOYLINK_ADAPTER_NET_TCP_H 

#include <stdint.h>

#define ANET_OK                     0
#define ANET_ERR                    -1
#define ANET_ERR_LEN                256

/* Flags used with certain functions. */
#define ANET_NONE                   0
#define ANET_IP_ONLY (1<<0)

#if defined(__sun)
#define AF_LOCAL AF_UNIX
#endif

#include "joylink_ret_code.h"
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
joylink_adapter_net_tcp_connect(char *err, const char *addr, int32_t port);

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
joylink_adapter_net_tcp_non_block_connect(char *err, char *addr, int32_t port);

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
joylink_adapter_net_tcp_server(char *err, int32_t port, char *bindaddr, int32_t backlog);

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
joylink_adapter_net_tcp6_server(char *err, int32_t port, char *bindaddr, int32_t backlog);

/**
 * brief: 
 *
 * @Param: err
 * @Param: serversock
 * @Param: ip
 * @Param: ip_len
 * @Param: port
 *
 * @Returns: 
 */
int32_t 
joylink_net_tcp_accept(char *err, int32_t serversock, char *ip, int32_t ip_len, int32_t *port);

/**
 * brief: 
 *
 * @Param: err
 * @Param: fd
 *
 * @Returns: 
 */
int32_t 
joylink_adapter_net_tcp_keep_alive(char *err, int32_t fd);

/**
 * brief: 
 *
 * @Param: err
 * @Param: serversock
 *
 * @Returns: 
 */
int32_t 
joylink_net_unix_accept(char *err, int32_t serversock);

/**
 * brief: 
 *
 * @Param: err
 * @Param: fd
 *
 * @Returns: 
 */
int32_t 
joylink_adapter_net_non_block(char *err, int32_t fd);

/**
 * brief: 
 *
 * @Param: err
 * @Param: fd
 *
 * @Returns: 
 */
int32_t 
joylink_adapter_net_enable_tcp_no_delay(char *err, int32_t fd);

/**
 * brief: 
 *
 * @Param: err
 * @Param: fd
 *
 * @Returns: 
 */
int32_t 
joylink_adapter_net_disable_tcp_no_delay(char *err, int32_t fd);

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
joylink_net_peer_to_string(int32_t fd, char *ip, int32_t ip_len, int32_t *port);

/**
 * brief: 
 *
 * @Param: err
 * @Param: fd
 * @Param: int32_terval
 *
 * @Returns: 
 */
int32_t 
joylink_adapter_net_keep_alive(char *err, int32_t fd, int32_t int32_terval);

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
joylink_net_sock_name(int32_t fd, char *ip, int32_t ip_len, int32_t *port);

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
joylink_adapter_net_resolve(char *err, char *host, char *ipbuf, int32_t ipbuf_len);

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
joylink_adapter_net_resolve_ip(char *err, char *host, char *ipbuf, int32_t ipbuf_len);

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
joylink_adapter_net_send(int32_t fd, char *buf, int32_t size);

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
joylink_adapter_net_recv(int32_t fd, char *buf, int32_t size);

/**
 * brief: 
 *
 * @Param: fd
 * @Param: buf
 * @Param: count
 *
 * @Returns: 
 */
int32_t 
joylink_adapter_net_read(int32_t fd, char *buf, int32_t count);

/**
 * brief: 
 *
 * @Param: fd
 * @Param: buf
 * @Param: count
 *
 * @Returns: 
 */
int32_t 
joylink_adapter_net_write(int32_t fd, char *buf, int32_t count);

/**
 * brief: 
 *
 * @Param: fd
 * @Param: buf
 * @Param: size
 * @Param: usec
 * @Param: sec
 *
 * @Returns: 
 */
int32_t
joylink_adapter_net_recv_with_time(int32_t fd, char *buf, int32_t size, int32_t usec, int32_t sec);

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
joylink_adapter_net_set_send_buf(char *err, int32_t fd, int32_t buffsize);

/**
 * brief: 
 *
 * @Param: fd
 *
 * @Returns: 
 */
int32_t 
joylink_adapter_net_close(int32_t fd);


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
                fd_set *exceptfds, struct timeval *timeout);

/**
 * brief: 
 *
 * @Returns: 
 */
int32_t 
joylink_adapter_udp_socket();

#endif
