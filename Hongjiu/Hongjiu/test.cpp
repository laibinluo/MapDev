// test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <Winsock2.h>
#include "WinBlockTcp.h"

#define MAXFILENAMELEN 512


#pragma pack(1)
////////请求长度
struct BASEASKHEAD
{
	unsigned int	length;	//包长度
	unsigned short	crc;	//crc
	unsigned short	xyh;	//协议号
	unsigned int	xlh;	//包序列号
	unsigned int	did;	//表did
};

////////响应长度
struct BASERESHEAD
{
	unsigned int	length;	//包长度
	unsigned short	crc;	//crc
	unsigned short	xyh;	//协议号
	unsigned int	xlh;	//包序列号
	unsigned int	tag;	//标志位为0 表示成功 否则失败
};
struct BASERESHEAD2
{
	unsigned int	length;	//包长度
	unsigned short	crc;	//crc
	unsigned int	rows;	//行数
	unsigned int	columns;//列数
};

#pragma pack()

unsigned int gnPackNo = 0;
bool InitInstance()
{	
	WSADATA             wsd;
	if (WSAStartup(MAKEWORD(2,2), &wsd) != 0)
    {
        printf("Failed to load Winsock library!\n");
		return false;
    }
	return true;
}

void ExitInstance()
{
	WSACleanup();
}

unsigned int FillString(const char* szString, char* pBuff)
{
	unsigned int len = 0;
	unsigned int nStrLen = strlen(szString);

	if(nStrLen < 128)
	{
		*(unsigned char*)pBuff++ = (unsigned char)nStrLen;
		memcpy(pBuff, szString, nStrLen);
		len = nStrLen+1;
	}
	else
	{
		*(unsigned char*)pBuff++ = (unsigned char)(nStrLen>>7)|0x80;
		*(unsigned char*)pBuff++ = (unsigned char)(nStrLen&0x7F);
		memcpy(pBuff, szString, nStrLen);
		len = nStrLen+2;
	}

	return len;
}

unsigned int ReadLenField(const char* szString, unsigned int nDataLen, unsigned int& nOffset)
{
	unsigned char value;
	unsigned int len = 0;

	for(nOffset = 0; nOffset<nDataLen && nOffset<5; nOffset++)
	{
		value = *(unsigned char*)(szString+nOffset);
		if(value < 128)
		{
			len = (len<<7)|value;
			nOffset++;
			break;
		}
		else
		{
			len = (len<<7)|(value&0x7F);
		}
	}

	return len;
}

CBlockTcp gsock;
const unsigned int gnBuffLen = 81920;
char gpBuff[gnBuffLen];
void RecvHotWineData()
{
	//////////
	//准备数据
	int length;
	int rest;
	unsigned int offset;
	unsigned int len = 0;
	char gpBuff[gnBuffLen];
	BASEASKHEAD* pAsk = (BASEASKHEAD*)gpBuff;
	char* point = (char*)(pAsk+1);
 
	pAsk->xyh = htons(10029);
	pAsk->xlh = htonl(++gnPackNo);
	pAsk->did = htonl(12502);

	len += FillString("12502", point);
	*(unsigned int*)(point+len) = htonl(1);
	len += sizeof(int);
	len += FillString("select C2, C3, C4, C5, C6, C7, C8, C9, C10 FROM ST12502_main LIMIT 5;", point+len);
	*(unsigned int*)(point+len) = htonl(1000);
	len += sizeof(int)+sizeof(BASEASKHEAD);

	pAsk->length = htonl(len);
	pAsk->crc = 0;
	//发送数据
	const char szServerIp[] = "10.9.2.174";
	const unsigned short nServerPort = 7000;

	gsock.Connect(szServerIp, nServerPort);
	if (gsock == INVALID_SOCKET)
		goto sockerr;
	if(gsock.Send(pAsk, len) != len)
		goto sockerr;
	//接收数据
	if(gsock.Recv(gpBuff, sizeof(BASERESHEAD))!=sizeof(BASERESHEAD))
		goto sockerr;
	BASERESHEAD* pRes = (BASERESHEAD*)gpBuff;
	pRes->length = htonl(pRes->length);
	length = pRes->length-sizeof(BASERESHEAD);
	if(pRes->length>sizeof(BASERESHEAD) && gsock.Recv(pRes+1, length)!=length)
		goto sockerr;

	if(!pRes->tag)
	{
		rest = ReadLenField(gpBuff+sizeof(BASERESHEAD), length, offset);
		if(offset && rest<=length-offset && rest>=sizeof(BASERESHEAD2))
		{
			char pOutBuf[128];
			BASERESHEAD2* pRes2 = (BASERESHEAD2*)(gpBuff+sizeof(BASERESHEAD)+offset);
			pRes2->rows = htonl(pRes2->rows);
			pRes2->columns = htonl(pRes2->columns);
			char* pData = (char*)(pRes2+1);
			rest -= sizeof(BASERESHEAD2);
			for(unsigned int i = 0, j; i < pRes2->rows && rest>0; i++)
			{
				printf("row = %d", i);
				for(j = 0; j < pRes2->columns && rest>0; j++)
				{			
					len = ReadLenField(pData, rest, offset);
					if(offset && len <= rest-offset)
					{
						if(len)
							memcpy(pOutBuf, pData+offset, len);
						pOutBuf[len] = '\0';
						printf(" ,%s", pOutBuf);
						pData += offset+len;//数据
						rest -= offset+len;

					}
					else
						goto err;

				}		
				printf("\n");
			}
		}

	}
	goto end;

sockerr:
	gsock.Close();
	;
err:
	;
end:
	;

}

//int main(int argc, char* argv[])
//{
//	if(!InitInstance())
//		return -1;
//	RecvHotWineData();
//
//	ExitInstance();
//	return 0;
//}

