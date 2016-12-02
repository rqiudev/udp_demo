#include "demo_adjure.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <map>

#include "json.h"


std::map<int, StRtppInfo> g_callid2rtppinfo;

int CreateSocketForRtpc(const char* localIp, unsigned short localPort)
{
    struct sockaddr_in stAddr = {0};
    int  lBuffSize = 4096 * 1024 * 2;
    int  lSocket = 0;

    if (0 > (lSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)))
    {
        printf("SOCKET : %s", strerror(errno));
        return -1;
    }

    stAddr.sin_family      = AF_INET;
    stAddr.sin_port        = htons(localPort);
    stAddr.sin_addr.s_addr = inet_addr(localIp);

    if (bind(lSocket, (struct sockaddr *)&stAddr, sizeof(stAddr)) < 0)
    {
        printf("bind : %s %s 0x%x", strerror(errno), localIp, stAddr.sin_addr.s_addr);
        close(lSocket);
        return -1;
    }
    if (setsockopt(lSocket, SOL_SOCKET, SO_RCVBUF, &lBuffSize, sizeof(lBuffSize)) < 0)
    {
        printf("setsockopt : %s", strerror(errno));
        close(lSocket);
        return -1;
    }

    if (setsockopt(lSocket, SOL_SOCKET, SO_SNDBUF, &lBuffSize, sizeof(lBuffSize)) < 0)
    {
        printf("setsockopt : %s", strerror(errno));
        close(lSocket);
        return -1;
    }

    printf("created socket succceed, socketId:%d, local ip:%s, local port:%d\n",
    		lSocket, localIp, localPort);
    return lSocket;
}


int DealRtpcCmdORsp(const json_t *js_root,int callId)
{
	json_t *js_label = NULL;
	json_t *js_call = NULL;

	int routeid = 0;
	int action = 0;
	int result = 0;
	StRtppInfo rtppInfo;

	if(NULL == (js_label = json_find_first_label(js_root,"result")))
	{
		printf("js cannot find result label.");
		return -1;
	}
	result = atoi(js_label->child->text);

	if(NULL == (js_label = json_find_first_label(js_root,"action")))
	{
		printf("js cannot find action label.");
		return -1;
	}
    if (result != 0)
    {
        printf("error! result is %d\n", result);
		return -1;
	}
	action = atoi(js_label->child->text);

    if (action == 3)
    {
		return 0;
	}

	if(NULL == (js_label = json_find_first_label(js_root,"routeid")))
	{
		printf("js cannot find routeid label.");
		return -1;
	}

	routeid = atoi(js_label->child->text);

	//caller
	if(NULL == (js_label = json_find_first_label(js_root,"caller")))
	{
		printf("js cannot find caller label.");
		return -1;
	}

	js_call = js_label->child;
	if(js_call)
	{
		if(NULL == (js_label = json_find_first_label(js_call,"ip")))
		{
			printf("js cannot find caller ip label.");
			return -1;
		}

		strncpy(rtppInfo.rtppCallerIp, js_label->child->text,16);


		if(NULL == (js_label = json_find_first_label(js_call,"port")))
		{
			printf("js cannot find caller port label.");
			return -1;
		}

		rtppInfo.rtppPort[0] = atoi(js_label->child->text);
        if(NULL != (js_label = json_find_first_label(js_call,"vport")))
        {
        	rtppInfo.rtppPort[2] = atoi(js_label->child->text);
        }
	}

	//callee
	if(NULL == (js_label = json_find_first_label(js_root,"callee")))
	{
		printf("js cannot find callee label.");
		return -1;
	}

	js_call = js_label->child;
	if(js_call)
	{
		if(NULL == (js_label = json_find_first_label(js_call,"ip")))
		{
			printf("js cannot find callee ip label.");
			return -1;
		}

		strncpy(rtppInfo.rtppCalleeIp, js_label->child->text,16);


		if(NULL == (js_label = json_find_first_label(js_call,"port")))
		{
			printf("js cannot find callee port label.");
			return -1;
		}

		rtppInfo.rtppPort[1] = atoi(js_label->child->text);
        if(NULL != (js_label = json_find_first_label(js_call,"vport")))
        {
        	rtppInfo.rtppPort[3] = atoi(js_label->child->text);
        }
	}

	if((rtppInfo.rtppPort[0] == 0) || (rtppInfo.rtppPort[1] == 0))
	{
		printf("rtpc reply incorrect port,[0]=%d,[1]=%d",rtppInfo.rtppPort[0],rtppInfo.rtppPort[1]);
		return -1;
	}
	g_callid2rtppinfo[callId] = rtppInfo;
	printf("open callid2rtppinfo, callid:%d, callerInfo:[%s-%d-%d], callerInfo:[%s-%d-%d]\n",
			callId, rtppInfo.rtppCallerIp, rtppInfo.rtppPort[0], rtppInfo.rtppPort[2],
			rtppInfo.rtppCalleeIp, rtppInfo.rtppPort[1], rtppInfo.rtppPort[3]);

	return 0;
}

