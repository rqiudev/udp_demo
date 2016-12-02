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




StSessionInfo g_sessionInfoSet[MAX_SESSION_NUM];
int session_num = 1;
int send_interval = 20;
int rtp_data_size = 20;
int send_pkg_num = 20000000;
int send_pkg_total = 0;

long getustime(void)
{
    struct timeval timev;
    gettimeofday(&timev, 0);
    return 1000000*timev.tv_sec + timev.tv_usec;
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
			pack_obj._timeStamp = getustime();
			pack_obj.makeup_rtp_packet(send_buf, &send_buf_size, rtp_data_size);
			struct sockaddr_in sock_addr;
			bzero(&sock_addr, sizeof(sock_addr));
			sock_addr.sin_family = AF_INET;
			sock_addr.sin_port = htons(g_sessionInfoSet[sessionIdx].rtppPort[0]);
			sock_addr.sin_addr.s_addr = inet_addr(g_sessionInfoSet[sessionIdx].rtppCallerIp);
			int sendRet = sendto(g_sessionInfoSet[sessionIdx].callerFd, send_buf, send_buf_size,
					0, (struct sockaddr*)&sock_addr, sizeof(struct sockaddr));
			if (sendRet < 0)
			{
				printf("UDP sending fail, ip = 0x%x port = %d, error(%s)\n",
						sock_addr.sin_addr.s_addr, sock_addr.sin_port, strerror(errno));
				return 0;
			}

			if (send_buf_size != sendRet)
			{
				printf("UDP send fail, len = %u not send", send_buf_size - sendRet);
				return 0;
			}

			//printf("=====send package from caller at port:%d to rtpp, len:%d, packageIdx:%d, send_time:%u\n",
			//		g_sessionInfoSet[sessionIdx].rtppPort[0], send_buf_size, pkgIdx, pack_obj._timeStamp);

			pack_obj._timeStamp = getustime();
			pack_obj.makeup_rtp_packet(send_buf, &send_buf_size, rtp_data_size);
			bzero(&sock_addr, sizeof(sock_addr));
			sock_addr.sin_family = AF_INET;
			sock_addr.sin_port = htons(g_sessionInfoSet[sessionIdx].rtppPort[1]);
			sock_addr.sin_addr.s_addr = inet_addr(g_sessionInfoSet[sessionIdx].rtppCalleeIp);
			sendRet = sendto(g_sessionInfoSet[sessionIdx].calleeFd, send_buf, send_buf_size, 0, (struct sockaddr*)&sock_addr, sizeof(struct sockaddr));
			if (sendRet < 0)
			{
				printf("UDP sending fail, ip = 0x%x port = %d, error(%s)\n",
						sock_addr.sin_addr.s_addr, sock_addr.sin_port, strerror(errno));
				return 0;
			}

			if (send_buf_size != sendRet)
			{
				printf("UDP send fail, len = %u not send", send_buf_size - sendRet);
				return 0;
			}

			//printf("=====send package from callee at port:%d to rtpp, len:%d, packageIdx:%d, send_time:%u\n",
			//		g_sessionInfoSet[sessionIdx].rtppPort[1], send_buf_size, pkgIdx, pack_obj._timeStamp);
			send_pkg_total++;

		}
		pkgIdx++;
	}
	pthread_exit(NULL);


	return 0;
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
	int pkgIdx = 0;
	int recv_buf_size = 0;
    while(true) {
		int ret = poll(pollfds, 2*session_num, 10);
		if (ret<=0)
			continue;

		for (int fdIdx=0; fdIdx<2*session_num; ++fdIdx) {
			if (pollfds[fdIdx].revents&POLLIN) {
				do {
					memset(&rtpp_addr,0,size);
					recv_buf_size = recvfrom(pollfds[fdIdx].fd, recv_buf, MAX_BUF_SIZE, 0, &rtpp_addr, (socklen_t *)&size);
				}while (recv_buf_size == -1 && (errno == EINTR || errno == EAGAIN));

				if (recv_buf_size == 0)
				{
					continue;
				}

				unsigned send_time = ntohl(*((unsigned int*)(recv_buf+4)));
				unsigned recv_time = getustime();
				printf("=====recv package from rtpp, fd:%d, port:%d, len:%d, packageIdx:%d, send_time:%u, recv_time:%u, timeval(us):%u\n",
						pollfds[fdIdx].fd, ntohs(((struct sockaddr_in *)(&rtpp_addr))->sin_port),
						recv_buf_size, pkgIdx, send_time,recv_time,recv_time-send_time);
				pkgIdx++;
			}
		}
	}
	pthread_exit(NULL);

	return NULL;
}


