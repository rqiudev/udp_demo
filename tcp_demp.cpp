
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

			OSAL_trace(eRTPC, eError,"Failed to recv tcp date, sock:%d, len = %u.", sock, 1);
			return -1;
		}
		else
		{
			OSAL_trace(eRTPC, eError,"The tcp may be closed, sock:%d", sock);
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

			OSAL_trace(eRTPC, eError,"Failed to recv tcp date, sock:%d, len = %u.", sock, len);
			return -1;
		} else {
			OSAL_trace(eRTPC, eError,"The tcp may be closed, sock:%d", sock);
			return 0;
		}
	}

	return nRecvTotalLen;
}



int second_tcp_recv(int sock, char* buf, int len)
{
	if (sock < 0 || buf == NULL || len <= 0)
	{
		OSAL_trace(eRTPP, eError,"Invalid arguments for tcp_recv");
	}

	int nRecv;
	while(len > 0)
	{
		nRecv = recv(sock, buf, len, 0);
		if(nRecv > 0)
		{
			return nRecv;
		} else if(nRecv < 0)
		{
			if(errno==EAGAIN || errno==EWOULDBLOCK || errno==EINTR)
			{
				usleep(10);
				continue;
			}

			OSAL_trace(eRTPP, eError,"Failed to recv tcp date, sock:%d, len = %u.", sock, len);
			return -1;
		} else
		{
			OSAL_trace(eRTPP, eError,"The tcp may be closed, sock:%d", sock);
			return 0;
		}
	}

	return nRecv;
}


void tcp_recv_deal(int fd)
{
	int recvLen = MAX_BUF_SIZE - gRtpcInfoSet[idxRtpc].recv_buf_end + gRtpcInfoSet[idxRtpc].recv_buf_begin;
	if (recvLen <= 0)
	{
		OSAL_trace(eRTPP, eError,"socket %d, not enough buffer to recv for rtpc", fd);
		return;
	}

	char *recvBuf = gRtpcInfoSet[idxRtpc].tcp_recv_buf + gRtpcInfoSet[idxRtpc].recv_buf_end;
	int rlen = tcp_recv(fd, recvBuf, recvLen);
	if (rlen <= 0)
	{
		OSAL_trace(eRTPP, eError,"Faild to rtpc_tcp_recv");
		OSAL_async_select (eRTPP, fd, 0, OSAL_NULL, OSAL_NULL);
		close(fd);
		return;
	}

	gRtpcInfoSet[idxRtpc].recv_buf_end += rlen;
	char* ip_str = gRtpcInfoSet[idxRtpc].ip_str;
	

	int handle_len = gRtpcInfoSet[idxRtpc].recv_buf_end - gRtpcInfoSet[idxRtpc].recv_buf_begin;
	while (handle_len > 0)
	{
		char* rbuf = gRtpcInfoSet[idxRtpc].tcp_recv_buf + gRtpcInfoSet[idxRtpc].recv_buf_begin;
		int msgLen = *rbuf;
		if (msgLen <= 1)
		{
			OSAL_trace(eRTPP, eError,"Invalid msg len %d", msgLen);
			OSAL_async_select (eRTPP, fd, 0, OSAL_NULL, OSAL_NULL);
			close(fd);
			return;
		}

		if (msgLen > handle_len) break;
		//处理命令
		rtpp_handle_command(cf, rbuf+1, msgLen-1, idxRtpc);

		handle_len -= msgLen;
		if (handle_len == 0)
		{
			gRtpcInfoSet[idxRtpc].recv_buf_begin = 0;
			gRtpcInfoSet[idxRtpc].recv_buf_end = 0;
		}
		else
		{
			gRtpcInfoSet[idxRtpc].recv_buf_begin += msgLen;
		}
	}

	return;
}


//remember to use first byte indicate the data length before send
int tcp_send(int sock, char* buf, int len)
{
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

			OSAL_trace(eRTPC, eError,"Failed to send tcp date, sock:%d, len = %u.", sock, len);
			return -1;
		}
	}
	return nSendTotalLen;
}

