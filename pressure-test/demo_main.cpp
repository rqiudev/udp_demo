#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <pthread.h>
#include <assert.h>
#include <poll.h>
#include "demo_adjure.h"
#include <string.h>
#include <unistd.h>  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <map>




StSessionInfo g_sessionInfoSet[MAX_SESSION_NUM];
std::map<std::string, StRtppStat> g_rtppStatMap;
int session_num = 1;
int send_interval = 20;
int rtp_data_size = 20;
int send_pkg_num = 0x7fffffff;
int info_level=2;
int delay_limit = 400;
double watch_ratio_limit = 0.1;
int enables_send_all = 0;
int ping_times = 0;


long getmstime(void)
{
    struct timeval timev;
    gettimeofday(&timev, 0);
	uint64_t tmp = timev.tv_sec;
	tmp = tmp*1000 + timev.tv_usec/1000;
    return tmp;
}

long getustime() {
	struct timeval timev; 
	gettimeofday(&timev, 0);
	uint64_t tmp = timev.tv_sec;
	tmp = tmp*1000000 + timev.tv_usec;
	return tmp;
}

class CRtpPacket {
public:
	CRtpPacket();
	CRtpPacket(unsigned int pkgIdx);
	~CRtpPacket();

public:
	int makeup_rtp_packet(char* buff, int* buffLen, int contentLen);

	unsigned short _sequenceNumber;
	unsigned int   _timeStamp;
	unsigned int   _ssrc;
};


CRtpPacket::CRtpPacket()
	: _sequenceNumber(0)
	, _timeStamp(0)
	, _ssrc(0)
{
}

CRtpPacket::CRtpPacket(unsigned int pkgIdx)
	: _sequenceNumber(pkgIdx)
	, _timeStamp(0)
	, _ssrc(pkgIdx)
{
	//_timeStamp = getustime();
}

CRtpPacket::~CRtpPacket()
{
}

int CRtpPacket::makeup_rtp_packet(char* buff, int* buffLen, int contentLen) {

	char payloadType = static_cast<char>(0x12); /// G729
	buff[0] = static_cast<char>(0x80);            // version 2
	buff[1] = static_cast<char>(payloadType);
	buff[1] |= 0x80; // kRtpMarkerBitMask;  // MarkerBit is set

	*((unsigned short*)(buff+2)) = htons(_sequenceNumber);
	*((unsigned int*)(buff+4)) = htonl(_timeStamp);
	*((unsigned int*)(buff+8)) = htonl(_ssrc);
	*buffLen =  contentLen+12;
	memset(buff+12, 'c', contentLen);

	return 0;
}

int get_random_ip(char* random_ip) {
	if (random_ip == NULL) return -1;
	struct in_addr ip;
	ip.s_addr = (rand()%255 |
		(rand()%255 << 8) |
		(rand()%255 << 16) |
		(rand()%255 << 24));
	strncpy(random_ip, inet_ntoa(ip), 16);
	return 0;
}

int sock_ip_valid(const char* szIp, uint32_t* ip /* = NULL */) {
	int nFieldCount = 0;
	char ch;
	char chField[4];
	char ch255[4] = {'2','5','5','\0'};
	int i=0, j=0;
	unsigned char _ip[4];
	chField[3] = 0;
	const char* lpszIP = szIp;

	if(ip)
		*ip = INADDR_NONE;

	for(;;) {
		ch = lpszIP[i];
		if((ch < '0' || ch > '9') && ch != '.' && ch != 0)
			return 0;

		if(ch && ch != '.') {
			if(j >= 3 || i >= 15)
				return 0;

			chField[j++] = lpszIP[i++];
		} else {
			if(j == 0)
				return 0;

			if(j == 2) {
				chField[2] = chField[1];
				chField[1] = chField[0];
				chField[0] = '0';
			} else if(j == 1) {
				chField[2] = chField[0];
				chField[1] = '0';
				chField[0] = '0';
			}

			j = 0;
			if(strcmp(chField, ch255) > 0)
				return 0;

			if(ip)
				_ip[nFieldCount] = (unsigned char )((chField[0]-'0') * 100 + (chField[1]-'0') * 10 + (chField[2]-'0'));

			nFieldCount++;

			if(lpszIP[i] == 0)
				break;

			if(nFieldCount == 4)
				return 0;

			i++;
		}
	}

	if(nFieldCount != 4)
		return 0;

	if(ip)
		*ip = *(uint32_t*)_ip;

	return 1;
}

bool set_nonblock_socket(int fd) {
	int flags;
	if ((flags = fcntl(fd, F_GETFL)) == -1) {
		fprintf(stderr, "set nonblock failed, errno: %d, error msg: %s\n", errno, strerror(errno));
		return false;
	}

	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
		fprintf(stderr, "set nonblock failed, errno: %d, error:%d msg: %s\n", fd, errno, strerror(errno));
		return false;
	}

	return true;
}

