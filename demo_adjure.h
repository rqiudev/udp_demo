#ifndef demo_adjure_h_
#define demo_adjure_h_
#include <pthread.h>


const char LOCALIP[16] = "113.31.82.185";
const char CALLERIP[16] = "113.31.82.185";
const char CALLEEIP[16] = "113.31.82.185";
const unsigned int LOCALPORT = 19967;
const unsigned int CALLERPORT = 20000;
const unsigned int CALLEEPORT = 30000;
const char RTPCIP[16] = "113.31.82.185";
const unsigned int RTPCPORT = 9966;
const int MAX_SESSION_NUM = 1000;
const char NETCARD[8] = "eth0";
#define RTP_MAX_SIZE 4096// 包体8192
#define MAX_BUF_SIZE 8192

typedef struct rtppInfo
{
	char rtppCallerIp[16];
	char rtppCalleeIp[16];
	unsigned short rtppPort[4];;
}StRtppInfo;

typedef struct sessionInfo
{
	char callerIp[16];
	char calleeIp[16];
	int callerPort;
	int calleePort;
	int sessionIdx;
	char rtppCallerIp[16];
	char rtppCalleeIp[16];
	// port 0 2 主叫 1 3主叫
	int rtppPort[4];
	int callerFd;
	int calleeFd;
}StSessionInfo;

class CDemoAadjure {
public:
	CDemoAadjure();
	virtual ~CDemoAadjure();

public:
	virtual int open_session(const StSessionInfo& sessionInfo);
	virtual int get_rtp_info(StSessionInfo& sessionInfo);
	virtual char* get_local_ip();
	virtual int close_session(int sessionIdx);

private:
	int m_localSocketId;
	char m_rtppIp[2][16];
	unsigned short m_rtppPort[4];
	char m_localIp[16];
};

#endif


