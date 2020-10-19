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
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "joylink_extern_user.h"
#include "joylink_stdio.h"
#include "joylink_stdint.h"

static FILE *file_fp = NULL;

int joylink_memory_init(void *index, int flag)
{
	char *save_path = (char *)index;

	if(file_fp > 0)	
		return -1;

	//jl_platform_printf("save_path: %s\n", save_path);
	
	if(flag == MEMORY_WRITE){
		file_fp = jl_platform_fopen(save_path, "wb+");
		if(file_fp == NULL){
			jl_platform_printf("Open file error!\n");
			return -1;
		}
	}
	else if(flag == MEMORY_READ){
		file_fp = jl_platform_fopen(save_path, "rb");
		if(file_fp == NULL){
			jl_platform_printf("Open file error!\n");
			return -1;
		}
	}
	return 0;
}

int joylink_memory_write(int offset, char *data, int len)
{
	if(file_fp == NULL || data == NULL)	
		return -1;

	return jl_platform_fwrite(data, 1, len, file_fp);
}

int joylink_memory_read(int offset, char *data, int len)
{
	if(file_fp == NULL || data == NULL)	
		return -1;

	return jl_platform_fread(data, 1, len, file_fp);
}

int joylink_memory_finish(void)
{
	if(file_fp == NULL)
		return -1;

	jl_platform_fclose(file_fp);
	file_fp = NULL;
	return 0;
}