int DealRtpcCmdDRsp(const json_t *js_root,int callId)
{
	std::map<int, StRtppInfo>::iterator iter = g_callid2rtppinfo.find(callId);
	if (iter == g_callid2rtppinfo.end())
	{
		printf("failed to get rtppinfo, callid:%d", callId);
		return -1;
	}
	g_callid2rtppinfo.erase(iter);
	printf("del callid2rtppinfo, callid:%d\n", callId);
    return 0;
}

//calllId/-1
int GetCallIdFromCookie(char *pcookie)
{//cookie format: ip@callId@sessionSn

	char *p = NULL;
	char *p1 = NULL;

	if(!pcookie)
		return -1;

	if(NULL == (p = strchr(pcookie,'@')))
	{
		return -1;
	}

	*p = '\0';
	p++;

	if(NULL == (p1 = strchr(p,'@')))
	{
		return -1;
	}

	*p1 = '\0';
	p1++;

	return(atoi(p));
}


void DealRtpcRsp(char *rbuf,int rlen,const char* lIp, int lPort)
{
	json_t *js_root = NULL;
	json_t *js_label = NULL;
	int js_ret = -1;

	char cmd[4] = "";
	unsigned int sn = 0;
	int result = 0;
	char cookie[65] = "";
	int callId = 0;
	int ret = 0;

	//update
	if (strncmp(lIp, RTPCIP, 16) != 0 || lPort != RTPCPORT)
	{
		printf("receive msg from invalid rtpc %s:%d", lIp, lPort);
		return;
	}

	//json format decode
	json_strip_white_spaces(rbuf);

	if(JSON_OK != (js_ret = json_parse_document(&js_root,rbuf)))
	{
		printf("rtpc_call_udp_deal:json_parse_document err(%d).",js_ret);
		return;
	}

	do
	{
		//cmd
		if(NULL == (js_label = json_find_first_label(js_root,"cmd")))
		{
			printf("js cannot find <%s> label.","cmd");
			break;
		}
		cmd[0] = js_label->child->text[0];
		cmd[1] = '\0';

		//sn
		if(NULL == (js_label = json_find_first_label(js_root,"sn")))
		{
			printf("js cannot find <%s> label.","sn");
			break;
		}
		sn = atoi(js_label->child->text);


		//cookie
		if(NULL == (js_label = json_find_first_label(js_root,"cookie")))
		{
			printf("js cannot find <%s> label.","cookie");
			break;
		}
		strncpy(cookie,js_label->child->text,sizeof(cookie));

		//decode callId from cookie
		if(-1 == (callId = GetCallIdFromCookie(cookie)))
		{
			printf("get callId err,cookie:%s!!!",cookie);
			break;
		}

		//result
		if(NULL == (js_label = json_find_first_label(js_root,"result")))
		{
			printf("js cannot find <%s> label.","result");
			break;
		}
		result = atoi(js_label->child->text);

		printf("cmd:%s,sn:%u,cookie:%s,result:%d\n",cmd,sn,cookie,result);

		if(cmd[0] == 'O')
		{
			ret = DealRtpcCmdORsp(js_root, callId);
			if( 0 != ret )
			{
				printf("allocate rtpc result=%d!",result);
			}

		}
		else if(cmd[0] == 'D')
		{
			if(result != 0)
				printf("release rtpc result=%d!",result);
			else
				DealRtpcCmdDRsp(js_root, callId);
		}
		else
		{
			printf("unknown cmd=%c!",cmd[0]);
		}
	}while(0);

	json_free_value(&js_root);

}