void* caller_thread_callback(void* lparam) {
	char send_buf[MAX_BUF_SIZE];

	long prev_us = 0;
	int pkgIdx = 0;
	int send_buf_size;
	CRtpPacket pack_obj(pkgIdx);

	while (true && pkgIdx < send_pkg_num) {
		long cur_us = getustime();
		if (cur_us-prev_us<1000*send_interval) {
			usleep(1000*send_interval - (cur_us-prev_us));
			continue;
		}

		prev_us = cur_us;
		for (int sessionIdx = 0; sessionIdx < session_num; ++sessionIdx)
		{
			pack_obj._timeStamp = getmstime();
			pack_obj.makeup_rtp_packet(send_buf, &send_buf_size, rtp_data_size);
			struct sockaddr_in sock_addr;
			bzero(&sock_addr, sizeof(sock_addr));
			sock_addr.sin_family = AF_INET;
			sock_addr.sin_port = htons(g_sessionInfoSet[sessionIdx].rtppPort[0]);
			sock_addr.sin_addr.s_addr = inet_addr(g_sessionInfoSet[sessionIdx].rtppCallerIp);
			int sendRet = sendto(g_sessionInfoSet[sessionIdx].callerFd, send_buf, send_buf_size,
					0, (struct sockaddr*)&sock_addr, sizeof(struct sockaddr));
			if (sendRet < 0) 	{
				printf("[%s : %d]: UDP sending fail, ip = 0x%x port = %d, error(%s)\n",
						__FILE__, __LINE__, sock_addr.sin_addr.s_addr, sock_addr.sin_port, strerror(errno));
				return 0;
			}

			if (send_buf_size != sendRet)
			{
				printf("[%s : %d]: UDP send fail, len = %u not send\n", __FILE__, __LINE__, send_buf_size - sendRet);
				return 0;
			}

			//printf("=====send package from caller at port:%d to rtpp, len:%d, packageIdx:%d, send_time:%u\n",
			//		g_sessionInfoSet[sessionIdx].rtppPort[0], send_buf_size, pkgIdx, pack_obj._timeStamp);

			pack_obj._timeStamp = getmstime();
			pack_obj.makeup_rtp_packet(send_buf, &send_buf_size, rtp_data_size);
			bzero(&sock_addr, sizeof(sock_addr));
			sock_addr.sin_family = AF_INET;
			sock_addr.sin_port = htons(g_sessionInfoSet[sessionIdx].rtppPort[1]);
			sock_addr.sin_addr.s_addr = inet_addr(g_sessionInfoSet[sessionIdx].rtppCalleeIp);
			sendRet = sendto(g_sessionInfoSet[sessionIdx].calleeFd, send_buf, send_buf_size, 0, (struct sockaddr*)&sock_addr, sizeof(struct sockaddr));
			if (sendRet < 0)
			{
				printf("[%s : %d]: UDP sending fail, ip = 0x%x port = %d, error(%s)\n",
						__FILE__, __LINE__, sock_addr.sin_addr.s_addr, sock_addr.sin_port, strerror(errno));
				return 0;
			}

			if (send_buf_size != sendRet)
			{
				printf("[%s : %d]: UDP send fail, len = %u not send\n", __FILE__, __LINE__, send_buf_size - sendRet);
				return 0;
			}

			//printf("=====send package from callee at port:%d to rtpp, len:%d, packageIdx:%d, send_time:%u\n",
			//		g_sessionInfoSet[sessionIdx].rtppPort[1], send_buf_size, pkgIdx, pack_obj._timeStamp);
		}
		pkgIdx++;
	}

	pthread_exit(NULL);

	return 0;
}

