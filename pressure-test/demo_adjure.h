#ifndef demo_adjure_h_
#define demo_adjure_h_
#include <pthread.h>

const unsigned int LOCALPORT = 19967;
const unsigned int CALLERPORT = 20000;
const unsigned int CALLEEPORT = 30000;
extern char RTPCIP[16];
extern unsigned int RTPCPORT;
extern char LOCALIP[16];
extern int enable_trace;

const int MAX_SESSION_NUM = 10000;
#define RTP_MAX_SIZE 4096// 包体4096
#define MAX_BUF_SIZE 8192

typedef struct rtppInfo
{
	char rtppCallerIp[16];
	char rtppCalleeIp[16];
	unsigned short rtppPort[4];;
}StRtppInfo;

typedef struct sessionStat
{
	unsigned min;
	unsigned max;
	unsigned total;
	unsigned count;
	unsigned countLimit;
	unsigned delayStat[5];
	double average;
} StSessionStat;

typedef struct rtppStat
{
	StSessionStat sessionStat[2];
	int rtppNum;
	int sessionNum;
} StRtppStat;

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
	StSessionStat sessionStat[2];
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


