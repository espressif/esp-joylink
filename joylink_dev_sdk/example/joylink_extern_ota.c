/************************************************************
 *File : joylink_ota.c
 *Auth : jd
 *Date : 2018/07/25
 *Mail : 
 ***********************************************************/

#include "joylink.h"

#include "joylink_extern.h"
#include "joylink_extern_user.h"
#include "joylink_extern_ota.h"
#include "joylink_extern_sub_dev.h"
#include "joylink_stdint.h"

// joylink platform layer header files
#include "joylink_stdio.h"
#include "joylink_string.h"
#include "joylink_memory.h"
#include "joylink_time.h"
#include "joylink_thread.h"
#include "joylink_log.h"
#include "joylink_softap.h"
#include "joylink_socket.h"


extern int joylink_ota_report_status(int status, int progress, char *desc);
extern void joylink_util_timer_reset(jl_time_stamp_t *tv);
extern int joylink_util_is_time_out(jl_time_stamp_t tv, uint32_t timeout);
extern uint32_t make_crc(uint32_t crc, unsigned char *string, uint32_t size);

static int dev_version = 0;
static JLOtaOrder_t otaOrder;

//-----------------------------------------------------------------------
int joylink_socket_create(const char *host, int port)
{
	jl_sockaddr_in server_addr; 
	int socket_fd;
	int ret;
	char ip[20] = {0};
 
    jl_platform_memset(ip,0,sizeof(ip)); 
	ret = jl_platform_gethostbyname((char *)host, ip, SOCKET_IP_ADDR_LEN);
 	if(ret < 0){
		log_error("get ip error");
		return -1;
	}
 
    server_addr.sin_family = jl_platform_get_socket_proto_domain(JL_SOCK_PROTO_DOMAIN_AF_INET);
	server_addr.sin_port = jl_platform_htons(port);
	server_addr.sin_addr.s_addr = jl_platform_inet_addr(ip);
 
    socket_fd = jl_platform_socket(JL_SOCK_PROTO_DOMAIN_AF_INET, JL_SOCK_PROTO_TYPE_SOCK_STREAM, JL_SOCK_PROTO_PROTO_IPPROTO_TCP);
	if(socket_fd == -1){
        log_error("jl_platform_socket failed.");
        	return -1;
	}
 
	if(jl_platform_connect(socket_fd, (jl_sockaddr *)&server_addr,sizeof(jl_sockaddr)) == -1){
		return -1;
	}

    jl_platform_fcntl(socket_fd, JL_FCNTL_CMD_SETFL_O_NONBLOCK);

	return socket_fd;
}

int joylink_socket_send(int socket_fd, char *send_buf, int send_len)
{
	int send_ok = 0;
	int ret_len = 0;

	while(send_ok < send_len){
		ret_len = jl_platform_send(socket_fd, send_buf + send_ok, send_len - send_ok, 0, 0);
		if(ret_len == -1){
			return -1;
		}
		send_ok += ret_len;
	}
	return send_ok;
}

int joylink_parse_url(const char *url, http_ota_st *ota_info)
{
	char *strp1 = NULL;
	char *strp2 = NULL;

	int len = 0;
	int file_len = 0;

	if(!url || !ota_info){
		return -1;
	}

	strp1 = (char *)url;
 
	if(!jl_platform_strncmp(strp1, "http://", jl_platform_strlen("http://"))){
		strp1 += jl_platform_strlen("http://");
	}else{
		return -1;
	}

	strp2 = jl_platform_strchr(strp1,'/');
	if(strp2){
		len = jl_platform_strlen(strp1) - jl_platform_strlen(strp2);
		jl_platform_memcpy(ota_info->host_name, strp1, len);

		if(*(strp2 + 1)){
			jl_platform_memcpy(ota_info->file_path, strp2 + 1, jl_platform_strlen(strp2) - 1);
		}

		file_len = jl_platform_strlen(strp2);
		while(file_len--){
			if(*(strp2 + file_len) == '/'){
				jl_platform_strncpy(ota_info->file_name, strp2 + file_len + 1, sizeof(ota_info->file_name));
				break;
			}
		}
	}
	else{
		jl_platform_memcpy(ota_info->host_name, strp1, jl_platform_strlen(strp1));
	}

	strp1 = jl_platform_strchr(ota_info->host_name, ':');
	if(strp1){
		*strp1++ = 0;
		ota_info->host_port = jl_platform_atoi(strp1);
	}
	else{
		ota_info->host_port = HTTP_DEFAULT_PORT;
	}

	return 0;
}