void session_stat(int sessionIdx, int direct, unsigned elapsed) {
	if (elapsed < 0 || direct < 0 || direct > 1) return;

	if (info_level < 1) return;


	std::string rtppStr = g_sessionInfoSet[sessionIdx].rtppCallerIp;
	if (strncmp(g_sessionInfoSet[sessionIdx].rtppCallerIp, g_sessionInfoSet[sessionIdx].rtppCalleeIp, 16)) {
		rtppStr = std::string("&") + std::string(g_sessionInfoSet[sessionIdx].rtppCalleeIp);
	}

	StRtppStat rtppStat = g_rtppStatMap[rtppStr];
	if (rtppStat.sessionStat[direct].max < elapsed) {
		rtppStat.sessionStat[direct].max = elapsed;
	}
	if (rtppStat.sessionStat[direct].min > elapsed) {
		rtppStat.sessionStat[direct].min = elapsed;
	}


	rtppStat.sessionStat[direct].count++;
	if (elapsed > delay_limit) {
		rtppStat.sessionStat[direct].countLimit++;
	}

	int delayIdx = -1;
	if (elapsed < 25.0)
	{
		delayIdx = 0;
	}
	else if (elapsed < 50.0)
	{
		delayIdx = 1;
	}
	else if (elapsed < 100.0)
	{
		delayIdx = 2;
	}
	else if (elapsed < 400.0)
	{
		delayIdx = 3;
	}
	else
	{
		delayIdx = 4;
	}
	rtppStat.sessionStat[direct].delayStat[delayIdx]++;

	rtppStat.sessionStat[direct].total += elapsed;
	rtppStat.sessionStat[direct].average = double(rtppStat.sessionStat[direct].total) / rtppStat.sessionStat[direct].count;
	g_rtppStatMap[rtppStr] = rtppStat;

	if (info_level < 2) return;

	if (g_sessionInfoSet[sessionIdx].sessionStat[direct].max < elapsed) {
		g_sessionInfoSet[sessionIdx].sessionStat[direct].max = elapsed;
	}
	if (g_sessionInfoSet[sessionIdx].sessionStat[direct].min > elapsed) {
		g_sessionInfoSet[sessionIdx].sessionStat[direct].min = elapsed;
	}

	g_sessionInfoSet[sessionIdx].sessionStat[direct].count++;
	if (elapsed > delay_limit) {
		g_sessionInfoSet[sessionIdx].sessionStat[direct].countLimit++;
	}
	g_sessionInfoSet[sessionIdx].sessionStat[direct].delayStat[delayIdx]++;

	g_sessionInfoSet[sessionIdx].sessionStat[direct].total += elapsed;
	g_sessionInfoSet[sessionIdx].sessionStat[direct].average =
			double(g_sessionInfoSet[sessionIdx].sessionStat[direct].total) / g_sessionInfoSet[sessionIdx].sessionStat[direct].count;
}

