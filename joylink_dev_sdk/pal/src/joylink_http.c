
#include "joylink_socket.h"
#include "joylink_http.h"
#include "joylink_string.h"
#include "joylink_memory.h"
#include "joylink_stdio.h"
#include "joylink_log.h"
#include "esp_tls.h"

#define SET_HTTP_RECV_MAX_LEN 64

static int joylink_dev_http_parse_content(
	char *response, 
	int response_len, 
	char *content,
	int content_len)
{
	int length = 0;
	content[0] = 0;

	char *p = jl_platform_strstr(response, "\r\n\r\n");
	if(p == NULL){
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

static void jl_platform_get_host(char *url, char *host, char *path)
{
	char *p = NULL, *p1 = NULL;
	int length = 0;

	jl_platform_strcpy(host, url);
	path[0] = '\0';
	p = jl_platform_strstr(url, "://");
	if(p == NULL){
		goto RET;
	}
	p += 3;
	p1 = jl_platform_strchr(p, '/');
	length = p1 - p;
	if(length >= 0)
	{
		// 如果有路径
		jl_platform_memcpy(host, p, length);
		host[length] = '\0';
		jl_platform_strcpy(path, p1);
		length = jl_platform_strlen(p1);
		path[length] = '\0';
	}
	else
	{
		// 如果没有路径
		length = jl_platform_strlen(p);
		jl_platform_strcpy(host, p);
		host[length] = '\0';
		path[0] = '\0';
	}

RET:
	log_debug("url = %s, host = %s, path = %s", url, host, path);
    
	return;
}

static char *jl_platform_get_request_data(char *host, char *path, char *header, char *body)
{
	char *request = NULL;
	int length = 0, offset = 0;
	
	if (!path)
	{
		goto RET;
	}
	
	if (body)
		length = jl_platform_strlen(body);
	request = jl_platform_malloc(length + 300);
	if(request == NULL)
		goto RET;
	
	offset = jl_platform_sprintf(request, "POST %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\nAccept: */*\r\n", path, host);
	if(header)
		offset += jl_platform_sprintf(request + offset, "%s\r\n", header);
	if(length)
		offset += jl_platform_sprintf(request + offset, "Content-Length:%d\r\n\r\n", length);
	if(body)
		offset += jl_platform_sprintf(request + offset, "%s", body);
	offset += jl_platform_sprintf(request + offset, "\r\n");

RET:
	if(!request)
		log_debug("request is NULL");
	else
		log_debug("request = %s", request);

	return request;
}

int32_t jl_platform_https_request(jl_http_t *info)
{
    int len = 0, ret = -1;
	char host[50];
	char path[100];
	char *request;
	
	if ( !info || !info->url || !info->response )
	{
		log_error("Invalid argument\n");
		return NULL;
	}

	jl_platform_get_host(info->url, host, path);
    esp_tls_cfg_t config = {
        // .skip_common_name = true,
    };
    esp_tls_t *tls = NULL;
    log_debug("jl_platform_https_request host:%s\r\n", host);
	request = jl_platform_get_request_data(host, path, info->header, info->body);
	if(request)
	{
		tls = esp_tls_conn_new(host, strlen(host), 443, &config);
		if (tls == NULL)
		{
			jl_platform_printf("esp_tls_conn_new failed\n");
			goto RET;
		}
		esp_tls_conn_write(tls, info->body, strlen(info->body));

		len = esp_tls_conn_read(tls, info->response, info->resp_len);
		esp_tls_conn_delete(tls);
	}

	log_info("... read data length = %d, response data = \r\n%s", len, info->response);
	ret = joylink_dev_http_parse_content(info->response, len, info->response, info->resp_len);
    // log_debug("revbuf:%d, %s\r\n",len,revbuf);
RET:
	if(request)
		jl_platform_free(request);

    return ret;
}

int32_t jl_platform_http_request(jl_http_t *info)
{
	char *recv_buf = NULL;
	int log_socket = -1;
	jl_sockaddr_in saServer; 
	char ip[20] = {0};
	char host[50];
	char path[100];
	char *request = NULL, *p;
	int ret = -1;
	
	if ( !info || !info->url || !info->response)
	{
		log_error("Invalid argument\n");
		return -1;
	}

	jl_platform_get_host(info->url, host, path);
	request = jl_platform_get_request_data(host, path, info->header, info->body);
	if(!request)
	{
		goto RET;
	}
	
	p = jl_platform_strchr(host, ':');
	if(p)
		*p = '\0';
	jl_platform_memset(ip,0,sizeof(ip)); 
	ret = jl_platform_gethostbyname(host, ip, SOCKET_IP_ADDR_LEN);
	if(ret < 0){
		log_error("get ip error");
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

	if (jl_platform_send(log_socket, request, jl_platform_strlen(request), 0, 5000) < 0) {
		log_error("... socket send failed");
		goto RET;
	}

	int count = 0;
	int offset = 0;
	int read_len = 0;

	recv_buf = (char *)jl_platform_malloc(SET_HTTP_RECV_MAX_LEN);
	if(recv_buf == NULL){
		goto RET;
	}
	jl_platform_memset(recv_buf, 0, SET_HTTP_RECV_MAX_LEN);

	while(1){
		offset = count * SET_HTTP_RECV_MAX_LEN + read_len;
		int ret_len = jl_platform_recv(log_socket, recv_buf + offset, SET_HTTP_RECV_MAX_LEN - read_len, 0, 20000);
		//log_debug("ret_len: %d\n", ret_len);
		if(ret_len > 0){
			read_len += ret_len;
			if(read_len != SET_HTTP_RECV_MAX_LEN){
				continue;
			}
		}
		if(read_len != SET_HTTP_RECV_MAX_LEN)
		{
			int len = offset;
			if(len <= 0){
				log_error("read len error");
				goto RET;
			}
			recv_buf[len] = '\0';
			log_debug("... read data length = %d, response data = \r\n%s", len, recv_buf);
			joylink_dev_http_parse_content(recv_buf, len, info->response, info->resp_len);
			break;
		}
		char *p_now = jl_platform_realloc(recv_buf, SET_HTTP_RECV_MAX_LEN*(count+2));
		if(p_now == NULL){
			log_error("realloc error");
			goto RET;
		}
		recv_buf = p_now;
		count++;
		read_len = 0;
	}
	ret = 0;
RET:
	if(-1 != log_socket){
		jl_platform_close(log_socket);
	}
	if(recv_buf != NULL){
		jl_platform_free(recv_buf);
	}
	if(request != NULL){
		jl_platform_free(request);
	}
	return ret;
}