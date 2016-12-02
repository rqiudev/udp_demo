#ifndef osal_sys_time_correct_h_
#define osal_sys_time_correct_h_

/*
 * Copyright (C) 深圳有信网络科技有限公司2016
 * author: andrew.liao
 * describe: 获取系统信息, 对部分系统调用做优化
 * date: 2016-11-22
 */

#include <sys/types.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************/
/*                             system time                              */
/************************************************************************/

/// @获取frequency
typedef enum _frequency_uint {
	frequency_sec,	/// 秒
	frequency_msec, /// 毫秒
	frequency_usec, /// 微秒
}frequency_uint;

double OSAL_get_cpu_frequency(frequency_uint unit);
int64_t OSAL_get_cpu_cycle_count(void);
int64_t OSAL_get_tick_count(void);

typedef struct OSAL_timeval_t
{
	int64_t tv_sec;                  /* seconds */
	int64_t tv_usec;                 /* and microseconds */
}OSAL_timeval;

/// @获取当前时间us级
/// @correct修正时间单位us, 建议为5000ms
/// @如果不需要对时间做优化,设置correct=0即可
int64_t OSAL_get_usel(OSAL_timeval* tv, int64_t correct);

/// @获取当前时间ms级
/// @correct修正时间单位ms, 建议为5000ms
/// @如果不需要对时间做优化,设置correct=0即可
int64_t OSAL_get_msel(int64_t correct );

/// @获取当前时间秒级
/// @correct修正时间单位ms, 建议为5s
/// @如果不需要对时间做优化,设置correct=0即可
int64_t OSAL_get_sel(int64_t correct);
int OSAL_get_cpu_count(void);
int64_t OSAL_cycle_count_revise();

#ifdef __cplusplus
}
#endif

#endif//osal_sys_time_correct_h_