void print_session_stat() {
	time_t g_now = time(NULL);
	struct tm tm_now;
	localtime_r(&g_now, &tm_now);

	if (info_level < 1) return;

	std::string rtppStatStr = "";
	char curbuf[1024];
	if (info_level > 1)
	{
		sprintf(curbuf, "" \
				"{\"topic\":\"uxin_session_monitor\"," \
					"\"content\":{" \
						"\"required\":{\"imei\": \"\", \"version\": \"unknow\",\"os\":\"unknow\"}," \
						"\"tags\": [");
		rtppStatStr  += std::string(curbuf);

		printf("\n====================================session stat(%d): %04d-%02d-%02d %02d:%02d:%02d===================================================================================\n"
				, session_num, tm_now.tm_year+1900, tm_now.tm_mon+1, tm_now.tm_mday, tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);
		printf("  idx |        caller   |        callee   |        rtpp1    |        rtpp2    |" \
				" min0  | max0  |    ave0    | count0 | limitRate0 |     delayStat0    |" \
				" min1  |  max1 |    ave1    | count1 | limitRate1 |     delayStat1    |\n");
		StSessionInfo* pSessionInfo;
		for (int idx=0; idx<session_num; ++idx) {
			pSessionInfo = &(g_sessionInfoSet[idx]);
			double ratio_limit0 = double(pSessionInfo->sessionStat[0].countLimit) / pSessionInfo->sessionStat[0].count;
			double ratio_limit1 = double(pSessionInfo->sessionStat[1].countLimit) / pSessionInfo->sessionStat[1].count;
			printf("%5d | %15s | %15s | %15s |  %15s |" \
					" %5u | %5u | %5.2f | %5u | %5f |  %u %u %u %u %u  |" \
					" %5u | %5u | %5.2f | %5u | %5f |  %u %u %u %u %u  | \n"
					, pSessionInfo->sessionIdx, pSessionInfo->callerIp, pSessionInfo->calleeIp,pSessionInfo->rtppCallerIp, pSessionInfo->rtppCalleeIp
					, pSessionInfo->sessionStat[0].min, pSessionInfo->sessionStat[0].max, pSessionInfo->sessionStat[0].average
					, pSessionInfo->sessionStat[0].count, ratio_limit0
					, pSessionInfo->sessionStat[0].delayStat[0], pSessionInfo->sessionStat[0].delayStat[1], pSessionInfo->sessionStat[0].delayStat[2]
					, pSessionInfo->sessionStat[0].delayStat[3],pSessionInfo->sessionStat[0].delayStat[4]
					, pSessionInfo->sessionStat[1].min, pSessionInfo->sessionStat[1].max, pSessionInfo->sessionStat[1].average
					, pSessionInfo->sessionStat[1].count, ratio_limit1
					, pSessionInfo->sessionStat[1].delayStat[0], pSessionInfo->sessionStat[1].delayStat[1], pSessionInfo->sessionStat[1].delayStat[2]
					, pSessionInfo->sessionStat[1].delayStat[3],pSessionInfo->sessionStat[1].delayStat[4]
					);
			sprintf(curbuf,
					"{\"idx\": %d, \"caller\": \"%s\",\"callee\": \"%s\"," \
					" \"rtpp1\": \"%s\",\"rtpp2\": \"%s\", " \
					" \"min0\":%u, \"max0\":%u, \"ave0\":%.2f, " \
					" \"count0\":%u, \"limitRate0\":%f, " \
					" \"delayStat0\":[%u, %u, %u, %u, %u], " \
					" \"min1\":%u, \"max1\":%u, \"ave1\":%.2f, " \
					" \"count1\":%u, \"limitRate1\":%f, " \
					" \"delayStat1\":[%u, %u, %u, %u, %u]" \
					"}"
					, pSessionInfo->sessionIdx, pSessionInfo->callerIp, pSessionInfo->calleeIp
					, pSessionInfo->rtppCallerIp,	pSessionInfo->rtppCalleeIp
					, pSessionInfo->sessionStat[0].min, pSessionInfo->sessionStat[0].max, pSessionInfo->sessionStat[0].average
					, pSessionInfo->sessionStat[0].count, ratio_limit0
					, pSessionInfo->sessionStat[0].delayStat[0], pSessionInfo->sessionStat[0].delayStat[1], pSessionInfo->sessionStat[0].delayStat[2]
					, pSessionInfo->sessionStat[0].delayStat[3],pSessionInfo->sessionStat[0].delayStat[4]
					, pSessionInfo->sessionStat[1].min, pSessionInfo->sessionStat[1].max, pSessionInfo->sessionStat[1].average
					, pSessionInfo->sessionStat[1].count, ratio_limit1
					, pSessionInfo->sessionStat[1].delayStat[0], pSessionInfo->sessionStat[1].delayStat[1], pSessionInfo->sessionStat[1].delayStat[2]
					, pSessionInfo->sessionStat[1].delayStat[3],pSessionInfo->sessionStat[1].delayStat[4]
					);
			rtppStatStr  += std::string(curbuf);
			if (idx != session_num-1) {
				rtppStatStr  += std::string(",");
			}
		}
		printf("  idx |        caller   |        callee   |        rtpp1    |        rtpp2    |" \
				" min0  | max0  |    ave0    | count0 | limitRate0 |     delayStat0    |" \
				" min1  |  max1 |    ave1    | count1 | limitRate1 |     delayStat1    |\n");

		sprintf(curbuf, "]," \
						"\"session_num\": %d," \
						"\"time\": \"%04d-%02d-%02d %02d:%02d:%02d\" " \
					"}" \
				"}", session_num,
				tm_now.tm_year+1900, tm_now.tm_mon+1, tm_now.tm_mday, tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec
		);
		rtppStatStr  += std::string(curbuf);
		printf("\n%s\n", rtppStatStr.c_str());
	}


	rtppStatStr = "";
	sprintf(curbuf, "" \
			"{\"topic\":\"uxin_rtpp_old_monitor\"," \
				"\"content\":{" \
					"\"required\":{\"imei\": \"\", \"version\": \"unknow\",\"os\":\"unknow\"}," \
					"\"rtpp_num\": %d, \"session_num\": %d," \
					"\"tags\": ["
					, g_rtppStatMap.size(), session_num);
	rtppStatStr  += std::string(curbuf);


	std::string rtppStatOldStr="";
	std::string rtppStatNewStr="";
	sprintf(curbuf, "\n====================================rtpp stat(%d, %d): %04d-%02d-%02d %02d:%02d:%02d====================================================================================\n"
			,g_rtppStatMap.size(), session_num, tm_now.tm_year+1900, tm_now.tm_mon+1, tm_now.tm_mday, tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);
	rtppStatOldStr  += std::string(curbuf);
	sprintf(curbuf, "               rtpp               | rnum | snum |  rtt  |" \
			"  min0 |  max0 |    ave0      | count0 | limitRate0 |     delayStat0    |" \
			"  min1 |  max1 |    ave1      | count1 | limitRate1 |     delayStat1    |\n");
	rtppStatOldStr  += std::string(curbuf);



	std::string outStr = "";
	int send_rtpp_alert = 0;
	int alert_rtpp_num = 0;
	int first_index = 0;
	int map_size = g_rtppStatMap.size();
	FILE *fstream=NULL;
	char popenBuf[16];
	char popenStr[64];
	for (std::map<std::string, StRtppStat>::iterator iter = g_rtppStatMap.begin(); iter != g_rtppStatMap.end(); ++iter) {
		StRtppStat rtppStat = iter->second;
		double ratio_limit0 = double(rtppStat.sessionStat[0].countLimit) / rtppStat.sessionStat[0].count;
		double ratio_limit1 = double(rtppStat.sessionStat[1].countLimit) / rtppStat.sessionStat[1].count;
		double pingRtt = 0.0;
		if (ping_times > 0 && rtppStat.rtppNum == 1) {
			do {
				sprintf(popenStr, "ping -c %d %s | grep 'rtt' | awk -F'/' '{print $5}'", ping_times, iter->first.c_str());
				if(NULL==(fstream=popen(popenStr, "r"))) break;
				memset(popenBuf, 0, sizeof(popenBuf));
				if(NULL==fgets(popenBuf, sizeof(popenBuf), fstream)) break;
			    pingRtt=atof(popenBuf);
			} while(0);
			fclose(fstream);
		}

		sprintf(curbuf, " %32s | %4d | %4d | %5.2f | " \
				" %5u | %5u | %5.2f | %5u | %5.2f |  %u %u %u %u %u  |" \
				" %5u | %5u | %5.2f | %5u | %5.2f |  %u %u %u %u %u  | \n"
				, iter->first.c_str(), rtppStat.rtppNum, rtppStat.sessionNum, pingRtt
				, rtppStat.sessionStat[0].min, rtppStat.sessionStat[0].max
				, rtppStat.sessionStat[0].average, rtppStat.sessionStat[0].count, ratio_limit0
				, rtppStat.sessionStat[0].delayStat[0], rtppStat.sessionStat[0].delayStat[1], rtppStat.sessionStat[0].delayStat[2]
				, rtppStat.sessionStat[0].delayStat[3],rtppStat.sessionStat[0].delayStat[4]
			    , rtppStat.sessionStat[1].min, rtppStat.sessionStat[1].max
				, rtppStat.sessionStat[1].average, rtppStat.sessionStat[1].count, ratio_limit1
				, rtppStat.sessionStat[1].delayStat[0], rtppStat.sessionStat[1].delayStat[1], rtppStat.sessionStat[1].delayStat[2]
				, rtppStat.sessionStat[1].delayStat[3],rtppStat.sessionStat[1].delayStat[4]
				);
		rtppStatOldStr  += std::string(curbuf);

		sprintf(curbuf,
				"{\"rtpp\": \"%s\", \"rnum\": %d, \"snum\": %d, \"rtt\":%.2f," \
				" \"min0\":%u, \"max0\":%u, \"ave0\":%.2f, \"count0\":%u, \"limitRate0\":%f, " \
				" \"delayStat0\":[%u, %u, %u, %u, %u], " \
				" \"min1\":%u, \"max1\":%u, \"ave1\":%.2f, \"count1\":%u, \"limitRate1\":%f, " \
				" \"delayStat1\":[%u, %u, %u, %u, %u]" \
				"}"
				, iter->first.c_str(), rtppStat.rtppNum, rtppStat.sessionNum, pingRtt
				, rtppStat.sessionStat[0].min, rtppStat.sessionStat[0].max
				, rtppStat.sessionStat[0].average, rtppStat.sessionStat[0].count, ratio_limit0
				, rtppStat.sessionStat[0].delayStat[0], rtppStat.sessionStat[0].delayStat[1], rtppStat.sessionStat[0].delayStat[2]
				, rtppStat.sessionStat[0].delayStat[3],rtppStat.sessionStat[0].delayStat[4]
			    , rtppStat.sessionStat[1].min, rtppStat.sessionStat[1].max
				, rtppStat.sessionStat[1].average, rtppStat.sessionStat[1].count, ratio_limit1
				, rtppStat.sessionStat[1].delayStat[0], rtppStat.sessionStat[1].delayStat[1], rtppStat.sessionStat[1].delayStat[2]
				, rtppStat.sessionStat[1].delayStat[3],rtppStat.sessionStat[1].delayStat[4]
				);

		rtppStatStr  += std::string(curbuf);
		first_index++;
		if (first_index != map_size) {
			rtppStatStr  += std::string(",");
		}

		sprintf(curbuf, "{\"topic\":\"uxin_rtpp_monitor\"," \
				"\"content\":{" \
					"\"rtpp\": \"%s\", \"rnum\": %d, \"snum\": %d, \"rtt\":%.2f, " \
					" \"min0\":%u, \"max0\":%u, \"ave0\":%.2f, \"count0\":%u, \"limitRate0\":%f, " \
					" \"delayStat0\":[%u, %u, %u, %u, %u], " \
					" \"min1\":%u, \"max1\":%u, \"ave1\":%.2f, \"count1\":%u, \"limitRate1\":%f, " \
					" \"delayStat1\":[%u, %u, %u, %u, %u], " \
					"\"time\": \"%04d-%02d-%02d %02d:%02d:%02d\" " \
					"}" \
				"}\n"
				, iter->first.c_str(), rtppStat.rtppNum, rtppStat.sessionNum, pingRtt
				, rtppStat.sessionStat[0].min, rtppStat.sessionStat[0].max
				, rtppStat.sessionStat[0].average, rtppStat.sessionStat[0].count, ratio_limit0
				, rtppStat.sessionStat[0].delayStat[0], rtppStat.sessionStat[0].delayStat[1], rtppStat.sessionStat[0].delayStat[2]
				, rtppStat.sessionStat[0].delayStat[3],rtppStat.sessionStat[0].delayStat[4]
			    , rtppStat.sessionStat[1].min, rtppStat.sessionStat[1].max
				, rtppStat.sessionStat[1].average, rtppStat.sessionStat[1].count, ratio_limit1
				, rtppStat.sessionStat[1].delayStat[0], rtppStat.sessionStat[1].delayStat[1], rtppStat.sessionStat[1].delayStat[2]
				, rtppStat.sessionStat[1].delayStat[3],rtppStat.sessionStat[1].delayStat[4]
			    , tm_now.tm_year+1900, tm_now.tm_mon+1, tm_now.tm_mday, tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec
				);
		rtppStatNewStr  += std::string(curbuf);

//		if (enables_send_all || ratio_limit0 > watch_ratio_limit || ratio_limit1 > watch_ratio_limit)
//		{
//			sprintf(curbuf, "RTPP-%s-%lu-%lu-%.2f-%lu-%f-%lu-%lu-%.2f-%lu-%f#", iter->first.c_str(),
//					rtppStat.sessionStat[0].min, rtppStat.sessionStat[0].max,
//					rtppStat.sessionStat[0].average, rtppStat.sessionStat[0].count, ratio_limit0,
//					rtppStat.sessionStat[1].min, rtppStat.sessionStat[1].max,
//					rtppStat.sessionStat[1].average, rtppStat.sessionStat[1].count, ratio_limit1);
//			outStr  += std::string(curbuf);
//			send_rtpp_alert = 1;
//			alert_rtpp_num++;
//		}
	}
	sprintf(curbuf, "               rtpp               | rnum | snum |  rtt  |" \
			"  min0 |  max0 |    ave0      | count0 | limitRate0 |     delayStat0    |" \
			"  min1 |  max1 |    ave1      | count1 | limitRate1 |     delayStat1    |\n");
	rtppStatOldStr  += std::string(curbuf);

	sprintf(curbuf, "]," \
					"\"time\": \"%04d-%02d-%02d %02d:%02d:%02d\" " \
				"}" \
			"}",
			tm_now.tm_year+1900, tm_now.tm_mon+1, tm_now.tm_mday, tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec
	);
	rtppStatStr  += std::string(curbuf);

	printf("\n%s\n", rtppStatNewStr.c_str());
	printf("\n%s\n", rtppStatOldStr.c_str());
	printf("\n%s\n", rtppStatStr.c_str());


//	if (send_rtpp_alert == 1) {
//		sprintf(curbuf, "RTPPALERT#NUM:%d#CONTENT:", alert_rtpp_num);
//		outStr = std::string(curbuf) + outStr;
//		outStr[1600] = '\0';
//		//curl支持最大1800个字符
//		char curlBuf[1801];
//		snprintf(curlBuf, sizeof(curlBuf), "curl \"http://123.56.19.101:5000/mail?secret=fskd2endprx&email=vic.qiu@uxin.com&subject=RTPP_ALERT&content=%s\"", outStr.c_str());
//		system(curlBuf);
//		snprintf(curlBuf, sizeof(curlBuf), "curl \"http://123.56.19.101:5000/mail?secret=fskd2endprx&email=andrew.liao@uxin.com&subject=RTPP_ALERT&content=%s\"", outStr.c_str());
//		system(curlBuf);
//		if (enable_trace == 1) {
//			printf("\ncurl buf = %s\n", curlBuf);
//		}
////		system("curl \"http://123.56.19.101:5000/mail?secret=fskd2endprx&email=vic.qiu@uxin.com&subject=测试主题&content=测试内容\"");
//	}
}

