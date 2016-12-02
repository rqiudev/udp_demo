
int tcp_recv(int sock, char* buf, int len)
{
	char* pBuf = (char*)buf;
	int nRecv;
	int nRecvTotalLen = 0;

	//first recv 1 byte indicate the datelen from tcp
	while(1)
	{
		nRecv = recv(sock, (void*)pBuf, 1, 0);
		if(nRecv == 1)
		{
			nRecvTotalLen += 1;
			break;
		}
		else if(nRecv < 0)
		{
			if(errno==EAGAIN || errno==EWOULDBLOCK || errno==EINTR)
			{
				usleep(10);
				continue;
			}

			xxxx_trace(exxxx, eError,"Failed to recv tcp date, sock:%d, len = %u.", sock, 1);
			return -1;
		}
		else
		{
			xxxx_trace(exxxx, eError,"The tcp may be closed, sock:%d", sock);
			return 0;
		}
	}

	//second recv datalen-1 from tcp
	len = (int)buf[0] - 1;
	pBuf = pBuf+1;
	while(len > 0)
	{
		nRecv = recv(sock, (void*)pBuf, len, 0);
		if(nRecv > 0)
		{
			if (nRecv == len)
			{
				nRecvTotalLen += nRecv;
				return nRecvTotalLen;
			}

			len -= nRecv;
			pBuf += nRecv;
			nRecvTotalLen += nRecv;
		} else if(nRecv < 0)
		{
			if(errno==EAGAIN || errno==EWOULDBLOCK || errno==EINTR)
			{
				usleep(10);
				continue;
			}

			xxxx_trace(exxxx, eError,"Failed to recv tcp date, sock:%d, len = %u.", sock, len);
			return -1;
		} else {
			xxxx_trace(exxxx, eError,"The tcp may be closed, sock:%d", sock);
			return 0;
		}
	}

	return nRecvTotalLen;
}



int xxxx_tcp_recv(int sock, char* buf, int len)
{
	if (sock < 0 || buf == NULL || len <= 0)
	{
		xxxx_trace(exxxx, eError,"Invalid arguments for tcp_recv");
	}

	int nRecv;
	while(len > 0)
	{
		nRecv = recv(sock, buf, len, 0);
		if(nRecv > 0)
		{
			xxxx_trace(exxxx, eDebug,"Success to xxxx_tcp_recv, sock=%d len=%d\n", sock, nRecv);
			return nRecv;
		} else if(nRecv < 0)
		{
			if(errno==EAGAIN || errno==EWOULDBLOCK || errno==EINTR)
			{
				usleep(10);
				continue;
			}

			xxxx_trace(exxxx, eError,"Failed to recv tcp date, sock:%d, len = %u.", sock, len);
			return -1;
		} else
		{
			xxxx_trace(exxxx, eError,"The tcp may be closed, sock:%d", sock);
			return 0;
		}
	}

	return nRecv;
}

//remember to use first byte indicate the data length before send
int xxxx_tcp_send(int sock, char* buf, int len)
{
	if (sock < 0 || buf == NULL || len <= 0)
	{
		xxxx_trace(exxxx, eError,"Invalid arguments for tcp_send");
	}

	char* pBuf = (char*)buf;
	int nSend;
	int nSendTotalLen = 0;
	while(len > 0)
	{
		nSend = send(sock, (const void*)pBuf, len, 0);
		if(nSend >= 0)
		{
			if (nSend == len)
			{
				nSendTotalLen += nSend;
				xxxx_trace(exxxx, eDebug,"Success to xxxx_tcp_send, sock=%d len=%d\n", sock, nSendTotalLen);
				return nSendTotalLen;
			}

			len -= nSend;
			pBuf += nSend;
			nSendTotalLen += nSend;
		} else if(nSend < 0) {
			if(errno==EAGAIN || errno==EWOULDBLOCK || errno==EINTR) {
				usleep(10);
				continue;
			}

			xxxx_trace(exxxx, eError,"Failed to send tcp date, sock:%d, len = %u.", sock, len);
			return -1;
		}
	}
	return nSendTotalLen;
}




int do_xxxx_connect_failed(int index_xxxx)
{
	xxxx_trace(exxxx, eError, "Failed to connect to xxxx socket, idx=%d ip=%s, port=%d, fd=%d\n",
			index_xxxx, gxxxx[index_xxxx].ip, gxxxx[index_xxxx].port, gxxxx[index_xxxx].fdCmdTcp);
	close (gxxxx[index_xxxx].fdCmdTcp);
	gxxxx[index_xxxx].fdCmdTcp= -1;
	gxxxx[index_xxxx].tcpConnected = 0;
	gxxxx[index_xxxx].status = 0;

	return 0;
}

