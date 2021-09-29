/*
 * @Author: your name
 * @Date: 2020-07-09 09:09:21
 * @LastEditTime: 2020-12-03 15:49:56
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: \joylink_dev_sdk_2.1\pal\inc\joylink_http.h
 */ 
#ifndef _JOYLINK_HTTP_H_
#define _JOYLINK_HTTP_H_
#include <stdbool.h>
#include "joylink_stdint.h"


typedef struct _jl_http_t{
    char        *url;
    char        *header;
    char        *body;
    char        *response;
    int32_t      resp_len;
}jl_http_t;

/**
 * @name:实现HTTPS的POST请求,请求响应填入revbuf参数 
 *
 * @param[in]: url POST请求的链接和路径
 * @param[in]: header POST请求的HEADER
 * @param[in]: body POST请求的Payload
 * @param[out]: revbuf 填入请求的响应信息
 *
 * @returns:   NULL - 发生错误, 其它 - 请求返回的数据 ，使用完毕内存需要释放
 *
 * @note:此函数必须正确实现,否则设备无法激活绑定
 * @note:小京鱼平台HTTPS使用的证书每年都会更新. 
 * @note:因为Joylink协议层面有双向认证的安全机制,所以此HTTPS请求设备无需校验server的证书. 
 * @note:如果设备必须校验server的证书,那么需要开发者实现时间同步等相关机制.
 */
int32_t jl_platform_https_request(jl_http_t *info);

/**
 * @name:实现HTTP的POST请求,请求响应填入revbuf参数 
 *
 * @param[in]: url POST请求的链接和路径
 * @param[in]: header POST请求的HEADER
 * @param[in]: body POST请求的Payload
 * @param[out]: revbuf 填入请求的响应信息
 *
 * @returns:   NULL - 发生错误, 其它 - 请求返回的数据 ，使用完毕内存需要释放
 */
int32_t jl_platform_http_request(jl_http_t *info);

#endif