int main(int argc, char** argv)
{
	int index;
	char ch;
	while((ch = getopt(argc, argv, "hn:s:i:p:")) != EOF) {
		switch(ch)
		{
		case 'h':
			fprintf(stderr, "使用说明：\n");
			fprintf(stderr, "  -h: 帮助\n");
			fprintf(stderr, "  -n: 会话数目（1~1000, 默认为1）\n");
			fprintf(stderr, "  -s: 发送RTP包数大小（0~4096字节， 默认为20）\n");
			fprintf(stderr, "  -i: 发送包时间间隔（毫秒，默认为20)\n");
			fprintf(stderr, "  -p: 单个会话单向发送包数量（默认为20000000)\n");
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

		default:
			printf("invalid parameters\n");
			return -1;
		}
	}


	CDemoAadjure* aadjure_obj = new CDemoAadjure();
	if (!aadjure_obj) {
		fprintf(stderr, "create object failed, aadjuer: %p\n", aadjure_obj);
		return -1;
	}
	
	int ret;
	for (int idx = 0; idx < session_num; ++idx)
	{
		StSessionInfo sessionInfo;
		sessionInfo.sessionIdx = idx + int(getpid());
		sessionInfo.callerPort = CALLERPORT+idx;
		sessionInfo.calleePort = CALLEEPORT+idx;
		strncpy(sessionInfo.callerIp, aadjure_obj->get_local_ip(), 16);
		strncpy(sessionInfo.calleeIp, aadjure_obj->get_local_ip(), 16);

		ret = aadjure_obj->open_session(sessionInfo);

		if (ret != 0) {
			fprintf(stderr, "aadjure open session:%d failed! ret: %d\n", sessionInfo.sessionIdx, ret);
			continue;;
		}

		int sock_fd = socket(PF_INET, SOCK_DGRAM, 0);
		if (sock_fd<=0) {
			fprintf (stderr, "create udp socket failed, ret: %d\n", sock_fd);
			continue;;
		}
		set_nonblock_socket(sock_fd);
		sessionInfo.callerFd = sock_fd;

		sock_fd = socket(PF_INET, SOCK_DGRAM, 0);
		if (sock_fd<=0) {
			fprintf (stderr, "create udp socket failed, ret: %d\n", sock_fd);
			continue;;
		}
		set_nonblock_socket(sock_fd);
		sessionInfo.calleeFd = sock_fd;


		aadjure_obj->get_rtp_info(sessionInfo);
		printf("Build a session index:%d, caller:[%s:%d], callee:[%s:%d]\n",
				sessionInfo.sessionIdx, sessionInfo.rtppCallerIp, sessionInfo.rtppPort[0],
				sessionInfo.rtppCalleeIp, sessionInfo.rtppPort[1]);

		g_sessionInfoSet[idx] = sessionInfo;

	}

	sleep(1);
	pthread_t caller_thread = 0;
	ret = pthread_create(&caller_thread, NULL, caller_thread_callback, NULL);
	if (0 != ret) {
		fprintf(stderr, "create caller thread failed, ret: %d", ret);
		return -1;
	}

	pthread_t callee_thread = 0;
	ret = pthread_create(&callee_thread, NULL, callee_thread_callback, NULL);
	if (0 != ret) {
		fprintf(stderr, "create callee thread failed, ret: %d", ret);
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
	return 0;
}