int do_xxxx_connect_success(int index_xxxx)
{
	if (xxxx_OK != xxxx_async_select(exxxx, gxxxx[index_xxxx].fdCmdTcp, xxxx_TCP_xxxx_CMD_MSG, xxxx_NULL, xxxx_NULL))
	{
		xxxx_trace (exxxx, eError, "failed to xxxx_async_select idx=%d", index_xxxx);
		do_xxxx_connect_failed(index_xxxx);
		return -1;
	}

	gxxxx[index_xxxx].tcpConnected = 1;
	xxxx_trace (exxxx, eWarn, "Success to connect to xxxx, idx=%d ip=%s, port=%d, fd=%d\n",
			index_xxxx, gxxxx[index_xxxx].ip, gxxxx[index_xxxx].port, gxxxx[index_xxxx].fdCmdTcp);
	return 0;
}

int xxxx_tcp_connect(int index_xxxx, int sec)
{
	if (index_xxxx < 0 || index_xxxx >= MAX_xxxx_NUM)
	{
		xxxx_trace(exxxx, eError,"Invalid xxxx index\n");
		return RET_ERR;
	}

    if (gxxxx[index_xxxx].fdCmdTcp == -1)
	{
        xxxx_trace(exxxx, eDebug, "connecting xxxx socket, idx=%d", index_xxxx);
    }
	else
	{
        xxxx_trace(exxxx, eDebug, "reconnecting xxxx socket, idx=%d", index_xxxx);
        close(gxxxx[index_xxxx].fdCmdTcp);
    }

    gxxxx[index_xxxx].fdCmdTcp = socket(AF_INET, SOCK_STREAM, 0);
    int sockfd = gxxxx[index_xxxx].fdCmdTcp;
    if (sockfd== -1)
	{
        xxxx_trace(exxxx, eError, "can't create xxxx socket, idx=%d", index_xxxx);
        return RET_ERR;
    }

    int flags = fcntl(sockfd, F_GETFL);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    /* 填写sockaddr_in结构*/
	struct sockaddr_in addr;
	bzero(&addr,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port=htons(gxxxx[index_xxxx].port);
	addr.sin_addr.s_addr = inet_addr(gxxxx[index_xxxx].ip);

	int ret;
    if ((ret = connect(sockfd, (struct sockaddr *)&addr, sizeof(addr))) < 0)
	{
    	if (errno != EINPROGRESS)
    	{
    		xxxx_trace (exxxx, eError, "Connect failed\n");
    		do_xxxx_connect_failed(index_xxxx);
			return RET_ERR;
    	}
    }
	else if (ret == 0)
	{
		do_xxxx_connect_success(index_xxxx);
    }

    fd_set rset, wset;
    struct timeval tval;
    FD_ZERO(&rset);
    FD_SET(sockfd, &rset);
    wset = rset;
    tval.tv_sec = sec;
    tval.tv_usec = 0;

    int selres = select(sockfd + 1, &rset, &wset, NULL, &tval);
	switch (selres)
	{
	case 0:
		xxxx_trace (exxxx, eError, "Select timeout\n");
		do_xxxx_connect_failed(index_xxxx);
		return RET_ERR;
	case -1:
		xxxx_trace (exxxx, eError, "Select error\n");
		do_xxxx_connect_failed(index_xxxx);
		return RET_ERR;
	default:
		if (FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset))
		{
			int errinfo, errlen;
			if (0 == getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &errinfo, &errlen) && 0 == errinfo)
			{
				do_xxxx_connect_success(index_xxxx);
				return RET_OK;
			}
		}

		xxxx_trace (exxxx, eError, "Select invalid socket\n");
		do_xxxx_connect_failed(index_xxxx);
		return RET_ERR;
	}

	return RET_OK;
}