void* RecvRtpcPthread(void *arg)
{
	struct sockaddr_in *s_in;
	char ip[16] ="";
	struct sockaddr rtpp_addr;
	struct pollfd fds[1];
	char rbuf[4096];
	int fds_num = 0;
	int events = 0;
	int size = 0;
	int rlen = 0;

	fds[0].fd = *((int*)(arg));
	fds[0].events = POLLIN;
	fds[0].revents = 0;
	fds_num++;

	for(;;)
	{
		do
		{
			events = poll(fds, fds_num, 2000 );
		}while (events == -1 && (errno == EINTR || errno == EAGAIN));
		if(events < 0)
		{
			printf("poll failed.");
			pthread_exit(NULL);
		}

		if(events == 0)
		{
			printf("poll no events happen,continue.");
                        //sched_yield();
                        sleep(1);
			continue;
		}

		if(fds[0].revents & POLLIN)
		{//udp

			size = sizeof(rtpp_addr);
			memset(&rtpp_addr,0,size);
			do
			{
				rlen = recvfrom(fds[0].fd, rbuf, sizeof(rbuf), 0,&rtpp_addr, (socklen_t *)&size);
			}while (rlen == -1 && (errno == EINTR || errno == EAGAIN));

			if(rlen == -1)
			{
				printf("recvfrom failed.");
				return 0;
			}

			s_in = (struct sockaddr_in*)(&rtpp_addr);
			rbuf[rlen] = '\0';
			strncpy(ip,inet_ntoa(s_in->sin_addr),sizeof(ip));
			printf("Recv rtpc[ip:%s][port:%d]:%s\n",ip,ntohs(s_in->sin_port), rbuf);

			//deal
			DealRtpcRsp(rbuf,rlen,ip,ntohs(s_in->sin_port));
		}
	}
	return NULL;
}


void GetLocalIpAddr(char* localIp)
{
	int inet_sock;
	struct ifreq ifr;
	inet_sock = socket(AF_INET, SOCK_DGRAM, 0);

	strcpy(ifr.ifr_name, NETCARD);
	//SIOCGIFADDR标志代表获取接口地址
	if (ioctl(inet_sock, SIOCGIFADDR, &ifr) <  0)
	{
		printf("failed to get local ip\n");
		return;
	}
	strncpy(localIp, inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr), 16);
}

CDemoAadjure::CDemoAadjure()
{
	GetLocalIpAddr(m_localIp);
	m_localSocketId = CreateSocketForRtpc(m_localIp, LOCALPORT);
}

CDemoAadjure::~CDemoAadjure()
{
	close(m_localSocketId);
}

int SendMsgToRtpc(void *pack, int len,int socketId, unsigned int ipAddr, unsigned short port)
{
    int  iRet = -1;
    struct sockaddr_in fsin;

    if (pack == NULL)
    {
        return -1;
    }

    fsin.sin_family = AF_INET;
    fsin.sin_port   = htons(port);
    fsin.sin_addr.s_addr = ipAddr;

    iRet = sendto(socketId, (char *)pack, len, 0, (struct sockaddr *)&fsin, sizeof(struct sockaddr));
    if (iRet < 0)
    {
        printf("UDP sending fail, ip = 0x%x port = %d, error(%s)\n",
            fsin.sin_addr.s_addr, fsin.sin_port, strerror(errno));
        return -1;
    }

    if (len != iRet)
    {
        printf("UDP send fail, len = %u not send", len - iRet);
		return -1;
    }

    return 0;
}

int RecvMsgToRtpc(int socketId)
{
	char rbuf[4096];
	int rlen = 0;
	struct sockaddr rtpp_addr;
	int size = sizeof(rtpp_addr);
	memset(&rtpp_addr,0,size);
	do
	{
		rlen = recvfrom(socketId, rbuf, sizeof(rbuf), 0, &rtpp_addr, (socklen_t *)&size);
	}while (rlen == -1 && (errno == EINTR || errno == EAGAIN));

	if(rlen == -1)
	{
		printf("recvfrom failed.");
		return -1;
	}

	struct sockaddr_in *s_in = (struct sockaddr_in*)(&rtpp_addr);
	rbuf[rlen] = '\0';
	char ip[16] ="";
	strncpy(ip,inet_ntoa(s_in->sin_addr),sizeof(ip));
	printf("Recv rtpc[ip:%s][port:%d]:%s\n",ip,ntohs(s_in->sin_port), rbuf);

	//deal
	DealRtpcRsp(rbuf,rlen,ip,ntohs(s_in->sin_port));
    return 0;
}


