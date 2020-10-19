#include "joylink_stdio.h"
#include "joylink_thread.h"
#include "joylink_extern.h"
#include "joylink.h"

int 
main()
{
	jl_thread_t task_id; 

	task_id.thread_task = (threadtask) joylink_main_start;
	task_id.stackSize = 0x5000;
    task_id.priority = JL_THREAD_PRI_DEFAULT;
    task_id.parameter = NULL;
    jl_platform_thread_start(&task_id);

//   joylink_softap_start();

    jl_platform_msleep(2000);
    // 开启局域网激活模式
    joylink_dev_lan_active_switch(1);

    while(1)
    {
        jl_platform_msleep(1000);
    }
    return 0;
}
