/************************************************************
 *File : joylink_ota.c
 *Auth : jd
 *Date : 2018/07/25
 *Mail : 
 ***********************************************************/

#include "joylink.h"

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

//-----------------------------------------------------------------------
int 
joylink_socket_recv(int socket_fd, char *recv_buf, int recv_len)
{
	int ret = 0;
	jl_fd_set  readfds;
	jl_timeval selectTimeOut;

	readfds = jl_platform_fd_set_allocate();
	JL_FD_ZERO(readfds);
	JL_FD_SET(socket_fd, readfds);

	selectTimeOut.tv_usec = 0L;
	selectTimeOut.tv_sec = (long)2;

	ret = jl_platform_select(socket_fd + 1, readfds, NULL, NULL, &selectTimeOut);
	if (ret <= 0){
		return -1;
	}
	if(JL_FD_ISSET(socket_fd, readfds)){
		return jl_platform_recv(socket_fd, recv_buf, recv_len, 0, 0);
	}
	else{
		return -1;
	}
}

//-----------------------------------------------------------------------
int 
joylink_socket_send(int socket_fd, char *send_buf, int send_len)
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

//-----------------------------------------------------------------------
void 
joylink_socket_close(int socket_fd)
{
	jl_platform_close(socket_fd);
}

//-----------------------------------------------------------------------
int 
joylink_parse_url(const char *url, http_ota_st *ota_info)
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

//-----------------------------------------------------------------------
int
joylink_get_file_size(int socket_fd, http_ota_st *ota_info)
{
	char *str_p = NULL;

	long int file_size = 0;
	
	char buf[512] = {0};

	int ret_len = 0;
	int recv_len = 0;

	jl_platform_snprintf(buf, sizeof(buf), HTTP_HEAD, ota_info->file_path, ota_info->host_name, ota_info->host_port);
	log_info("send:\n%s\n",buf);

	if(joylink_socket_send(socket_fd, buf, jl_platform_strlen(buf)) < 0){
		log_error("get_file_size send failed!\n");
		return -1;
	}

	jl_platform_memset(buf, 0, sizeof(buf));
	while(1)
	{
		ret_len = joylink_socket_recv(socket_fd, buf + recv_len, 512 - recv_len);
		if(ret_len < 0){
			log_error("get_file_size recv failed!\n");
			return -1;
		}

		recv_len += ret_len;

		if(recv_len > 50){
			break;

		}
	}
	log_info("recv:\n%s\n", buf);

	str_p = jl_platform_strstr(buf,"Content-Length");
	if(str_p == NULL){
		log_error("http parse file size error!\n");
		return -1;
	}
	else{
		str_p = str_p + jl_platform_strlen("Content-Length") + 2; 
		file_size = jl_platform_atoi(str_p);
	}

        return file_size;
}

//-----------------------------------------------------------------------

int
joylink_ota_get_info(char *url, http_ota_st *ota_info)
{
	int socket_fd = 0;
	
	if(!url){
		log_error("url is NULL!\n");
		return -1;
	}

	if(joylink_parse_url(url, ota_info)){
		log_error("http_parse_url failed!\n");
		return -1;
	}
	log_info("host name: %s port: %d\n", ota_info->host_name, ota_info->host_port);
	log_info("file path: %s name: %s\n\n", ota_info->file_path, ota_info->file_name);

	socket_fd = joylink_socket_create(ota_info->host_name, ota_info->host_port);
	if(socket_fd < 0){
		log_error("http_tcp client_create failed\n");
		return -1;
	}

	ota_info->file_size = joylink_get_file_size(socket_fd, ota_info);
	if(ota_info->file_size <= 0){
		log_error("get file size failed\n");
		return -1;
	}
	log_info("file size %ld\n\n", ota_info->file_size);

	joylink_socket_close(socket_fd);

    return 0;
}

