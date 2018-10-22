/* --------------------------------------------------
 * @file: joylink_extern_tool.C
 *
 * @brief: 
 *
 * @version: 2.0
 *
 * @date: 2018/07/26 PM
 *
 * --------------------------------------------------
 */

#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "joylink_extern_user.h"

int file_fd = -1;

int
joylink_memory_init(void *index, int flag)
{
	char *save_path = (char *)index;

	if(file_fd > 0)	
		return -1;

	//printf("save_path: %s\n", save_path);
	
	if(flag == MEMORY_WRITE){
		file_fd = open(save_path, O_WRONLY | O_CREAT | O_TRUNC, 0777);
		if(file_fd < 0){
			printf("Open file error!\n");
			return -1;
		}
	}
	else if(flag == MEMORY_READ){
		file_fd = open(save_path, O_RDONLY | O_CREAT);
		if(file_fd < 0){
			printf("Open file error!\n");
			return -1;
		}
	}
	return 0;
}

int
joylink_memory_write(int offset, char *data, int len)
{
	if(file_fd < 0 || data == NULL)	
		return -1;

	return write(file_fd, data, len);
}

int
joylink_memory_read(int offset, char *data, int len)
{
	if(file_fd < 0 || data == NULL)	
		return -1;

	return read(file_fd, data, len);
}

int
joylink_memory_finish(void)
{
	if(file_fd < 0)
		return -1;

	close(file_fd);
	file_fd = -1;
	return 0;
}



