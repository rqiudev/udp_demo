
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
			return -1;
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
			return -1;
		}
	}

	return nRecvTotalLen;
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