void* callee_thread_callback(void* lparam) {

	struct pollfd pollfds[2*MAX_SESSION_NUM];
	memset(&pollfds, 0, sizeof(pollfds));

	for (int idx=0; idx<session_num; ++idx) {
		pollfds[2*idx].fd = g_sessionInfoSet[idx].callerFd;
		pollfds[2*idx].events = POLLIN|POLLPRI;
		pollfds[2*idx+1].fd = g_sessionInfoSet[idx].calleeFd;
		pollfds[2*idx+1].events = POLLIN|POLLPRI;
	}	

	struct sockaddr rtpp_addr;
	int size = sizeof(rtpp_addr);
	char recv_buf[MAX_BUF_SIZE];

	int recv_buf_size = 0;
	while(true) {
		int ret = poll(pollfds, 2*session_num, 10);
		if (ret<=0)
			continue;

		for (int fdIdx=0; fdIdx<2*session_num; ++fdIdx) {
			if (pollfds[fdIdx].revents&POLLIN) {
				do {
//					memset(&rtpp_addr,0,size);
					recv_buf_size = recvfrom(pollfds[fdIdx].fd, recv_buf, MAX_BUF_SIZE, 0, &rtpp_addr, (socklen_t *)&size);
				}while (recv_buf_size == -1 && (errno == EINTR || errno == EAGAIN));

				if (recv_buf_size == 0)
				{
					continue;
				}

				unsigned send_time = ntohl(*((unsigned int*)(recv_buf+4)));
				unsigned recv_time = getmstime();

				session_stat(fdIdx/2, fdIdx%2, recv_time-send_time);
				//if (enable_trace == 1)
				if (enable_trace == 1 && g_sessionInfoSet[fdIdx/2].sessionStat[fdIdx%2].count % 10 == 0)
				{
					printf("=====recv package from rtpp, fd:%d, port:%d, len:%d, packageIdx:%u, send_time:%u, recv_time:%u, timeval(us):%u\n",
											pollfds[fdIdx].fd, ntohs(((struct sockaddr_in *)(&rtpp_addr))->sin_port),
											recv_buf_size, g_sessionInfoSet[fdIdx/2].sessionStat[fdIdx%2].count, send_time,recv_time,recv_time-send_time);
				}

			}
		}
	}
	pthread_exit(NULL);

	return NULL;
}

