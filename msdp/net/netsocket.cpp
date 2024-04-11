// netsocket.cpp
//


// #include <stropts.h>
#include <stdio.h>
#include <errno.h>
#include <iostream>
#include "netsocket.h"
CNetSocket::CNetSocket()
{
	m_bIsReceiveSocketOpened = false;
	m_bIsSendSocketOpened = false;
	m_nSocketReceive = 0;
	m_nSocketSend = 0;
	m_port = -1;
}

CNetSocket::~CNetSocket()
{
	if (m_bIsReceiveSocketOpened)
	{
		close(m_nSocketReceive);
		m_bIsReceiveSocketOpened = false;
	}

	if (m_bIsSendSocketOpened)
	{
		close(m_nSocketSend);
		m_bIsSendSocketOpened = false;
	}
}

bool CNetSocket::InitNetSocket(char m, char *ip, int port)
{
	mode = m;
	m_port = port;
	strcpy(m_IP, ip);
	if (m == 'R')
	{
		const int on = 1;
		m_nSocketReceive = socket(AF_INET, SOCK_DGRAM, 0);
		if (m_nSocketReceive <= 0)
		{
			printf("Create ReceiveSocket failed!\n");
			return false;
		}

		if (setsockopt(m_nSocketReceive, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) != 0)
		{
			printf("set receivesocket opt error!\n");
			return false;
		}
		int setrcvlen = 131070;
		if (setsockopt(m_nSocketReceive, SOL_SOCKET, SO_RCVBUF, (char *)&setrcvlen, sizeof(setrcvlen)) != 0)
		{
			printf("set receivesocket receive length error!\n");
			return false;
		}
		int revlen = 0;
		socklen_t aa = sizeof(revlen);
		if (getsockopt(m_nSocketReceive, SOL_SOCKET, SO_RCVBUF, (char *)&revlen, (socklen_t *)&aa) != 0)
			printf("get receivesocket opt error!\n");
		printf("func InitReceiveSocket invoked, socket fd: %d, receive port: %d\n", m_nSocketReceive, m_port);

		memset(&local_addin, 0, sizeof(local_addin));
		local_addin.sin_family = AF_INET;
		local_addin.sin_port = htons(m_port);
		local_addin.sin_addr.s_addr = htonl(INADDR_ANY);
		if (bind(m_nSocketReceive, (sockaddr *)&local_addin, sizeof(local_addin)) != 0)
		{
			printf("Bind receivesocket failed!\n");
			return false;
		}
		// memset(&opp_addin, 0, sizeof(sockaddr_in));
		m_bIsReceiveSocketOpened = true;
		return true;
	}
	else if (m == 'S')
	{
		const int on1 = 1;
		m_nSocketSend = socket(AF_INET, SOCK_DGRAM, 0);
		if (m_nSocketSend <= 0)
		{
			printf("Create SendSocket failed!\n");
			return false;
		}

		int flag1 = 1;

		if (ioctl(m_nSocketSend, FIONBIO, &flag1) == -1)
		// if (ioctlsocket(m_nSocketSend, FIONBIO, (unsigned long *)&flag1) == SOCKET_ERROR)
		{
			printf("set sendsocket options failed!\n");
			return false;
		}
		if (setsockopt(m_nSocketSend, SOL_SOCKET, SO_REUSEADDR | SO_BROADCAST, (char *)&on1, sizeof(on1)) != 0)
		{
			printf("set sendsocket opt error!\n");
			return false;
		}
		m_bIsSendSocketOpened = true;
		memset(&opp_addin, 0, sizeof(opp_addin));
		opp_addin.sin_family = AF_INET;
		opp_addin.sin_port = htons(m_port);
		// const char *addr = "192.168.1.136";
		opp_addin.sin_addr.s_addr = inet_addr(m_IP);
		// memset(&local_addin, 0, sizeof(sockaddr_in));
		printf("func InitSendSocket invoked, socket fd: %d, send port: %d\n", m_nSocketSend, m_port);
		return true;
	}
}

