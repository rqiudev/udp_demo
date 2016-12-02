#include <stdio.h>
#include "sys_time_correct.h"
#include <time.h>
#include <stdint.h>

int main(int argc, char** argv) {
	if (argc<3) {
		printf ("please input total count, get time type, 1: sys, 2: optimize\n");
		return -1;
	}

	uint64_t total = strtoull(argv[1], 0, NULL);
	int type = strtoul(argv[2], NULL, 0);

#define OSAL_TIME OSAL_get_msel
	
	int64_t sleep_ts = 50000;
	int64_t start_ts = OSAL_TIME(sleep_ts);
	int64_t end_ts =  OSAL_TIME(sleep_ts);

	int64_t test_start_ts = OSAL_TIME(sleep_ts);

	struct timeval tv;
	if (type==1) {
		for(uint64_t i=0; i<total; ++i) {
//			OSAL_TIME(sleep_ts);
			//	    time(NULL);
			gettimeofday(&tv, NULL);
		}
	} else if (type==2) {
		for(uint64_t i=0; i<total; ++i) {
			 OSAL_TIME(sleep_ts);
		}
	}

	int64_t test_end_ts = OSAL_TIME(sleep_ts);
	printf ("ts: %lu(ms)\n", test_end_ts-test_start_ts);
	return 0;

	printf ("not sleep,     cur: %ld, start_ts: %ld, end_ts: %ld, end_ts - start_ts: %d\n", time(NULL), start_ts, end_ts, end_ts-start_ts);
	usleep(1000000);
	end_ts =  OSAL_TIME(sleep_ts);
	printf ("sleep(1000),   cur: %ld, start_ts: %ld, end_ts: %ld, end_ts - start_ts: %d\n", time(NULL), start_ts, end_ts, end_ts-start_ts);

	usleep(2000000);
	end_ts =  OSAL_TIME(sleep_ts);
	printf ("sleep(3000),   cur: %ld, start_ts: %ld, end_ts: %ld, end_ts - start_ts: %d\n", time(NULL), start_ts, end_ts, end_ts-start_ts);
	
	usleep(3000000);
	end_ts =  OSAL_TIME(sleep_ts);
	printf ("sleep(6000),   cur: %ld, start_ts: %ld, end_ts: %ld, end_ts - start_ts: %d\n", time(NULL), start_ts, end_ts, end_ts-start_ts);
	
	usleep(4000000);
	end_ts =  OSAL_TIME(sleep_ts);
	printf ("sleep(10000),  cur: %ld, start_ts: %ld, end_ts: %ld, end_ts - start_ts: %d\n", time(NULL), start_ts, end_ts, end_ts-start_ts);

}