int joylink_ota_set_download_len(int socket_fd, http_ota_st *ota_info)
{
	char *recv_p1 = NULL;
	char *recv_p2 = NULL;
	char buf[1024] = {0};
	int recv_len = 0;
	int ret_len = 0;

	long int download_len = 0;

	jl_platform_snprintf(buf, sizeof(buf), HTTP_GET, ota_info->file_path, ota_info->host_name);
	log_info("send:\n%s\n",buf);

	if(joylink_socket_send(socket_fd, buf, jl_platform_strlen(buf)) < 0){
		log_error("get_file_data send failed!\n");
		return -1;
	}

	jl_platform_memset(buf, 0, 1024);
	while(1){
		ret_len = jl_platform_recv(socket_fd, buf + recv_len, 1, 0, 2000);
		if(ret_len < 0){
			log_error("get_file_data recv failed!\n");
			return -1;
		}
		
		recv_len += ret_len;

		if(recv_len < 200) continue;

		recv_p1 = jl_platform_strstr(buf, "\r\n\r\n");
		if(recv_p1 == NULL) continue;

		recv_p2 = jl_platform_strstr(buf, "Content-Length");
		if(recv_p2 == NULL){
			log_error("Parse data length error!\n");
			return -1;
		}
		recv_p2 = recv_p2 + jl_platform_strlen("Content-Length") + 2; 
		download_len = jl_platform_atoi(recv_p2);
		ota_info->file_size = download_len;
		break;
	}

	log_info("download_len: %ld, recv:\n%s\n", download_len, buf);

	return download_len;
}

int joylink_ota_get_data(http_ota_st *ota_info)
{
	int socket_fd = 0;
	int32_t download_len = 0;
	int read_len = 0;
	int ret_len = 0;
	int last_percent = 0;
	char recv_buf[EVERY_PACKET_LEN] = {0};
	uint32_t file_crc32 = 0;
	jl_time_stamp_t last_time;
	
	if(joylink_memory_init((void *)ota_info->file_name, MEMORY_WRITE) != 0)
	{
		joylink_ota_report_status(OTA_STATUS_FAILURE, 0, "No flash to update firmware");
		return -1;
	}

	socket_fd = joylink_socket_create(ota_info->host_name, ota_info->host_port);
	if(socket_fd < 0){
		log_error("joylink_socket_create failed\n");
		return -1;
	}
	
	joylink_util_timer_reset(&last_time);
	joylink_ota_report_status(OTA_STATUS_DOWNLOAD, 0, "ota download start");

	download_len = joylink_ota_set_download_len(socket_fd, ota_info);
	if(download_len < 0){
		log_error("set download_len error!\n");
		return -1;
	}

	ota_info->file_offset = read_len = 0;

	while(download_len > 0){
		// memset(recv_buf, 0, EVERY_PACKET_LEN);
		ret_len = jl_platform_recv(socket_fd, recv_buf, EVERY_PACKET_LEN, 0, 3000);
		if(ret_len > 0){

			file_crc32 = make_crc(file_crc32, (unsigned char *)recv_buf, ret_len);
			if(joylink_memory_write(0, recv_buf, ret_len) != ret_len)
			{
				joylink_ota_report_status(OTA_STATUS_FAILURE, 
					ota_info->finish_percent, 
					"Write firmware data to flash failed");
				goto RET;
			}
		}
		else{
			log_error("recv file data error!\n");
			break;
		}

		download_len -= ret_len;
		read_len += ret_len;

		ota_info->finish_percent = read_len * 100 / ota_info->file_size;
		if(last_percent != ota_info->finish_percent )
		{
			last_percent = ota_info->finish_percent;
			if(ota_info->finish_percent % 10 == 0)
				log_info("OTA download percent: %d", ota_info->finish_percent);
		}

	}
	ota_info->file_offset += read_len;

	if(download_len != 0){
		goto RET;
	}

	if(joylink_util_is_time_out(last_time, OTA_TIME_OUT))
	{
		log_info("OTA last_time: %d, OTA_TIME_OUT: %d", last_time, OTA_TIME_OUT);
		joylink_ota_report_status(OTA_STATUS_DOWNLOAD, ota_info->finish_percent, "ota timeout error");
		goto RET;
	}
	joylink_ota_report_status(OTA_STATUS_DOWNLOAD, ota_info->finish_percent, "ota download");

RET:
	jl_platform_close(socket_fd);
	if(joylink_memory_finish() != 0)
	{
		joylink_ota_report_status(OTA_STATUS_FAILURE, 0, "joylink_memory_finish failed");
		return -1;
	}

	if(ota_info->file_size == ota_info->file_offset){
		if(otaOrder.crc32 == file_crc32)
		{
			log_info("[OTA] Check CRC OK\n");
			return 0;
		}
		else
		{
			joylink_ota_report_status(OTA_STATUS_FAILURE, 0, "check crc32 error");
			log_error("[OTA] Check CRC failed, cloud CRC= 0x%08x, local CRC= 0x%08x\n", 
				otaOrder.crc32, file_crc32);
			return -1;
		}
	} else {
			joylink_ota_report_status(OTA_STATUS_FAILURE, 0, "get data error");
			log_error("[OTA] download file failed, cloud size = %d, receive len = %d\n", 
				ota_info->file_size, ota_info->file_offset);
		return -1;	
	}
}

