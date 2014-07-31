#pragma once
#ifndef _CBLOCKTCPSOCK1
#define  _CBLOCKTCPSOCK1  1

#define  DEFAULTWAITMINISEC  20
#define  DEFAULTWAITRCVSEC   15
#define  DEFAULTWAITSNDSEC   15
#define  DEFAULTCONNECTSEC    0
#define  BYTENUMPERSEND    1460

#define  SENDALIVEINTERVAL    20000 
#define  RECVALIVEINTERVAL   120000


#include <Winsock2.h>
 
class CBlockTcp
{

public :
	CBlockTcp(SOCKET fd);
	CBlockTcp();
	~CBlockTcp();
	
	operator SOCKET(){ 	return m_sock; }
	bool Attach(SOCKET fd);	
	SOCKET Detach();

	int SetNoBlockMode(bool bNoBlock);// �� m_sock ���ó��첽��ʽ

	int CanWrite(int nSecond = 0, int nMiniSec = DEFAULTWAITMINISEC);
	int CanRead(int nSecond = 0, int nMiniSec = DEFAULTWAITMINISEC); // �ж��Ƿ������ݵ���
	int RecvUnknowLen(void *ptr, int nbytes, int nSecond = DEFAULTWAITRCVSEC);	
	int Recv(void *ptr, int nbytes, int nSecond = DEFAULTWAITRCVSEC);	
	int Send(const void *ptr, int nbytes, int nSecond = DEFAULTWAITSNDSEC);
	
	void Connect(const char *host, unsigned short port, int nSecond = DEFAULTCONNECTSEC);
	SOCKET Listen(const char *host, const short serv, int *addrlenp);
	
	SOCKET Accept(struct sockaddr * addr, int * addrlen){return accept(m_sock, addr, addrlen);}
		
	void Close();

private:
	int  ConnectTimeOut(SOCKADDR* pAddr, int len, int nSecond);
	SOCKET  m_sock;
};

#endif
