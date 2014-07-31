// Hongjiu.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdlib.h>

#include <Winsock2.h>
#include "WinBlockTcp.h"
#include "RedWine.h"
#include "DataType.h"
#include "BuffServer.h"


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

char data[1024 * 1024] = {0};

#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <iostream>

using namespace std;

int _tmain(int argc, _TCHAR* argv[])
{
	if(!InitInstance())
		return -1;
	
	CRedWine wind;
	ZXCMDHEAD head = {0};
	head.m_wCmdType = ZXCMD_REDWINE_BASE_INFOR_DATA;
	head.m_nAttr = 2;
	head.m_nExData = 21;		
	
	int iDataLen = 0;
	int iRes = 0;
	char *pData = NULL;

	CRedWine::InitialRedWine();
	CRedWine::PrepareNetEnv();
	
	for (int i = 0; i < 4; i++)
	{
		if (CRedWine::RequestAllRemotetData())
		{
			break;
		}
	}

	CRedWine::ReleaseNetEnv();

	if (!CRedWine::HaveGotTotalData())
	{
		return false;
	}

	head.m_wCmdType = ZXCMD_REDWINE_BASE_INFOR_DATA;
	wind.ProcRedwineRequest(&head, &pData, iDataLen);
	ZXCMDHEAD *DataHead = (ZXCMDHEAD*)pData;	
	RedwineBaseInforTag *Item = (RedwineBaseInforTag*)(pData + sizeof(ZXCMDHEAD));
	RedwineBaseInforTag *Item2 = Item + 1;

	head.m_wCmdType = ZXCMD_REDWINE_MONEY_UNIT_DATA;
	wind.ProcRedwineRequest(&head, &pData, iDataLen);
	DataHead = (ZXCMDHEAD*)pData;	
	RedwineMoneyUnitTag* Item3 = (RedwineMoneyUnitTag*)(pData + sizeof(ZXCMDHEAD));
	RedwineMoneyUnitTag* Item4 = Item3 + 1;

	head.m_wCmdType = ZXCMD_REDWINE_SALE_ADDRESS_DATA;
	wind.ProcRedwineRequest(&head, &pData, iDataLen);
	DataHead = (ZXCMDHEAD*)pData;	
	RedwineSaleAddressTag* Item5 = (RedwineSaleAddressTag*)(pData + sizeof(ZXCMDHEAD));
	RedwineSaleAddressTag* Item6 = Item5 + 1;

	head.m_wCmdType = ZXCMD_REDWINE_AUCTION_HOUSE_DATA;
	wind.ProcRedwineRequest(&head, &pData, iDataLen);
	DataHead = (ZXCMDHEAD*)pData;	
	RedwineAuctionHouseTag* Item7 = (RedwineAuctionHouseTag*)(pData + sizeof(ZXCMDHEAD));
	RedwineAuctionHouseTag* Item8 = Item7 + 1;

	head.m_wCmdType = ZXCMD_REDWINE_AUCTION_SCHEDULE_DATA;
	wind.ProcRedwineRequest(&head, &pData, iDataLen);
	DataHead = (ZXCMDHEAD*)pData;	
	RedwineAuctionScheduleTag* Item9 = (RedwineAuctionScheduleTag*)(pData + sizeof(ZXCMDHEAD));
	RedwineAuctionScheduleTag* Item10 = Item9 + 1;

	head.m_wCmdType = ZXCMD_REDWINE_NAME_LIST_DATA;
	wind.ProcRedwineRequest(&head, &pData, iDataLen);
	DataHead = (ZXCMDHEAD*)pData;	
	RedwineNameTag* Item11 = (RedwineNameTag*)(pData + sizeof(ZXCMDHEAD));
	RedwineNameTag* Item12 = Item11 + 1;

	head.m_wCmdType = ZXCMD_REDWINE_LIV_MEMBER_DATA;
	wind.ProcRedwineRequest(&head, &pData, iDataLen);
	DataHead = (ZXCMDHEAD*)pData;	
	RedwineLivMemEnumTag* Item13 = (RedwineLivMemEnumTag*)(pData + sizeof(ZXCMDHEAD));
	RedwineLivMemEnumTag* Item14 = Item13 + 1;


	head.m_wCmdType = ZXCMD_REDWINE_RETAIL_MEMBER_DATA;
	wind.ProcRedwineRequest(&head, &pData, iDataLen);
	DataHead = (ZXCMDHEAD*)pData;	
	RedwineRetailMemEnumTag* Item17 = (RedwineRetailMemEnumTag*)(pData + sizeof(ZXCMDHEAD));
	RedwineRetailMemEnumTag* Item18 = Item17 + 1;

	head.m_wCmdType = ZXCMD_REDWINE_CHILDAREA_LIST;
	wind.ProcRedwineRequest(&head, &pData, iDataLen);
	DataHead = (ZXCMDHEAD*)pData;	
	RedWineChildAreaTag* Item19 = (RedWineChildAreaTag*)(pData + sizeof(ZXCMDHEAD));
	RedWineChildAreaTag* Item20 = Item19 + 1;

	head.m_wCmdType = ZXCMD_REDWINE_PARENTAREA_LIST;
	wind.ProcRedwineRequest(&head, &pData, iDataLen);
	DataHead = (ZXCMDHEAD*)pData;	
	RedWineParentAreaTag* Item21 = (RedWineParentAreaTag*)(pData + sizeof(ZXCMDHEAD));
	RedWineParentAreaTag* Item22 = Item21 + 1;

	head.m_wCmdType = ZXCMD_REDWINE_AREACLASSIFIED_LIST;
	wind.ProcRedwineRequest(&head, &pData, iDataLen);
	DataHead = (ZXCMDHEAD*)pData;	
	RedWineAreaClassifiedTag* Item23 = (RedWineAreaClassifiedTag*)(pData + sizeof(ZXCMDHEAD));
	RedWineAreaClassifiedTag* Item24 = Item23 + 1;

	head.m_wCmdType = ZXCMD_REDWINE_CRITIC_LIST;
	wind.ProcRedwineRequest(&head, &pData, iDataLen);
	DataHead = (ZXCMDHEAD*)pData;	
	RedWineCriticTag* Item25 = (RedWineCriticTag*)(pData + sizeof(ZXCMDHEAD));
	RedWineCriticTag* Item26 = Item25 + 1;
	
	head.m_nAttr = 2;
	head.m_nExData = 4;
	head.m_wCmdType = ZXCMD_REDWINE_INDEX_ELEMENT;
	wind.ProcRedwineRequest(&head, &pData, iDataLen);
	DataHead = (ZXCMDHEAD*)pData;	
	WORD* Item27 = (WORD*)(pData + sizeof(ZXCMDHEAD));
	WORD* Item28 = Item27 + 1;

	head.m_nAttr = 2;
	head.m_nExData = 4;
	head.m_wCmdType = ZXCMD_REDWINE_LIV_DAY_DATA;
	wind.ProcRedwineRequest(&head, &pData, iDataLen);
	DataHead = (ZXCMDHEAD*)pData;	
	RedwinePriceLivDayHistDataTag* Item29 = (RedwinePriceLivDayHistDataTag*)(pData + sizeof(ZXCMDHEAD));
	RedwinePriceLivDayHistDataTag* Item30 = Item29 + 1;

	head.m_nAttr = 2;
	head.m_nExData = 4;
	head.m_wCmdType = ZXCMD_REDWINE_LIV_MONTH_DATA;
	wind.ProcRedwineRequest(&head, &pData, iDataLen);
	DataHead = (ZXCMDHEAD*)pData;	
	RedwinePriceLivMonthHistDataTag* Item31 = (RedwinePriceLivMonthHistDataTag*)(pData + sizeof(ZXCMDHEAD));
	RedwinePriceLivMonthHistDataTag* Item32 = Item31 + 1;

	head.m_nAttr = 2;
	head.m_nExData = 4;
	head.m_wCmdType = ZXCMD_REDWINE_AUCTION_DAY_DATA;
	wind.ProcRedwineRequest(&head, &pData, iDataLen);
	DataHead = (ZXCMDHEAD*)pData;	
	RedwinePriceAuctionDayHistTag* Item33 = (RedwinePriceAuctionDayHistTag*)(pData + sizeof(ZXCMDHEAD));
	RedwinePriceAuctionDayHistTag* Item34 = Item33 + 1;

	head.m_nAttr = 2;
	head.m_nExData = 4;
	head.m_wCmdType = ZXCMD_REDWINE_AUCTION_MONTH_DATA;
	wind.ProcRedwineRequest(&head, &pData, iDataLen);
	DataHead = (ZXCMDHEAD*)pData;	
	RedwinePriceHistDataTag* Item35 = (RedwinePriceHistDataTag*)(pData + sizeof(ZXCMDHEAD));
	RedwinePriceHistDataTag* Item36 = Item35 + 1;

	head.m_nAttr = 2;
	head.m_nExData = 4;
	head.m_wCmdType = ZXCMD_REDWINE_RETAIL_MONTH_DATA;
	wind.ProcRedwineRequest(&head, &pData, iDataLen);
	DataHead = (ZXCMDHEAD*)pData;	
	RedwinePriceHistDataTag* Item37 = (RedwinePriceHistDataTag*)(pData + sizeof(ZXCMDHEAD));
	RedwinePriceHistDataTag* Item38 = Item37 + 1;

	head.m_wCmdType = ZXCMD_REDWINE_AUCTION_SEARCH_HIS_DATA;
	wind.ProcRedwineRequest(&head, &pData, iDataLen);
	DataHead = (ZXCMDHEAD*)pData;	
	RedwineAuctionResSearchDataTag* Item39 = (RedwineAuctionResSearchDataTag*)(pData + sizeof(ZXCMDHEAD));
	RedwineAuctionResSearchDataTag* Item40 = Item39 + 1;

	head.m_wCmdType = ZXCMD_REDWINE_RETAIL_LATEST_PRICE;
	wind.ProcRedwineRequest(&head, &pData, iDataLen);
	DataHead = (ZXCMDHEAD*)pData;	
	RedwineRetailLatestPriceTag* Item41 = (RedwineRetailLatestPriceTag*)(pData + sizeof(ZXCMDHEAD));
	RedwineRetailLatestPriceTag* Item42 = Item41 + 1;

	head.m_nExData = 1;
	head.m_nAttr = 3;	
	head.m_wCmdType = ZXCMD_REDWINE_AUCTION_SINGLE_TYPE_DATA;
	wind.ProcRedwineRequest(&head, &pData, iDataLen);
	DataHead = (ZXCMDHEAD*)pData;
	RedwineAuctBySingleTypeDataTag* Item43 = (RedwineAuctBySingleTypeDataTag*)(pData + sizeof(ZXCMDHEAD));
	RedwineAuctBySingleTypeDataTag* Item44 = Item43 + 1;
		
	ExitInstance();
	system("pause");
	return 0;
}