int xxxx_tcp_disconnect(int index_xxxx)
{
	if (index_xxxx < 0 || index_xxxx >= MAX_xxxx_NUM)
	{
		xxxx_trace(exxxx, eError,"Invalid xxxx index\n");
		return RET_ERR;
	}

	xxxx_async_select (exxxx, gxxxx[index_xxxx].fdCmdTcp, 0, xxxx_NULL, xxxx_NULL);
	gxxxx[index_xxxx].tcpConnected = 0;
	gxxxx[index_xxxx].status = 0;
	shutdown(gxxxx[index_xxxx].fdCmdTcp, SHUT_RDWR);
	close(gxxxx[index_xxxx].fdCmdTcp);
	gxxxx[index_xxxx].fdCmdTcp = -1;
	return RET_OK;
}

static void xxxx_tcp_xxxx_cmd_deal(int fd)
{
	char cmd_buf[1024];
	int idxxxxx = rtcp_get_rtp_index_by_fd(fd);
	if(idxxxxx == -1){
		xxxx_trace(exxxx, eError,"socket %d is not for xxxx", fd);
		return;
	}

	int recvLen = MAX_BUF_SIZE - gxxxx[idxxxxx].recv_buf_end + gxxxx[idxxxxx].recv_buf_begin;
	if (recvLen <= 0)
	{
		xxxx_trace(exxxx, eError,"socket %d, not enough buffer to recv for xxxx", fd);
		return;
	}

	char *recvBuf = gxxxx[idxxxxx].tcp_recv_buf + gxxxx[idxxxxx].recv_buf_end;
	int rlen = xxxx_tcp_recv(fd, recvBuf, recvLen);
	if (rlen <= 0)
	{
		xxxx_trace(exxxx, eError,"Faild to xxxx_tcp_recv, sock=%d\n", fd);
		xxxx_async_select (exxxx, fd, 0, xxxx_NULL, xxxx_NULL);
		close (gxxxx[idxxxxx].fdCmdTcp);
		gxxxx[idxxxxx].fdCmdTcp= -1;
		gxxxx[idxxxxx].tcpConnected = 0;
		gxxxx[idxxxxx].status = 0;
		return;
	}

	gxxxx[idxxxxx].recv_buf_end += rlen;
	
	int handle_len = gxxxx[idxxxxx].recv_buf_end - gxxxx[idxxxxx].recv_buf_begin;
	int headerLen = sizeof(int);
	while (handle_len > headerLen)
	{
		char* rbuf = gxxxx[idxxxxx].tcp_recv_buf + gxxxx[idxxxxx].recv_buf_begin;
		int msgLen = *((int*)rbuf);
		if (msgLen <= headerLen)
		{
			xxxx_trace(exxxx, eError,"Invalid msg len %d", msgLen);
			xxxx_async_select (exxxx, fd, 0, xxxx_NULL, xxxx_NULL);
			close(fd);
			return;
		}

		if (msgLen > handle_len) break;
		//处理命令
		//TODO： 兼容api拷贝， 后续优化
		memcpy(cmd_buf, rbuf+headerLen, msgLen-headerLen);
		cmd_buf[msgLen-headerLen] = '\0';
		xxxx_trace(exxxx, eWarn,"#####xxxx recv command, sock=%d len=%d, content=%s\n",
				fd, msgLen-headerLen, cmd_buf);
		xxxx_cmd_respond_deal(cmd_buf, msgLen-headerLen, gxxxx[idxxxxx].ipvalue);

		handle_len -= msgLen;
		if (handle_len == 0)
		{
			gxxxx[idxxxxx].recv_buf_begin = 0;
			gxxxx[idxxxxx].recv_buf_end = 0;
		}
		else
		{
			gxxxx[idxxxxx].recv_buf_begin += msgLen;
		}
	}

	return;
}