int CDemoAadjure::open_session(const StSessionInfo& sessionInfo)
{
	// Send O Command to RTPC
	char szCmd[2048] = {0};
    snprintf(szCmd, sizeof(szCmd)-1,
    		"{\"cmd\":\"O\",\"sn\":1062,\"cookie\":\"113.31.82.185@%d@36\",\"callid\":\"1000020529jV21114656vH13725554536\",\"notify\":\"%s:%d\",\"action\":1,\"record\":0,\"fb\":1,\"callmode\":1,\"caller\":{\"uid\":\"1000020529\",\"ip\":\"%s\",\"port\":%d,\"vport\":0,\"ver\":\"5.0.9\",\"pf\":4,\"rtpplist\":[],\"phone\":\"15875589105\"},\"callee\":{\"uid\":\"1000020534\",\"ip\":\"%s\",\"port\":%d,\"vport\":0,\"ver\":\"5.0.9\",\"pf\":4,\"rtpplist\":[],\"phone\":\"013725554536\"}}",
			sessionInfo.sessionIdx, m_localIp, LOCALPORT,
			sessionInfo.callerIp, sessionInfo.callerPort,
			sessionInfo.calleeIp, sessionInfo.calleePort);
	//snprintf(szCmd, sizeof(szCmd)-1, "{\"cmd\":\"O\",\"sn\":1062,\"cookie\":\"113.31.82.185@8257536@36\",\"callid\":\"1000020529jV21114656vH13725554536\",\"notify\":\"113.31.82.185:19967\",\"action\":1,\"record\":0,\"fb\":1,\"callmode\":1,\"caller\":{\"uid\":\"1000020529\",\"ip\":\"183.15.244.8\",\"port\":0,\"vport\":0,\"ver\":\"5.0.9\",\"pf\":4,\"rtpplist\":[],\"phone\":\"15875589105\"},\"callee\":{\"uid\":\"1000020534\",\"ip\":\"183.15.244.8\",\"port\":0,\"vport\":0,\"ver\":\"5.0.9\",\"pf\":4,\"rtpplist\":[],\"phone\":\"013725554536\"}}");
	int cmdLen = strlen(szCmd);
	if(0 != SendMsgToRtpc((void *)szCmd,cmdLen,m_localSocketId, inet_addr(RTPCIP), RTPCPORT))
    {
		printf("Failed to Send O Command to rtpc, cmd:%s, rtpcip:%s, rtpcport:%d!\n", szCmd, RTPCIP, RTPCPORT);
        return -1;
    }
    printf("Send O cmd to rtpc success, ip:%s, port:%d cmd:%s\n",RTPCIP, RTPCPORT, szCmd);
	RecvMsgToRtpc(m_localSocketId);
	return 0;
}

int CDemoAadjure::close_session(int sessionIdx)
{
	// Send D Command to RTPC
	char szCmd[2048] = {0};


	snprintf(szCmd, sizeof(szCmd)-1, "{\"cmd\":\"D\",\"sn\":1068,\"cookie\":\"113.31.82.185@%d@36\",\"action\":1,\"routeid\":0,\"caller\":\"1000020529\",\"callee\":\"1000020534\"}",
			sessionIdx);

	//snprintf(szCmd, sizeof(szCmd)-1, "{\"cmd\":\"D\",\"sn\":1068,\"cookie\":\"113.31.82.185@8257536@36\",\"action\":1,\"routeid\":0,\"caller\":\"1000020529\",\"callee\":\"1000020534\"}");
	int cmdLen = strlen(szCmd);
	if(0 != SendMsgToRtpc((void *)szCmd,cmdLen,m_localSocketId, inet_addr(RTPCIP), RTPCPORT))
    {
		printf("Failed to Send D Command to rtpc, cmd:%s, rtpcip:%s, rtpcport:%d!\n", szCmd, RTPCIP, RTPCPORT);
        return -1;
    }
	printf("Send D cmd to rtpc success, ip:%s, port:%d cmd:%s\n",RTPCIP, RTPCPORT, szCmd);
	RecvMsgToRtpc(m_localSocketId);
	return 0;
}

int CDemoAadjure::get_rtp_info(StSessionInfo& sessionInfo)
{
	int callId = sessionInfo.sessionIdx;
	std::map<int, StRtppInfo>::iterator iter = g_callid2rtppinfo.find(callId);
	if (iter == g_callid2rtppinfo.end())
	{
		printf("failed to get rtppinfo, callid:%d", callId);
		return -1;
	}

	strncpy(sessionInfo.rtppCallerIp, iter->second.rtppCallerIp, 16);
	strncpy(sessionInfo.rtppCalleeIp, iter->second.rtppCalleeIp, 16);
	sessionInfo.rtppPort[0] = iter->second.rtppPort[0];
	sessionInfo.rtppPort[1] = iter->second.rtppPort[1];
	sessionInfo.rtppPort[2] = iter->second.rtppPort[2];
	sessionInfo.rtppPort[3] = iter->second.rtppPort[3];
	//printf("get caller rtpp info [%s:%d]\n", ip, *port);
	return 0;
}

char* CDemoAadjure::get_local_ip()
{
	return m_localIp;
}
