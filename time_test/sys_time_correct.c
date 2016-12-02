#include <sys/time.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "sys_time_correct.h"
#include <stdio.h>

double OSAL_get_cpu_frequency(frequency_uint unit) {
	static double fq_sec = 0;
	static double fq_msec = 0;
	static double fq_usec = 0;

	if (unit==frequency_sec && fq_sec!=0)
		return fq_sec;
	else if ( unit==frequency_msec && 0!=fq_msec)
		return fq_msec;
	else if (unit==frequency_usec && 0!=fq_usec)
		return fq_usec;

	/// 以下代码只适合于linux服务器(笔记本或者具有节能功能的cpu中的虚拟机linux可能误差较大)
#if defined(__linux__)
	FILE* file = NULL;
	char file_cmd[260];
	snprintf(file_cmd, sizeof(file_cmd), "cat /proc/cpuinfo");

	file = popen(file_cmd, "r");
	if (!file) {
		return 0;
	}

	char buf[1024]= { 0 };
	while (fgets(buf, 1024, file)) {
		if (0==strlen(buf))
			continue;

		if (buf[0]=='b' || buf[0]=='B') {
			uint32_t k = 0;
			for (; k<strlen("bogomips"); ++k) {
				if (buf[k]>='A' && buf[k]<='Z')
					buf[k] += 32;
			}

			if ( 0==strncmp(buf, "bogomips", strlen("bogomips"))) {
				char* ppp = strstr(buf, ":");
				if (ppp) {
					/// xxx,172.16.12.28开发环境cpu MHz与实际不一至
					fq_sec = (int64_t)(atof(ppp+1)/2*1000000.0 + 0.5);
					fq_msec = (int64_t)(atof(ppp+1)/2*1000.0 + 0.5);
					fq_usec = (int64_t)(atof(ppp+1)/2 + 0.5);
				}
				break;
			}
		}
	}

	pclose(file);
	
#else
#error "os nonsupport"
#endif
	if (unit==frequency_sec && fq_sec!=0)
		return fq_sec;
	else if ( unit==frequency_msec && 0!=fq_msec)
		return fq_msec;
	else if (unit==frequency_usec && 0!=fq_usec)
		return fq_usec;
}

int64_t OSAL_get_cpu_cycle_count(void)
{	
#if defined(__linux__) || defined(__LINUX__)
#ifdef __x86_64__
	uint32_t  u_hieght=0;
	uint32_t  u_low=0;
	__asm__ __volatile__(
		".byte 0x0f, 0x31 \r\t"
		:"=a"(u_low),"=d"(u_hieght));
	return  (int64_t)((uint64_t)u_hieght << 32) | ((uint64_t)u_low);
#else
	int64_t ret = 0;
	__asm__ __volatile__(
		".byte 0x0f, 0x31 \r\t"
		:"=A"(ret));
	return ret;
#endif
#else
#error "os nonsupport"
#endif
}

int64_t OSAL_cycle_count_revise() {
	static int64_t cy = 0;
	if (cy > 0)
		return cy;

	cy = OSAL_get_cpu_cycle_count();
	cy = OSAL_get_cpu_cycle_count() - cy;
	return cy;
}

int64_t OSAL_get_tick_count() {
	static double fq = 0;
	if (fq ==0 )
		fq = (double)OSAL_get_cpu_frequency(frequency_msec);

	double tick = (double)OSAL_get_cpu_cycle_count();

	return (int64_t)((tick/fq));	
}

static int64_t OSAL_get_tick_us() {
	static double fq = 0;
	if (fq ==0 )
		fq = (double)OSAL_get_cpu_frequency(frequency_usec);

	double tick = (double)OSAL_get_cpu_cycle_count();

	return (int64_t)((tick/fq));	
}