/return:-1/socket value
int xxxx_create_sock(char *ip, unsigned short port,int sock_type,int proto)
{
    struct sockaddr_in serv_addr;
    int  buffSize = 4096 * 1024 * 2;
    int  sock = -1;
	int optval;

    if ((sock = socket(AF_INET, sock_type, proto)) < 0)
    {
        xxxx_trace(exxxx, eError,"socket err: %s.", strerror(errno));
        return -1;
    }


	optval=1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR ,(void*)&optval, sizeof(optval)) == -1)
	{
		xxxx_trace(exxxx, eError,"setsockopt SO_REUSEADDR err: %s.", strerror(errno));
        return -1;
	}

    if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &buffSize, sizeof(buffSize)) < 0)
    {
		xxxx_trace(exxxx, eError,"setsockopt SO_RCVBUF err: %s.", strerror(errno));
        close(sock);
        return -1;
    }

    if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &buffSize, sizeof(buffSize)) < 0)
    {
		xxxx_trace(exxxx, eError,"setsockopt SO_RCVBUF err: %s.", strerror(errno));
        close(sock);
        return -1;
    }

	bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family      = AF_INET;
    serv_addr.sin_port        = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(ip);

    if (bind(sock,(struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        xxxx_trace(exxxx, eError,"bind ip:%s,port:%d err: %s.", ip,port,strerror(errno));
        close(sock);
        return -1;
    }

    xxxx_trace(exxxx, eSys,"create %s socket OK!ip:%s,port:%d.",sock_type == SOCK_STREAM ? "tcp":"udp",ip,port);

    return sock;
}

void tcp_accept()
{
		gTcpCmdSock = xxxx_create_sock(glocalIp,(unsigned short)gTcpCmdPort,SOCK_STREAM, IPPROTO_TCP);
		if(gTcpCmdSock < 0)
		{
			xxxx_trace(exxxx, eError,"xxxx_create_tcpsock err.");
			break;
		}

		if(listen(gTcpCmdSock, MAX_xxxx_NUM) == -1)
		{
			xxxx_trace(exxxx, eError,"listen err: %s.", strerror(errno));
			break;
		}

		if (xxxx_OK != xxxx_async_select (exxxx, gTcpCmdSock, xxxx_TCP_xxxx_CMD_CONNECT, xxxx_NULL, xxxx_NULL))
		{
			xxxx_trace (exxxx, eError, "select tcp call sock failed.");
			break;
		}

}

oid xxxx_tcp_xxxx_cmd_conn_deal(int fd)
{
	struct sockaddr_in xxxx_addr;
	socklen_t size = sizeof(xxxx_addr);
	memset(&xxxx_addr,0,size);

	int connect_sock = -1;
	connect_sock = accept(gTcpCmdSock, (struct sockaddr*)&xxxx_addr,&size);
	if(connect_sock < 0) {
		xxxx_trace(exxxx, eError,"socket accept failed: %s(%d).", strerror(errno), errno);
		return;
	}

	xxxx_trace(exxxx, eSys,"accept a new tcp cmd connection,from ip:%s port:%d",
			inet_ntoa(xxxx_addr.sin_addr), gTcpCmdPort);

	int index = xxxx_get_xxxx_index_by_ipvalue(xxxx_addr.sin_addr.s_addr);
	if (index < 0) {
		index = gxxxxNum;
	}
	else if (gxxxxInfoSet[index].fd != -1 && gxxxxInfoSet[index].fd != connect_sock){
		xxxx_trace(exxxx, eSys,"xxxx(ip:%s) reconnect now close old socket...",gxxxxInfoSet[index].ip_str);
		xxxx_async_select (exxxx, gxxxxInfoSet[index].fd, 0, xxxx_NULL, xxxx_NULL);
		close(gxxxxInfoSet[index].fd);
	}

    int flags = fcntl(connect_sock, F_GETFL);
    fcntl(connect_sock, F_SETFL, flags | O_NONBLOCK);
	inet_ntop(AF_INET, &((satosin(&xxxx_addr))->sin_addr), gxxxxInfoSet[index].ip_str, sizeof(gxxxxInfoSet[index].ip_str));
	gxxxxInfoSet[index].fd = connect_sock;
	gxxxxInfoSet[index].ip_val = xxxx_addr.sin_addr.s_addr;
	gxxxxInfoSet[index].port = gTcpCmdPort;
	gxxxxInfoSet[index].recv_buf_begin = 0;
	gxxxxInfoSet[index].recv_buf_end = 0;

	if (xxxx_OK != xxxx_async_select (exxxx, connect_sock, xxxx_TCP_xxxx_CMD_MSG, xxxx_NULL, xxxx_NULL))
	{
		xxxx_trace (exxxx, eError, "select tcp cmd sock failed.");
		close(connect_sock);
		gxxxxInfoSet[index].fd = -1;
		return;
	}

	gxxxxNum++;
	return;
}


#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <errno.h>
#include <strings.h>
#include <string.h>
int main(int argc, char **argv)
{
        int old;
        int err;
        int errlen = sizeof(err);
        struct sockaddr_in addr;
        socklen_t len = sizeof(struct sockaddr_in);
        int timeo;
        int fd; 
        struct pollfd pfd;
        int ret;

        if (argc != 4) {
                printf("Usage %s ip port timeout\n", argv[0]);
                return -1; 
        } 
        timeo = atoi(argv[3]);
        fd = socket(AF_INET, SOCK_STREAM, 0); 
        if (fd < 0) {
                printf("Create socket error erno=%d : %s\n", errno,strerror(errno));
                return -1; 
        } 

        bzero(&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(argv[1]);
        addr.sin_port = htons(atoi(argv[2]));

        old = fcntl(fd, F_GETFL, 0); 
        if (fcntl(fd, F_SETFL, old|O_NONBLOCK) < 0) {
                printf("setting fd error: errno,%s\n", errno,strerror(errno));
                close(fd);
                 return -1;
        }

        if (connect(fd, (struct sockaddr*)&addr, len) == -1 &&
                        errno != EINPROGRESS) {
                printf("Error in connect: %d,%s\n", errno,strerror(errno));
                close(fd);
                return -1;
        }

        pfd.fd = fd;
        pfd.events = POLLOUT | POLLERR | POLLHUP | POLLNVAL;
        errno = 0;

        if (timeo <= 0)
                timeo = -1;

        ret = poll(&pfd, 1, timeo);

        if (ret < 0) {
                if (errno == EINTR) {
                        close(fd);
                        return -1;
                }
                close(fd);
                return -1;

        } else if (0 == ret) {
                close(fd);
                return -1;
        }

        if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
                close(fd);
                return -1;
        }
        if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, (socklen_t*)&errlen) == -1) {
                close(fd);
                 return -1;
        }
        if (err != 0) {//check the SO_ERROR state
                errno = err;
                close(fd);
                return -1;
        }

        fcntl(fd, F_SETFL, old);
        printf("Connect real OK\n");
        return fd;
}