bool CNetSocket::InitNetMultiSocket(char m, char* ip, int port)
{
	mode = m;
	m_port = port;
	strcpy(m_IP, ip);
	if (m == 'R')
	{
		const int on = 1;
		m_nSocketReceive = socket(AF_INET, SOCK_DGRAM, 0);
		if (m_nSocketReceive <= 0)
		{
			printf("Create ReceiveSocket failed!\n");
			return false;
		}

		if (setsockopt(m_nSocketReceive, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) != 0)
		{
			printf("set receivesocket opt error!\n");
			return false;
		}
		int setrcvlen = 131070;
		if (setsockopt(m_nSocketReceive, SOL_SOCKET, SO_RCVBUF, (char *)&setrcvlen, sizeof(setrcvlen)) != 0)
		{
			printf("set receivesocket receive length error!\n");
			return false;
		}
		int revlen = 0;
		socklen_t aa = sizeof(revlen);
		if (getsockopt(m_nSocketReceive, SOL_SOCKET, SO_RCVBUF, (char *)&revlen, (socklen_t *)&aa) != 0)
			printf("get receivesocket opt error!\n");
		printf("func InitMultiReceiveSocket invoked, socket fd: %d, receive port: %d\n", m_nSocketReceive, m_port);
		m_bIsReceiveSocketOpened = true;

		// 组播实现
		struct ip_mreq m_mreq;
		m_mreq.imr_multiaddr.s_addr = inet_addr(m_IP);
		m_mreq.imr_interface.s_addr = htonl(INADDR_ANY);
		if(setsockopt(m_nSocketReceive, IPPROTO_IP, IP_ADD_MEMBERSHIP, &m_mreq, sizeof(m_mreq)) != 0) 
		{
			printf("set receivesocket multicast group error! errno:%d, error:%s\n", errno, strerror(errno));
			return false;
		}

		memset(&local_addin, 0, sizeof(local_addin));
		local_addin.sin_family = AF_INET;
		local_addin.sin_port = htons(m_port);
		local_addin.sin_addr.s_addr = htonl(INADDR_ANY);
		if (bind(m_nSocketReceive, (sockaddr *)&local_addin, sizeof(local_addin)) != 0)
		{
			printf("Bind receivesocket failed!\n");
			return false;
		}
		return true;
	}
	else if (m == 'S')
	{
		// 组播发送初始化与普通的单播发送没有区别，不需要改动
		const int on1 = 1;
		m_nSocketSend = socket(AF_INET, SOCK_DGRAM, 0);
		if (m_nSocketSend <= 0)
		{
			printf("Create SendSocket failed!\n");
			return false;
		}

		int flag1 = 1;

		if (ioctl(m_nSocketSend, FIONBIO, &flag1) == -1)
		// if (ioctlsocket(m_nSocketSend, FIONBIO, (unsigned long *)&flag1) == SOCKET_ERROR)
		{
			printf("set sendsocket options failed!\n");
			return false;
		}
		if (setsockopt(m_nSocketSend, SOL_SOCKET, SO_REUSEADDR | SO_BROADCAST, (char *)&on1, sizeof(on1)) != 0)
		{
			printf("set sendsocket opt error!\n");
			return false;
		}
		m_bIsSendSocketOpened = true;
		memset(&opp_addin, 0, sizeof(opp_addin));
		opp_addin.sin_family = AF_INET;
		opp_addin.sin_port = htons(m_port);
		// const char *addr = "192.168.1.136";
		opp_addin.sin_addr.s_addr = inet_addr(m_IP);
		// memset(&local_addin, 0, sizeof(sockaddr_in));
		printf("func InitMultiSendSocket invoked, socket fd: %d, send port: %d\n", m_nSocketSend, m_port);
		return true;
	}
}
int CNetSocket::ReceiveData(unsigned char *buf)
{
	short int len = 0;
	if (m_bIsReceiveSocketOpened)
	{
		socklen_t client_addin_len = sizeof(client_addin);
		// len = recvfrom(m_nSocketReceive, (char *)buf, 8192, 0, (sockaddr*)&client_addin, &client_addin_len); // todo debug 0218 FOUR_K -> 8192  如果没有数据就等待，阻塞在这一行
		len = recvfrom(m_nSocketReceive, (char *)buf, 8192,MSG_DONTWAIT,(sockaddr*)&client_addin, &client_addin_len); //todo debug 0218 FOUR_K -> 8192  没有数据也会返回一个值，代码往下走
		if (len == -1)
		{
			// std::cout<<"len = -1"<<std::endl;
			return 0;
		}
		else
		{
			// std::cout<<"len "<<std::endl;
			return len;
		}
	}
}

bool CNetSocket::SendData(unsigned char *buf, int buflength)
{
	if (m_bIsSendSocketOpened)
	{
		int len = 0;
		len = sendto(m_nSocketSend, buf, buflength, 0, (struct sockaddr *)&opp_addin, sizeof(sockaddr_in));
		// cout<<
		// printf("send dat len = %d\n", len);
		if (len == -1)
		{
			printf("send failed! errno[%d], error:%s\n", errno, strerror(errno));
			return false;
		}
		else
		{
			// printf("data len = %d\n", len);
			return true;
		}
	}
}
