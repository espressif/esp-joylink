/************************************************************
 *File : joylink_ota.c
 *Auth : jd
 *Date : 2018/07/25
 *Mail : 
 ***********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#include "joylink.h"
#include "joylink_packets.h"
#include "joylink_auth_crc.h"

#include <fcntl.h> 

#include "joylink_extern_user.h"
#include "joylink_extern_ota.h"

//-----------------------------------------------------------------------
int 
joylink_socket_create(const char *host, int port)
{
	struct hostent *he;
	struct sockaddr_in server_addr; 
	int socket_fd;
	int flags;
 
	if((he = gethostbyname(host)) == NULL){
		return -1;
	}
 
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr = *((struct in_addr *)he->h_addr);
 
	if((socket_fd = socket(AF_INET,SOCK_STREAM,0)) == -1){
        	return -1;
	}
 
	if(connect(socket_fd, (struct sockaddr *)&server_addr,sizeof(struct sockaddr)) == -1){
		return -1;
	}

	flags = fcntl(socket_fd, F_GETFL, 0); 
	fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);

	return socket_fd;
}

//-----------------------------------------------------------------------
int 
joylink_socket_recv(int socket_fd, char *recv_buf, int recv_len)
{
	int ret = 0;
	fd_set  readfds;
	struct timeval selectTimeOut;

	FD_ZERO(&readfds);
	FD_SET(socket_fd, &readfds);

        selectTimeOut.tv_usec = 0L;
        selectTimeOut.tv_sec = (long)2;

	ret = select(socket_fd + 1, &readfds, NULL, NULL, &selectTimeOut);
	if (ret <= 0){
		return -1;
	}
	if(FD_ISSET(socket_fd, &readfds)){
		return recv(socket_fd, recv_buf, recv_len, 0);
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
		ret_len = send(socket_fd, send_buf + send_ok, send_len - send_ok, 0);
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
	close(socket_fd);
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
 
	if(!strncmp(strp1, "http://", strlen("http://"))){
		strp1 += strlen("http://");
	}else{
		return -1;
	}

	strp2 = strchr(strp1,'/');
	if(strp2){
		len = strlen(strp1) - strlen(strp2);
		memcpy(ota_info->host_name, strp1, len);

		if(*(strp2 + 1)){
			memcpy(ota_info->file_path, strp2 + 1, strlen(strp2) - 1);
		}
	}
	else{
		memcpy(ota_info->host_name, strp1, strlen(strp1));
	}

	strp1 = strchr(ota_info->host_name, ':');
	if(strp1){
		*strp1++ = 0;
		ota_info->host_port = atoi(strp1);
	}
	else{
		ota_info->host_port = HTTP_DEFAULT_PORT;
	}

	file_len = strlen(strp2);
	while(file_len--){
		if(*(strp2 + file_len) == '/'){
			strcpy(ota_info->file_name, strp2 + file_len + 1);
			break;
		}
	}

	return 0;
}

//-----------------------------------------------------------------------
int
joylink_get_file_size(int socket_fd, http_ota_st *ota_info)
{
	char *str_p = NULL;

	long int file_size = 0;
	
	char send_buf[512] = {0};
	char recv_buf[512] = {0};

	int ret_len = 0;
	int recv_len = 0;

	sprintf(send_buf, HTTP_HEAD, ota_info->file_path, ota_info->host_name, ota_info->host_port);
	//printf("send:\n%s\n",send_buf);

	if(joylink_socket_send(socket_fd, send_buf, strlen(send_buf)) < 0){
		printf("get_file_size send failed!\n");
		return -1;
	}

	while(1)
	{
		ret_len = joylink_socket_recv(socket_fd, recv_buf + recv_len, 512 - recv_len);
		if(ret_len < 0){
			printf("get_file_size recv failed!\n");
			return -1;
		}

		recv_len += ret_len;

		if(recv_len > 50){
			if(strstr(recv_buf,"\r\n\r\n") == NULL){
				printf("Parse head tail failed!\n");
				return -1;
			}
			else{
				break;
			}
		}
	}
	//printf("recv:\n%s\n", recv_buf);

	str_p = strstr(recv_buf,"Content-Length");
	if(str_p == NULL){
		printf("http parse file size error!\n");
		return -1;
	}
	else{
		str_p = str_p + strlen("Content-Length") + 2; 
		file_size = atoi(str_p);
	}

        return file_size;
}

//-----------------------------------------------------------------------

int
joylink_ota_get_info(char *url, http_ota_st *ota_info)
{
	int socket_fd = 0;
	
	int file_fd = 0;
	char save_file[128] = {0};

	if(!url){
		printf("url is NULL!\n");
		return -1;
	}

	if(joylink_parse_url(url, ota_info)){
		printf("http_parse_url failed!\n");
		return -1;
	}
	printf("host name: %s port: %d\n", ota_info->host_name, ota_info->host_port);
	printf("file path: %s name: %s\n\n", ota_info->file_path, ota_info->file_name);

	socket_fd = joylink_socket_create(ota_info->host_name, ota_info->host_port);
	if(socket_fd < 0){
		printf("http_tcpclient_create failed\n");
		return -1;
	}

	ota_info->file_size = joylink_get_file_size(socket_fd, ota_info);
	if(ota_info->file_size <= 0){
		printf("get file size failed\n");
		return -1;
	}
	printf("file size %ld\n\n", ota_info->file_size);

	joylink_memory_init((void *)ota_info->file_name, MEMORY_WRITE);
	
	joylink_socket_close(socket_fd);
}

int
joylink_ota_set_download_len(int socket_fd, http_ota_st *ota_info)
{
	long int max_len = ota_info->file_size / 50;
	long int read_len = max_len;

	char *recv_p1 = NULL;
	char *recv_p2 = NULL;

	char send_buf[256] = {0};
	char recv_buf[1024] = {0};

	int recv_len = 0;
	int ret_len = 0;

	long int data_len = 0;

	if(ota_info->file_size - ota_info->file_offset < max_len){
		read_len = ota_info->file_size - ota_info->file_offset;
	}

	sprintf(send_buf, HTTP_GET, ota_info->file_path, ota_info->host_name, ota_info->host_port, ota_info->file_offset, (ota_info->file_offset + read_len - 1));
	//printf("send:\n%s\n",send_buf);

	if(joylink_socket_send(socket_fd, send_buf, strlen(send_buf)) < 0){
		printf("get_file_data send failed!\n");
		return -1;
	}

	memset(recv_buf, 0, 1024);
	while(1){
		ret_len = joylink_socket_recv(socket_fd, recv_buf + recv_len, 1);
		if(ret_len < 0){
			printf("get_file_data recv failed!\n");
			return -1;
		}
		
		recv_len += ret_len;

		if(recv_len < 200) continue;

		recv_p1 = strstr(recv_buf, "\r\n\r\n");
		if(recv_p1 == NULL) continue;

		recv_p2 = strstr(recv_buf, "Content-Length");
		if(recv_p2 == NULL){
			printf("Parse data length error!\n");
			return -1;
		}
		recv_p2 = recv_p2 + strlen("Content-Length") + 2; 
		data_len = atoi(recv_p2);
		break;
	}

	//printf("recv:\n%s\n", recv_buf);
	printf("read_len: %ld data_len: %ld\n", read_len, data_len);

	if(read_len != data_len){
		return -1;
	}

	return read_len;
}

//---------------------------------------------------------------------------
int joylink_ota_report_status(int status, int progress, char *desc);

extern void joylink_util_timer_reset(uint32_t *timestamp);
extern int joylink_util_is_time_out(uint32_t timestamp, uint32_t timeout);

int
joylink_ota_get_data(http_ota_st *ota_info)
{
	int file_fd = 0;
	int socket_fd = 0;
	int download_len = 0;
	int save_len = 0;

	char recv_buf[EVERY_PACKET_LEN] = {0};
	int read_len = 0;
	int ret_len = 0;

	char save_file[128] = {0};

	unsigned int timer_out = 0;
	unsigned int interval_out = OTA_TIME_OUT;

	socket_fd = joylink_socket_create(ota_info->host_name, ota_info->host_port);
	if(socket_fd < 0){
		printf("joylink_socket_create failed\n");
		return -1;
	}

	joylink_util_timer_reset(&timer_out);
	joylink_ota_report_status(OTA_STATUS_DOWNLOAD, 0, "ota download start");

	while(ota_info->file_size > ota_info->file_offset){
		download_len = joylink_ota_set_download_len(socket_fd, ota_info);
		if(download_len < 0){
			printf("set download_len error!\n");
			break;
		}

		read_len = EVERY_PACKET_LEN;
		save_len = download_len;

		while(download_len != 0){
			memset(recv_buf, 0, EVERY_PACKET_LEN);
			ret_len = joylink_socket_recv(socket_fd, recv_buf, read_len);
			if(ret_len > 0){
				joylink_memory_write(0, recv_buf, ret_len);
			}
			else{
				printf("recv file data error!\n");
				break;
			}

			download_len -= ret_len;
			if(download_len / EVERY_PACKET_LEN == 0){
				read_len == download_len % EVERY_PACKET_LEN;
			}
			else{
				read_len = EVERY_PACKET_LEN;
			}
		}

		if(download_len == 0){
			ota_info->file_offset += save_len;
		}
		else{
			break;
		}

		ota_info->finish_percent = ota_info->file_offset * 100 / ota_info->file_size;
		printf("\njoylink OTA download: %d%%\n\n", ota_info->finish_percent);

		if(joylink_util_is_time_out(timer_out, interval_out))
		{
			joylink_ota_report_status(OTA_STATUS_DOWNLOAD, ota_info->finish_percent, "ota timeout error");
			break;
		}
		joylink_ota_report_status(OTA_STATUS_DOWNLOAD, ota_info->finish_percent, "ota download");
	}
	joylink_memory_finish();
	joylink_socket_close(socket_fd);

	if(ota_info->file_size == ota_info->file_offset){
		return 0;
	}
	else{
		return -1;	
	}
}

int joylink_ota_check_crc(unsigned int cloud_crc32, char *path)
{
	int ret = 0;

	int read_len = 0;
	int file_size = 0;

	char buffer[EVERY_PACKET_LEN];

	unsigned int file_crc32 = 0;

	printf("ota check crc!\npath: %s\n", path);

	make_crc32_table();

	ret = joylink_memory_init(path, MEMORY_READ);
	if(ret < 0){
		log_error("open file error");
		return -1;
	}
   
	while(1){
		memset(buffer, 0, sizeof(buffer));
		read_len = joylink_memory_read(0, buffer, sizeof(buffer));
		if(read_len <= 0){
			break;
		}
		file_crc32 = make_crc(file_crc32, buffer, read_len);
		file_size += read_len;		
	}
	joylink_memory_finish();

	printf("cloud crc32: %d, file crc32:%d, file size:%d\n", cloud_crc32, file_crc32, file_size);

	if(cloud_crc32 == file_crc32)
		return 0;
	else
		return -1;
}

//-----------------------------------------------------------------------

static int save_version = 0;

int
joylink_ota_report_status(int status, int progress, char *desc)
{
	JLOtaUpload_t otaUpload;

	strcpy(otaUpload.feedid, _g_pdev->jlp.feedid);
	strcpy(otaUpload.productuuid, _g_pdev->jlp.uuid);

	_g_pdev->jlp.version = 1;
	otaUpload.status = status;

	otaUpload.progress = progress;
	strcpy(otaUpload.status_desc, desc);

	joylink_server_ota_status_upload_req(&otaUpload);
}

static JLOtaOrder_t otaOrder;

//-----------------------------------------------------------------------
void *
joylink_ota_task(void *data)
{
	int ret = 0;
	http_ota_st ota_info;
	//JLOtaOrder_t *otaOrder = (JLOtaOrder_t *)index;

	memset(&otaOrder, 0, sizeof(JLOtaOrder_t));
	memcpy(&otaOrder, data, sizeof(JLOtaOrder_t));
	
	printf("\n\nJoylink ota statrt!\n");

	printf("\nserial:%d | feedid:%s | productuuid:%s | version:%d | versionname:%s | crc32:%d | url:%s\n\n",\
		otaOrder.serial, otaOrder.feedid, otaOrder.productuuid, otaOrder.version,\
		otaOrder.versionname, otaOrder.crc32, otaOrder.url);

	memset(&ota_info, 0, sizeof(http_ota_st));

	save_version = otaOrder.version;

	ret = joylink_ota_get_info(otaOrder.url, &ota_info);
	if(ret < 0){
		joylink_ota_report_status(OTA_STATUS_FAILURE, 0, "get info error");
		printf("Joylink ota get info error!\n");
		return NULL;
	}

	ret = joylink_ota_get_data(&ota_info);
	if(ret < 0){
		joylink_ota_report_status(OTA_STATUS_FAILURE, 0, "get data error");
		printf("Joylink ota get data error!\n");
		return NULL;
	}

	ret = joylink_ota_check_crc(otaOrder.crc32, ota_info.file_name);
	if(ret < 0){
		joylink_ota_report_status(OTA_STATUS_FAILURE, 0, "check crc32 error");
		printf("Joylink ota check crc32 error!\n");
		return NULL;
	}

	joylink_ota_report_status(OTA_STATUS_SUCCESS, ota_info.finish_percent, "ota is ok");

	memset(&ota_info, 0, sizeof(http_ota_st));

	printf("Joylink ota is ok!\n");
}