extern char RTPCIP[16];
extern unsigned int RTPCPORT;
extern char LOCALIP[16];
//extern char NETCARD[8];

int main(int argc, char** argv)
{
	int index;
	char ch;
	while((ch = getopt(argc, argv, "htfn:s:i:p:a:d:t:l:r:j:k:m:")) != EOF) {
		switch(ch)
		{
		case 'h':
			fprintf(stderr, "使用说明：\n");
			fprintf(stderr, "  -h: 帮助\n");
			fprintf(stderr, "  -n: 会话数目（1~2000, 默认为1）\n");
			fprintf(stderr, "  -s: 发送RTP包数大小（0~4096字节， 默认为20）\n");
			fprintf(stderr, "  -i: 发送包时间间隔（毫秒，默认为20)\n");
			fprintf(stderr, "  -p: 单个会话单向发送包数量（默认为20000000)\n");
			fprintf(stderr, "  -a: rtpc的ip地址\n");
			fprintf(stderr, "  -d: rtpc的端口 (默认端口为9966)\n");
			fprintf(stderr, "  -l: 本地ip地址\n");
			fprintf(stderr, "  -j: 监控延时值（毫秒， 默认值400）\n");
			fprintf(stderr, "  -k: 监控告警比例（默认值为0.1）\n");
			fprintf(stderr, "  -m: ping rtpp次数（默认值为0）\n");
			fprintf(stderr, "  -t: 打开trace信息开关(默认关闭)\n");
			fprintf(stderr, "  -f: 发送所有rtpp信息开关(默认关闭)\n");
			fprintf(stderr, "  -r: 输出统计信息级别， 0， 不输出， 1， 输出 rtpp统计， 2，输出rtpp和session统计， 默认为2\n");
			return -1;

		case 'n':
			session_num = atoi(optarg);
			if (session_num > MAX_SESSION_NUM || session_num < 1)
			{
				printf("session number should be 1~%d\n", MAX_SESSION_NUM);
				return -1;
			}
		  break;

		case 'i':
			send_interval = atoi(optarg);
		  break;
		case 's':
			rtp_data_size = atoi(optarg);
			if (rtp_data_size > RTP_MAX_SIZE || rtp_data_size <= 0)
			{
				printf("rtp size should be 0~%d\n", RTP_MAX_SIZE);
				return -1;
			}

			break;
		case 'p':
			send_pkg_num = atoi(optarg);
			if (send_pkg_num <= 0)
			{
				printf("send pkg number should exceed 0\n");
				return -1;
			}
			break;
		case 'a':
			strcpy(RTPCIP, optarg);
			break;

		case 'd':
			RTPCPORT = atoi(optarg);
			break;

		case 't':
			enable_trace = 1;
			break;
		case 'f':
			enables_send_all = 1;
			break;

		case 'l':
			strcpy(LOCALIP, optarg);
			break;
		case 'j':
			delay_limit = atoi(optarg);
			if (delay_limit < 0)
			{
				printf("delay_limit should be 0~无穷大\n");
				return -1;
			}
			break;
		case 'k':
			watch_ratio_limit = atof(optarg);
			if (watch_ratio_limit < 0.0 || watch_ratio_limit > 1.0)
			{
				printf("ratio_limit should be 0.0~1.0\n");
				return -1;
			}
			break;
		case 'r':
			info_level = atoi(optarg);
			if (info_level < 0)
			{
				printf("info_level should be 0~2\n");
				return -1;
			}
			break;
		case 'm':
			ping_times = atoi(optarg);
			if (ping_times < 0)
			{
				printf("ping_times should > 0\n");
				return -1;
			}
			break;

		default:
			printf("invalid parameters\n");
			return -1;
		}
	}

	if (0==sock_ip_valid(RTPCIP, NULL) || 0==sock_ip_valid(LOCALIP, NULL))
	{
		printf("rtcpip [%s] or localip [%s] maybe invalid\n", RTPCIP, LOCALIP);
		return -1;
	}


	if (enable_trace == 1) {
		printf ("localip: %s, rtpcip: %s, rtpcport: %d, callerip: %s, calleeip: %s\n",
				LOCALIP, RTPCIP, RTPCPORT, LOCALIP, LOCALIP);
	}

	CDemoAadjure* aadjure_obj = new CDemoAadjure();
	if (!aadjure_obj) {
		fprintf(stderr, "create object failed, aadjuer: %p\n", aadjure_obj);
		return -1;
	}
	
	int ret;
	srand(time(NULL));
	for (int idx = 0; idx < session_num; ++idx)
	{
		StSessionInfo sessionInfo;
		sessionInfo.sessionIdx = idx + int(getpid());
		sessionInfo.callerPort = CALLERPORT+idx;
		sessionInfo.calleePort = CALLEEPORT+idx;
//		strncpy(sessionInfo.callerIp, aadjure_obj->get_local_ip(), 16);
//		strncpy(sessionInfo.calleeIp, aadjure_obj->get_local_ip(), 16);
		get_random_ip(sessionInfo.callerIp);
		get_random_ip(sessionInfo.calleeIp);

		ret = aadjure_obj->open_session(sessionInfo);

		if (ret != 0) {
			fprintf(stderr, "aadjure open session:%d failed! ret: %d\n", sessionInfo.sessionIdx, ret);
			return -1;
		}

		int sock_fd = socket(PF_INET, SOCK_DGRAM, 0);
		if (sock_fd<=0) {
			fprintf (stderr, "create udp socket failed, ret: %d\n", sock_fd);
			return -1;
		}
		set_nonblock_socket(sock_fd);
		sessionInfo.callerFd = sock_fd;

		sock_fd = socket(PF_INET, SOCK_DGRAM, 0);
		if (sock_fd<=0) {
			fprintf (stderr, "create udp socket failed, ret: %d\n", sock_fd);
			return -1;
		}
		set_nonblock_socket(sock_fd);
		sessionInfo.calleeFd = sock_fd;


		aadjure_obj->get_rtp_info(sessionInfo);
		if (enable_trace == 1) {
			printf("Build a session index:%d, caller:[%s], callee:[%s], rtppCaller:[%s:%d], rtppCallee:[%s:%d]\n",
					sessionInfo.sessionIdx, sessionInfo.callerIp, sessionInfo.calleeIp,sessionInfo.rtppCallerIp,
					sessionInfo.rtppPort[0], sessionInfo.rtppCalleeIp, sessionInfo.rtppPort[1]);
		}

		std::string rtppStr = sessionInfo.rtppCallerIp;
		int rtppNum = 1;
		if (strncmp(sessionInfo.rtppCallerIp, sessionInfo.rtppCalleeIp, 16)) {
			rtppStr = std::string("&") + std::string(sessionInfo.rtppCalleeIp);
			rtppNum = 2;
		}

		std::map<std::string, StRtppStat>::iterator rtppIter = g_rtppStatMap.find(rtppStr);
		if (rtppIter != g_rtppStatMap.end()) {
			rtppIter->second.sessionNum++;
		} else {
			StRtppStat rtppStat;
			memset(&rtppStat, 0, sizeof(StRtppStat));
			rtppStat.sessionStat[0].min = 1000000;
			rtppStat.sessionStat[1].min = 1000000;
			g_rtppStatMap[rtppStr] = rtppStat;
			g_rtppStatMap[rtppStr].rtppNum = rtppNum;
			g_rtppStatMap[rtppStr].sessionNum = 1;
		}

		memset(sessionInfo.sessionStat, 0, 2*sizeof(sessionInfo.sessionStat[0]));
		sessionInfo.sessionStat[0].min = 1000000;
		sessionInfo.sessionStat[1].min = 1000000;
		g_sessionInfoSet[idx] = sessionInfo;

	}

	sleep(1);
	pthread_t caller_thread = 0;
	ret = pthread_create(&caller_thread, NULL, caller_thread_callback, NULL);
	if (0 != ret) {
		fprintf(stderr, "create caller thread failed, ret: %d\n", ret);
		return -1;
	}

	pthread_t callee_thread = 0;
	ret = pthread_create(&callee_thread, NULL, callee_thread_callback, NULL);
	if (0 != ret) {
		fprintf(stderr, "create callee thread failed, ret: %d\n", ret);
		return -1;
	}

	pthread_join(caller_thread, NULL);
	sleep(1);
	pthread_cancel(callee_thread);
	for (int idx = 0; idx < session_num; ++idx)
	{
		close(g_sessionInfoSet[idx].callerFd);
		close(g_sessionInfoSet[idx].calleeFd);
		aadjure_obj->close_session(g_sessionInfoSet[idx].sessionIdx);
	}

	free(aadjure_obj);
	aadjure_obj = NULL;
	sleep(1);
	print_session_stat();
	return 0;
}