int64_t OSAL_get_usel(OSAL_timeval* tv, int64_t correct) {
	static int64_t prev_ts = 0;
	static int64_t cur_ts = 0;
	static int64_t prev_tick = 0;

	/// 最多只能修正120s
//	if (correct>1200000000)
//		correct = 1500000000;

	if (cur_ts == 0) {
		struct timeval tv;
		gettimeofday(&tv, NULL);
		cur_ts = (uint64_t)tv.tv_sec*1000000 + tv.tv_usec;
		prev_ts = cur_ts;
		prev_tick = OSAL_get_tick_us();
	} 

	/// 修正时间
	int64_t cur_tick = OSAL_get_tick_us();
	if (cur_tick>prev_tick) {
		cur_ts += cur_tick-prev_tick;
		prev_tick = cur_tick;
	}

	if (cur_ts > prev_ts+correct) {
		struct timeval tv;
		gettimeofday(&tv, NULL);
		cur_ts = (uint64_t)tv.tv_sec*1000000 + (tv.tv_usec);
		prev_ts = cur_ts;
		prev_tick = OSAL_get_tick_us();
	}

	if (tv) {
		tv->tv_sec = cur_ts/1000000;
		tv->tv_usec = cur_ts%1000000;
	}

	return cur_ts;
}

int64_t OSAL_get_msel(int64_t correct) {
#if 0
	static int64_t prev_ts = 0;
	static int64_t cur_ts = 0;
	static int64_t prev_tick = 0;

	/// 最多只能修正15s
	if (correct>15000)
		correct = 15000;
	
	if (cur_ts == 0) {
		struct timeval tv;
		gettimeofday(&tv, NULL);
		cur_ts = (uint64_t)tv.tv_sec*1000 + (tv.tv_usec)/1000;
		prev_ts = cur_ts;
		prev_tick = OSAL_get_tick_count();
	} 

	/// 修正时间
	int64_t cur_tick = OSAL_get_tick_count();
	if (cur_tick>prev_tick) {
		cur_ts += cur_tick-prev_tick;
		prev_tick = cur_tick;
	}

	if (cur_ts > prev_ts+correct) {
		struct timeval tv;
		gettimeofday(&tv, NULL);
		cur_ts = (uint64_t)tv.tv_sec*1000 + (tv.tv_usec)/1000;
		prev_ts = cur_ts;
		prev_tick = OSAL_get_tick_count();
	}

	return cur_ts;
#else
	OSAL_timeval tv;
	int64_t ts = OSAL_get_usel(&tv, correct*1000);
	return ts/1000;
#endif
}

int64_t OSAL_get_sel(int64_t correct) {
#if 0
	static int64_t prev_ts = 0;
	static int64_t cur_ts = 0;
	static int64_t prev_tick = 0;

	/// 最多只能修正15s
	if (correct>15)
		correct = 15;


	if (cur_ts == 0) {
		struct timeval tv;
		gettimeofday(&tv, NULL);
		cur_ts = (uint64_t)tv.tv_sec + (tv.tv_usec)/1000000;
		prev_ts = cur_ts;
		prev_tick = OSAL_get_tick_count()/1000;
	} 

	/// 修正时间
	int64_t cur_tick = OSAL_get_tick_count()/1000;
	if (cur_tick>prev_tick) {
		cur_ts += cur_tick-prev_tick;
		prev_tick = cur_tick;
	}

	if (cur_ts > prev_ts+correct) {
		struct timeval tv;
		gettimeofday(&tv, NULL);
		cur_ts = (uint64_t)tv.tv_sec + (tv.tv_usec)/1000000;
		prev_ts = cur_ts;
		prev_tick = OSAL_get_tick_count()/1000;
	}

	return cur_ts;
#else
	OSAL_timeval tv;
	int64_t ts = OSAL_get_usel(&tv, correct*1000000);
	return ts/1000000;
#endif
}

int OSAL_get_cpu_count() {
	static int sCount = 0;
	if(0 == sCount) {
		/// armƽ̨Ϊ: Processor
		const char* const PROCESSOR_VAR_NAME = "processor";

		size_t len = 0;
		FILE* f = fopen("/proc/cpuinfo", "r");
		if(NULL == f) {
			return 1;
		}

		char line[1024];
		while(fgets(line, len, f)) {
			uint32_t k = 0;
			for (; k<strlen("processor"); ++k) {
				if (line[k]>='A' && line[k]<='Z')
					line[k] += 32;
			}

			if(0 == strncmp(line, PROCESSOR_VAR_NAME, 9))
				++sCount;
		}

		fclose(f);
	}

	return sCount;
}


