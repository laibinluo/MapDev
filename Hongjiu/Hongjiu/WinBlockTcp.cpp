#include "stdafx.h"
#include "WinBlockTcp.h"

#pragma comment(lib,"Ws2_32.lib")

CBlockTcp::CBlockTcp(SOCKET fd)
{
	m_sock = fd;
}
CBlockTcp::CBlockTcp()
{
	m_sock = INVALID_SOCKET;
}
CBlockTcp::~CBlockTcp()
{
	//Close();
}
///////////////////成员函数实现////////////////////////////


////////////////////////////////////////////////////////////////
bool CBlockTcp::Attach(SOCKET fd)
{ 
	bool ret = false;
	if (m_sock == INVALID_SOCKET)
	{
		m_sock = fd; 
		ret = true;
	}
	return ret;
}	

SOCKET CBlockTcp::Detach()
{
	SOCKET temp = m_sock;
	m_sock = INVALID_SOCKET;
	return temp;
}
 
int CBlockTcp::SetNoBlockMode(bool bNoBlock)
{
	unsigned long on = bNoBlock?1:0;
	return ioctlsocket(m_sock, FIONBIO, (unsigned long*)&on);
}

int CBlockTcp::CanRead(int nSecond, int nMiniSec)
{
	fd_set          set;
	struct timeval  waitTime;

	FD_ZERO(&set);
	FD_SET(m_sock, &set);	    
	
	waitTime.tv_sec = nSecond;
	waitTime.tv_usec = nMiniSec*1000;

	return select(0, &set, NULL, NULL, &waitTime);
}

int CBlockTcp::CanWrite(int nSecond, int nMiniSec)
{
	fd_set          set;
	struct timeval  waitTime;

	FD_ZERO(&set);
	FD_SET(m_sock, &set);	    
	
	waitTime.tv_sec = nSecond;
	waitTime.tv_usec = nMiniSec*1000;

	return select(0, NULL, &set, NULL, &waitTime);
}

int CBlockTcp::RecvUnknowLen(void *ptr, int nbytes, int nSecond)
{
	int             num = 0;

	if (nbytes > 0)
	{
		if (CanRead(nSecond, 0) <= 0)
		{
			num = -1;
		}
		else
		{
			num = recv(m_sock, (char *)ptr, nbytes, 0);
			if (SOCKET_ERROR == num)
			{
				int iId = WSAGetLastError();
			}
		}
	}
	return num;
}

int CBlockTcp::Recv(void *ptr, int nbytes, int nSecond)
{
	int		n;
	int     num = 0;

	while (nbytes > 0)
	{
		if ((n = RecvUnknowLen(ptr, nbytes, nSecond)) <= 0)
		{
			num = -1;
			break;
		}
		num += n;
		nbytes -= n;
		ptr = (char *)ptr + n;
	}

	return num;
}

///////////////////////////////////////////
int CBlockTcp::Send(const void *ptr, int nbytes, int nSecond)
{
	int 		    n;
	int             num = 0;

	while (nbytes > 0)
	{
		if (CanWrite(nSecond, 0) <= 0)
		{
			num = -1;
			break;
		}
		if (nbytes > BYTENUMPERSEND)
			n = BYTENUMPERSEND;
		else
			n = nbytes;

		if ((n = send(m_sock, (const char *)ptr, n, 0)) <= 0)
		{
			num = -1;
			break;
		}
		num += n;		
		nbytes -= n;
		ptr = (const char *)ptr + n;
	}

	return num;
}
/////////////////////////////////////////////
int CBlockTcp::ConnectTimeOut(SOCKADDR* pAddr, int len, int nSecond)
{
	//设置非阻塞方式连接
	SetNoBlockMode(true);
	int ret = connect(m_sock, pAddr, len);
	if(SOCKET_ERROR == ret)
	{
		int err = WSAGetLastError();
		if(WSAEWOULDBLOCK == err)
		{
			fd_set          set;
			struct timeval  waitTime;

			FD_ZERO(&set);
			FD_SET(m_sock, &set);	    
			waitTime.tv_sec = nSecond;
			waitTime.tv_usec = 0;
			ret = select(0, NULL, &set, NULL, &waitTime)==1 ? 0 : SOCKET_ERROR;
		}
	}
	SetNoBlockMode(false);
	return ret;
}

void CBlockTcp::Connect(const char *host, unsigned short port, int nSecond)
{
	struct sockaddr_in server;

	Close();
	server.sin_family = AF_INET;
    server.sin_port = htons(port);	

    server.sin_addr.s_addr = inet_addr(host);
	if (server.sin_addr.s_addr == INADDR_NONE)
    {
        struct hostent *inhost = gethostbyname(host);
        
		if (inhost)
		{
			for (int i = 0; inhost->h_addr_list[i]; i++)
			{
				m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				memcpy(&server.sin_addr, inhost->h_addr_list[i], inhost->h_length);
				if (m_sock != INVALID_SOCKET)
			    {
					if (nSecond > 0)
					{
						if (ConnectTimeOut((struct sockaddr *)&server, sizeof(server), nSecond) == 0)
						{
							break;
						}
					}
					else
					{
						if (connect(m_sock, (struct sockaddr *)&server, sizeof(server)) == 0)
						{
							break;
						}
					}
				}
				Close();
			}       
		}
    }
	else
	{
		m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (m_sock != INVALID_SOCKET)
		{
			if (nSecond > 0)
			{
				if (ConnectTimeOut((struct sockaddr *)&server, sizeof(server), nSecond) == SOCKET_ERROR)
				{
					Close();
				}
			}
			else
			{
				if (connect(m_sock, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR)
				{
					Close();
				}
			}
		}
	}

	return;
}
/////////////////////////////////////////////////////////
SOCKET CBlockTcp::Listen(const char *host, const short serv, int *addrlenp)
{
	int		 on = 1;
    struct sockaddr_in local;

    Close();
    m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if (m_sock == INVALID_SOCKET)
	{
		return INVALID_SOCKET;
	}
	if (setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&on, sizeof(on)) < 0)
	{
		Close();
		return INVALID_SOCKET;
	}

	local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_family = AF_INET;
    local.sin_port = htons(serv);

	if (bind(m_sock, (struct sockaddr *)&local, sizeof(local)) != 0)
    {
		Close();
    }
	else if (listen(m_sock, 100) < 0)
	{
		Close();	
	}

	return m_sock;	
}


/////////////////////////////////////////////////////////
void CBlockTcp::Close()
{
	if (m_sock != INVALID_SOCKET)
	{
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
	}
	
	return;
}

