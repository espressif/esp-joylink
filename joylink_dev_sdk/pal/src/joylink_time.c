#include <stddef.h>
#include <stdint.h>

#include <time.h>
#include <sys/time.h>

#include "joylink_time.h"

/**
 * 初始化系统时间
 *
 * @return: 0 - success or -1 - fail
 *
 */
int32_t jl_set_UTCtime(jl_time_stamp_t time_stamp)
{
    return 0;
}

/**
 * 获取系统时间
 *
 * @out param: time
 * @return: success or fail
 *
 */
int32_t jl_time_get_time(jl_time_t *jl_time)
{
#ifdef __LINUX_PAL__
    time_t timep;
    struct tm *p;
    jl_time->timestamp = (uint32_t) time(&timep);
    p = gmtime(&timep);
    jl_time->year      = p->tm_year;
    jl_time->month     = p->tm_mon;
    jl_time->week      = p->tm_wday;
    jl_time->day       = p->tm_mday;
    jl_time->hour      = p->tm_hour;
    jl_time->minute    = p->tm_min;
    jl_time->second    = p->tm_sec;
#endif
    return 0;
}

/**
* @brief 获取当前系统时间ms
* @param none
* @return time ms
*/
uint32_t jl_time_get_timestamp_ms(jl_time_stamp_t *time_stamp)
{
#ifdef __LINUX_PAL__
    struct timeval now;
    gettimeofday(&now,NULL);
    if(time_stamp)
    {
        time_stamp->second = (uint32_t) now.tv_sec;
        time_stamp->ms = (uint32_t) (now.tv_usec/1000);
    }
    return (uint32_t)(now.tv_sec*1000 + now.tv_usec/1000);
#else
    return 0;
#endif
}

/**
 * 获取系统UTC时间
 *
 * @return: UTC Second
 *
 */
uint32_t jl_time_get_timestamp(jl_time_t *jl_time)
{
#ifdef __LINUX_PAL__
    return (uint32_t)time(NULL);
#else
    return 0;
#endif
}

/**
 * 获取系统时间
 *
 * @out param: "year-month-day hour:minute:second"
 * @return: success or fail
 *
 */
int32_t jl_get_time_str(char *out, int32_t len)
{
    return 0;
}

/**
 * get os time
 *
 * @out param: none
 * @return: sys time ticks ms since sys start
*/
uint32_t jl_get_os_time(void)
{
#ifdef __LINUX_PAL__
        return (uint32_t) jl_time_get_timestamp_ms(NULL); // FIXME do not recommand this method
        // return clock();
#else
        return 0;
#endif
}



