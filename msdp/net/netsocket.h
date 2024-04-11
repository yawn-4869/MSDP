#ifndef NET_H_H_H
#define NET_H_H_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/ioctl.h>

class CNetSocket
{
public:
	CNetSocket();
	~CNetSocket();
public:
	bool InitNetSocket(char m, char* ip, int port);
	bool InitNetMultiSocket(char m, char* ip, int port); // 组播
	bool SendData(unsigned char* buf,int buflength);
	int ReceiveData(unsigned char* buf);
	int GetReceiveFd() {
		return m_nSocketReceive;
	}
	int GetSendFd() {
		return m_nSocketSend;
	}
	int GetPort() {
		return m_port;
	}
	std::string getIp() {
		return std::string(m_IP);
	}
	sockaddr_in getClientAddr() {
		return client_addin;
	}

private:
	bool m_bIsReceiveSocketOpened;
	bool m_bIsSendSocketOpened;
	int m_nSocketReceive;
	int m_nSocketSend;
	char mode;
	int m_port;
	char m_IP[17];
	sockaddr_in local_addin;
	sockaddr_in opp_addin;
	sockaddr_in client_addin;
	// ip_mreq m_mreq; // 组播结构体
};

#endif