int
joylink_ota_set_download_len(int socket_fd, http_ota_st *ota_info)
{
	long int max_len = ota_info->file_size;
	long int read_len = max_len;

	char *recv_p1 = NULL;
	char *recv_p2 = NULL;

	char buf[1024] = {0};

	int recv_len = 0;
	int ret_len = 0;

	long int data_len = 0;

	if(ota_info->file_size - ota_info->file_offset < max_len){
		read_len = ota_info->file_size - ota_info->file_offset;
	}

	jl_platform_snprintf(buf, sizeof(buf), HTTP_GET, ota_info->file_path, ota_info->host_name, ota_info->host_port, ota_info->file_offset, max_len);//, (ota_info->file_offset + read_len - 1));
	//log_info("send:\n%s\n",buf);

	if(joylink_socket_send(socket_fd, buf, jl_platform_strlen(buf)) < 0){
		log_error("get_file_data send failed!\n");
		return -1;
	}

	jl_platform_memset(buf, 0, 1024);
	while(1){
		ret_len = joylink_socket_recv(socket_fd, buf + recv_len, 1);
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
		data_len = jl_platform_atoi(recv_p2);
		break;
	}

	log_info("recv:\n%s\n", buf);
	log_info("read_len: %ld data_len: %ld\n", read_len, data_len);

	if(read_len != data_len){
		return -1;
	}

	return read_len;
}

//---------------------------------------------------------------------------
int joylink_ota_report_status(int status, int progress, char *desc);

extern void joylink_util_timer_reset(uint32_t *timestamp);
extern int joylink_util_is_time_out(uint32_t timestamp, uint32_t timeout);

extern uint32_t make_crc(uint32_t crc, unsigned char *string, uint32_t size);

int joylink_ota_get_data(http_ota_st *ota_info)
{
	int socket_fd = 0;
	int32_t download_len = 0;
	int save_len = 0;
	int read_len = 0;
	int ret_len = 0;
	int last_percent = 0;
	char recv_buf[EVERY_PACKET_LEN] = {0};
	uint32_t file_crc32 = 0;
	uint32_t last_time = 0;
	
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

	while(ota_info->file_size > ota_info->file_offset){
		download_len = joylink_ota_set_download_len(socket_fd, ota_info);
		if(download_len < 0){
			log_error("set download_len error!\n");
			break;
		}

		read_len = 0;
		save_len = download_len;

		while(download_len > 0){
			// memset(recv_buf, 0, EVERY_PACKET_LEN);
			ret_len = joylink_socket_recv(socket_fd, recv_buf, EVERY_PACKET_LEN);

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

			ota_info->finish_percent = (ota_info->file_offset + read_len) * 100 / ota_info->file_size;
			if(last_percent !=ota_info->finish_percent )
			{
				last_percent =ota_info->finish_percent;
				log_info("OTA download percent: %d", ota_info->finish_percent);
			}

		}

		if(download_len == 0){
			ota_info->file_offset += save_len;
		}
		else{
			break;
		}

		log_info("OTA last_time: %d, OTA_TIME_OUT: %d", last_time, OTA_TIME_OUT);
		if(joylink_util_is_time_out(last_time, OTA_TIME_OUT))
		{
			joylink_ota_report_status(OTA_STATUS_DOWNLOAD, ota_info->finish_percent, "ota timeout error");
			break;
		}
		joylink_ota_report_status(OTA_STATUS_DOWNLOAD, ota_info->finish_percent, "ota download");
	}

RET:
	joylink_socket_close(socket_fd);
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

//-----------------------------------------------------------------------

int
joylink_ota_report_status(int status, int progress, char *desc)
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

//-----------------------------------------------------------------------
void *
joylink_ota_task(void *data)
{
	int ret = 0;
	http_ota_st ota_info;
	//JLOtaOrder_t *otaOrder = (JLOtaOrder_t *)index;

	jl_platform_memset(&otaOrder, 0, sizeof(JLOtaOrder_t));
	jl_platform_memcpy(&otaOrder, data, sizeof(JLOtaOrder_t));
	
	log_info("\n\nJoylink ota statrt!\n");

	log_info("\nserial:%d | feedid:%s | productuuid:%s | version:%d | versionname:%s | crc32:%d | url:%s\n\n",\
		otaOrder.serial, otaOrder.feedid, otaOrder.productuuid, otaOrder.version,\
		otaOrder.versionname, otaOrder.crc32, otaOrder.url);

	jl_platform_memset(&ota_info, 0, sizeof(http_ota_st));

	dev_version = otaOrder.version;

	ret = joylink_ota_get_info(otaOrder.url, &ota_info);
	if(ret < 0){
		joylink_ota_report_status(OTA_STATUS_FAILURE, 0, "get info error");
		log_error("Joylink ota get info error!\n");
		goto RET;
	}

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