int joylink_ota_report_status(int status, int progress, char *desc)
{
	JLOtaUpload_t otaUpload;

	jl_platform_strncpy(otaUpload.feedid, otaOrder.feedid, JL_MAX_FEEDID_LEN);
	jl_platform_strncpy(otaUpload.productuuid, otaOrder.productuuid, JL_MAX_UUID_LEN);

	otaUpload.status = status;

	log_info("status: %d, version: %d\n", status, dev_version);

	otaUpload.progress = progress;
	jl_platform_strncpy(otaUpload.status_desc, desc, JL_MAX_STATUS_DESC_LEN);

	joylink_server_ota_status_upload_req(&otaUpload);
    return 0;
}

void *joylink_ota_task(void *data)
{
	int ret = 0;
	http_ota_st ota_info;
	
	log_info("\n\nJoylink ota start!\n");

	log_info("\nserial:%d | feedid:%s | productuuid:%s | version:%d | versionname:%s | crc32:%d | url:%s | upgradetype:%d\n\n",\
		otaOrder.serial, otaOrder.feedid, otaOrder.productuuid, otaOrder.version,\
		otaOrder.versionname, otaOrder.crc32, otaOrder.url, otaOrder.upgradetype);

	jl_platform_memset(&ota_info, 0, sizeof(http_ota_st));

	dev_version = otaOrder.version;

	if(joylink_parse_url(otaOrder.url, &ota_info)){
		log_error("http_parse_url failed!\n");
		goto RET;
	}
	log_info("host name: %s port: %d\n", ota_info.host_name, ota_info.host_port);
	log_info("file path: %s name: %s\n\n", ota_info.file_path, ota_info.file_name);

	ret = joylink_ota_get_data(&ota_info);
	if(ret < 0){
		log_error("Joylink ota get data error!\n");
		goto RET;
	}

	if(!jl_platform_strcmp(_g_pdev->jlp.feedid, otaOrder.feedid)){
		_g_pdev->jlp.version = dev_version;
	}
	else{
		joylink_dev_sub_version_update(otaOrder.feedid, dev_version);
	}

	joylink_ota_report_status(OTA_STATUS_SUCCESS, ota_info.finish_percent, "ota is ok");

	jl_platform_memset(&ota_info, 0, sizeof(http_ota_st));

	log_info("Joylink ota is ok!\n");
RET:
    return NULL;
}

//-----------------------------------------------------------------------
void joylink_set_ota_info(JLOtaOrder_t *ota_info)
{
	jl_platform_memset(&otaOrder, 0, sizeof(JLOtaOrder_t));
	jl_platform_memcpy(&otaOrder, ota_info, sizeof(JLOtaOrder_t));
}