static void do_poll(int listenfd)
{
    int  connfd,sockfd;
    struct sockaddr_in cliaddr;
    socklen_t cliaddrlen;
    struct pollfd clientfds[OPEN_MAX];
    int maxi;
    int i;
    int nready;
    //添加监听描述符
    clientfds[0].fd = listenfd;
    clientfds[0].events = POLLIN;
    //初始化客户连接描述符
    for (i = 1;i < OPEN_MAX;i++)
        clientfds[i].fd = -1;
    maxi = 0;
    //循环处理
    for ( ; ; )
    {
        //获取可用描述符的个数
        nready = poll(clientfds,maxi+1,INFTIM);
        if (nready == -1)
        {
            perror("poll error:");
            exit(1);
        }
        //测试监听描述符是否准备好
        if (clientfds[0].revents & POLLIN)
        {
            cliaddrlen = sizeof(cliaddr);
            //接受新的连接
            if ((connfd = accept(listenfd,(struct sockaddr*)&cliaddr,&cliaddrlen)) == -1)
            {
                if (errno == EINTR)
                    continue;
                else
                {
                   perror("accept error:");
                   exit(1);
                }
            }
            fprintf(stdout,"accept a new client: %s:%d\n", inet_ntoa(cliaddr.sin_addr),cliaddr.sin_port);
            //将新的连接描述符添加到数组中
            for (i = 1;i < OPEN_MAX;i++)
            {
                if (clientfds[i].fd < 0)
                {
                    clientfds[i].fd = connfd;
                    break;
                }
            }
            if (i == OPEN_MAX)
            {
                fprintf(stderr,"too many clients.\n");
                exit(1);
            }
            //将新的描述符添加到读描述符集合中
            clientfds[i].events = POLLIN;
            //记录客户连接套接字的个数
            maxi = (i > maxi ? i : maxi);
            if (--nready <= 0)
                continue;
        }
        //处理客户连接
        handle_connection(clientfds,maxi);
    }
}

static void handle_connection(struct pollfd *connfds,int num)
{
    int i,n;
    char buf[MAXLINE];
    memset(buf,0,MAXLINE);
    for (i = 1;i <= num;i++)
    {
        if (connfds[i].fd < 0)
            continue;
        //测试客户描述符是否准备好
        if (connfds[i].revents & POLLIN)
        {
            //接收客户端发送的信息
            n = read(connfds[i].fd,buf,MAXLINE);
            if (n == 0)
            {
                close(connfds[i].fd);
                connfds[i].fd = -1;
                continue;
            }
           // printf("read msg is: ");
            write(STDOUT_FILENO,buf,n);
            //向客户端发送buf
            write(connfds[i].fd,buf,n);
        }
    }
}

