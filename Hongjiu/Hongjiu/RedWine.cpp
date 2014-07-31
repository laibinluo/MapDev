#include "StdAfx.h"
#include "RedWine.h"
#include "WinBlockTcp.h"
#include "DataType.h"

#include <stdio.h>
#include <stdlib.h>


using namespace std;

// 默认请求超时时间
const int sDefaultCacheTimeout = 1000 * 30;

#pragma pack(1)
// 包头
struct Head10029
{
	// 包长度(发送包的长度加上包头6个字节)
	unsigned int PackLength;
	//crc
	unsigned short Crc;
public:
	Head10029()
		: PackLength(0),
		Crc(0)
	{
	}
};


// 请求数据集
struct RequestPack
{
	// 协议命令字10029
	unsigned short Cmd;

	// 数据包序列号(用于标识该请求的序号,请求完成后会原样返回)
	unsigned int Seq;

	// sql执行涉及到的某个table id
	unsigned int TableId;

	// sql执行涉及到的所有table id list
	string TableIdList;

	// sql的数量
	unsigned int Count;

	// sql语句
	vector<string> SqlList;

	// cache超时时间
	unsigned int CacheTimeout;

public:
	RequestPack()
	{
		Clear();
	}

	// 清空
	void Clear()
	{
		Cmd = htons(10029);
		Seq = htonl((unsigned int)0);
		TableId = htonl(0);
		TableIdList.clear();
		Count = htonl(0);
		CacheTimeout = htonl(sDefaultCacheTimeout);
	}
};


// 响应数据集
struct ResponseDataBuf
{
	// 数据类型关键字
	std::string KeyType;
	// 数据起始地址
	char *DataBuf;
	// 数据长度
	int DataLength;
	// 数据类型长度
	int TypeLength;
	// 包含的分段数据总数（即head数）
	int SegCount;

public:
	ResponseDataBuf(const string &keytype, const char* data, const int totallength, const int typelen, const int count)
		: KeyType(keytype),DataBuf((char*)data), DataLength(totallength), TypeLength(typelen),SegCount(count)
	{
	}

	ResponseDataBuf & operator = (const ResponseDataBuf &item)
	{
		KeyType = item.KeyType;
		DataBuf = item.DataBuf;
		DataLength = item.DataLength;
		TypeLength = item.TypeLength;
		SegCount = item.SegCount;
		return *this;
	}

	// 获取指定索引的数据Head
	ZXCMDHEAD* GetIndexHead(const int index, int *OffSetToBegin = NULL)const
	{
		if (NULL == DataBuf || DataLength < 1 ||
			SegCount < 1 || TypeLength < 1 || index < 0 || index >= SegCount)
		{
			return NULL;
		}
		
		size_t iOffSet = 0;
		ZXCMDHEAD *pHead = NULL;
		for (int i = 0; i < index && i < SegCount; i++)
		{
			pHead = (ZXCMDHEAD*)(DataBuf + i * sizeof(ZXCMDHEAD) + iOffSet);
			if (NULL == pHead)
			{
				break;
			}
			iOffSet += pHead->m_nLen;
		}

		iOffSet += index * sizeof(ZXCMDHEAD);
		if (NULL != OffSetToBegin)
		{
			*OffSetToBegin = iOffSet;
		}
		return (ZXCMDHEAD*)(DataBuf + iOffSet);
	}
	
	// 访问索引数据(特定的segment内的数据)
	void* GetIndexData(const int MainIndex, const int SubIndex) const
	{
		int iOffSetToBegin = 0;
		ZXCMDHEAD *pHead = GetIndexHead(MainIndex, &iOffSetToBegin);
		if (NULL == pHead)
		{
			return NULL;
		}

		if (pHead->m_nLen < 1 || TypeLength < 1)
		{
			return NULL;
		}
		int iCount = pHead->m_nLen / TypeLength;
		if (SubIndex < 0 || SubIndex >= iCount)
		{
			return NULL;
		}
		
		return (DataBuf + iOffSetToBegin + sizeof(ZXCMDHEAD) + SubIndex * TypeLength);
	}

	// 返回区间的数据长度
	const int GetRangeLength(const int StartPos, const int EndPos)const
	{
		if (NULL == DataBuf || DataLength < 1 ||
			SegCount < 1 || TypeLength < 1 || StartPos < 0 || StartPos >= SegCount
			|| EndPos < 0 || EndPos >= SegCount || StartPos > EndPos)
		{
			return 0;
		}

		int iLenToBegin1 = 0, iLenToBegin2 = 0;
		ZXCMDHEAD *pHeadStart = GetIndexHead(StartPos, &iLenToBegin1);
		ZXCMDHEAD *pHeadEnd = GetIndexHead(EndPos, &iLenToBegin2);
		if (NULL == pHeadStart || NULL == pHeadEnd)
		{
			return 0;
		}

		return (iLenToBegin2 - iLenToBegin1 + pHeadEnd->m_nLen + sizeof(ZXCMDHEAD));
	}

	// 返回数据总长度
	const int GetDataLength()const
	{
		return DataLength;
	}

	// 返回索引数据区记录数目
	const int GetIndexDataCount(const int MainIndex)const
	{
		ZXCMDHEAD *pHead = GetIndexHead(MainIndex);
		if (NULL == pHead)
		{
			return 0;
		}

		if (pHead->m_nLen < 1 || TypeLength < 1)
		{
			return 0;
		}

		return pHead->m_nLen / TypeLength;
	}
};
struct BaseResHead
{
	// 头
	Head10029 head;

	// 协议命令字10029
	unsigned short Cmd;

	// 数据包序列号(用于标识该请求的序号,请求完成后会原样返回)
	unsigned int Seq;

	//请求的处理是否成功,0:处理成功,非0:处理失败
	unsigned int Result;
};

struct ResHead
{
	// 头
	Head10029 head;
	//行数
	unsigned int rows;
	//列数	
	unsigned int columns;
};


class CBinaryDataBuff
{
private:
	char *m_DataCache;
	const size_t m_iInitialSize; 
	size_t m_iCurrentPos;
	size_t m_iEndPos;
public:
	CBinaryDataBuff(void *cache, const size_t size)
		: m_DataCache((char*)cache),
		m_iInitialSize(size),
		m_iCurrentPos(0),
		m_iEndPos(0)
	{
	}

	void Clear()
	{
		if (NULL != m_DataCache)
		{
			memset(m_DataCache, 0, m_iInitialSize);
		}
		m_iCurrentPos = 0;
		m_iEndPos = 0;
	}

	bool PushBackString(const char *data, const int size)
	{
		if (m_iCurrentPos >= m_iInitialSize || NULL == m_DataCache
			|| (m_iCurrentPos + size) >= m_iInitialSize)
		{
			return false;
		}
		

		if(size < 128)
		{
			(*(unsigned char*)(m_DataCache + m_iCurrentPos)) = (unsigned char)size;
			m_iCurrentPos++;
			memcpy(m_DataCache + m_iCurrentPos, data, size);
		}
		else
		{
			(*(unsigned char*)(m_DataCache + m_iCurrentPos)) = (unsigned char)(size>>7)|0x80;
			m_iCurrentPos++;
			(*(unsigned char*)(m_DataCache + m_iCurrentPos)) = (unsigned char)(size&0x7F);
			m_iCurrentPos++;
			memcpy(m_DataCache + m_iCurrentPos, data, size);
		}

		m_iCurrentPos += size;
		UpateEndPos();

		return true;
	}

	bool PushBackInt(const int value)
	{
		if (m_iCurrentPos >= m_iInitialSize || NULL == m_DataCache
			|| (m_iCurrentPos + sizeof(int)) >= m_iInitialSize)
		{
			return false;
		}

		memcpy(m_DataCache + m_iCurrentPos, &value, sizeof(int));
		m_iCurrentPos += sizeof(int);
		UpateEndPos();
		return true;
	}

	bool PushBackShort(const short value)
	{
		if (m_iCurrentPos >= m_iInitialSize || NULL == m_DataCache
			|| (m_iCurrentPos + sizeof(short)) >= m_iInitialSize)
		{
			return false;
		}

		memcpy(m_DataCache + m_iCurrentPos, &value, sizeof(short));
		m_iCurrentPos += sizeof(short);
		UpateEndPos();
		return true;
	}

	bool SeekPos(const size_t pos)
	{
		if (pos < 0 || pos >= m_iInitialSize)
		{
			return false;
		}
		m_iCurrentPos = pos;
		UpateEndPos();
		return true;
	}

	bool GetInt(int &value)
	{
		value = 0;
		if (m_iCurrentPos>= 0 && (m_iCurrentPos + sizeof(int)) < m_iInitialSize)
		{
			memcpy(&value, m_DataCache + m_iCurrentPos, sizeof(int));
			return true;
		}
		return false;
	}

	bool GetShort(short &value)
	{
		value = 0;
		if (m_iCurrentPos>= 0 && (m_iCurrentPos + sizeof(short)) < m_iInitialSize)
		{
			memcpy(&value, m_DataCache + m_iCurrentPos, sizeof(short));
			return true;
		}
		return false;
	}

	const char *GetDataCache() const
	{
		return m_DataCache;
	}

	const int GetDataLength()const
	{
		return m_iEndPos;
	}

private:
	void UpateEndPos()
	{
		if (m_iCurrentPos > m_iEndPos)
		{
			m_iEndPos = m_iCurrentPos;
		}
	}
};
class CWriteBuff
{
private:
	// 数据缓存
	void *m_pBuf;
	// 输出初始位置
	const size_t m_iStartPos;
	// 缓存全部输出空间
	const size_t m_iTotalDataBufSize;
	// 数据当前位置
	size_t m_iCurrentPos;
	// 新输出的数据的长度
	size_t m_iDataLength;
	// 输出结束时的位置
	size_t m_iEndPos;

public:
	CWriteBuff(void *buf, const size_t BufSize, const size_t StartPos)
		: m_pBuf(buf),
		m_iStartPos(StartPos),
		m_iTotalDataBufSize(BufSize),
		m_iCurrentPos(m_iStartPos),
		m_iDataLength(0),
		m_iEndPos(StartPos)
	{
	}

	// 写入，并改变游标位置
	bool Push_back(void *data, const size_t len)
	{
		if (NULL == m_pBuf || NULL == data || m_iCurrentPos >= m_iTotalDataBufSize
			|| (m_iCurrentPos + len) > m_iTotalDataBufSize)
		{
			return false;
		}
		memcpy((char*)m_pBuf + m_iCurrentPos, data, len);
		m_iCurrentPos += len;
		m_iDataLength += len;
		m_iEndPos += len;
		return true;
	}

	// 写入，不改变位置信息
	bool Push_set(void *data, const size_t len)
	{
		if (NULL == m_pBuf || NULL == data || m_iCurrentPos >= m_iTotalDataBufSize
			|| (m_iCurrentPos + len) > m_iTotalDataBufSize)
		{
			return false;
		}
		memcpy((char*)m_pBuf + m_iCurrentPos, data, len);
		m_iDataLength += len;
		return true;
	}

	// 置位
	bool SeekPos(const size_t pos)
	{
		if (pos >= m_iTotalDataBufSize)
		{
			return false;
		}
		if (pos > m_iCurrentPos)
		{
			m_iEndPos = pos;
		}
		m_iCurrentPos = pos;
		return true;
	}

	// 置位到开始
	void SeekToBegin()
	{
		m_iCurrentPos = m_iStartPos;
	}

	// 置位到末尾
	void SeekToEnd()
	{
		m_iCurrentPos = m_iEndPos;
	}

	// 获取写入数据长度
	size_t GetNewdataLen() const
	{
		return m_iDataLength;
	}
};
// 响应数据解析器
class CResDataParser
{
private:
	// 数据缓存
	void *m_pBuf;
	// 数据长度
	const size_t m_iDataLength;
	// 请求的数据结构
	const RequestPack *m_pRequestPack;
	// 响应头
	ResHead *m_pResHead;
	// 响应数据
	char *m_pRespData;
	// 当前取数据位置
	size_t m_iCurrentGetDataPos;
	size_t m_iCurrentRestLen;
	// 当前行
	size_t m_iCurrentRow;
	// 当前列
	size_t m_iCurrentColumn;

public:
	CResDataParser(void *BufData, const size_t length, const RequestPack *RequestPack)
		: m_pBuf(BufData),
		m_iDataLength(length),
		m_pRequestPack(RequestPack),
		m_pResHead(NULL),
		m_pRespData(NULL),
		m_iCurrentGetDataPos(0),
		m_iCurrentRestLen(0),
		m_iCurrentRow(0),
		m_iCurrentColumn(0)
	{
	}

	unsigned int ReadLenField(const char* szString, size_t nDataLen, size_t &nOffset)
	{
		unsigned char value;
		unsigned int len = 0;

		for(nOffset = 0; nOffset < nDataLen && nOffset < 5; nOffset++)
		{
			value = *(unsigned char*)(szString + nOffset);
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

	bool CheckValid()
	{
		if (NULL == m_pBuf || NULL == m_pRequestPack)
		{
			return false;
		}
		
		size_t offset = 0;
				
		m_iCurrentRestLen = ReadLenField((char*)m_pBuf, m_iDataLength, offset);
		if(offset && m_iCurrentRestLen <= m_iDataLength - offset && m_iCurrentRestLen >= sizeof(ResHead))
		{
			m_pResHead = (ResHead*)((char*)m_pBuf + offset);
			if (NULL == m_pResHead)
			{
				return false;
			}
			m_pResHead->rows = htonl(m_pResHead->rows);
			m_pResHead->columns = htonl(m_pResHead->columns);

			m_pRespData = (char*)(m_pResHead + 1);
			m_iCurrentGetDataPos = 0;
			m_iCurrentRestLen -= sizeof(ResHead);
			m_iCurrentRow = 0;
			m_iCurrentColumn = 0;
			
			return true;
		}
		return false;
	}

	bool GetNext(std::string &Column)
	{
		if (NULL == m_pResHead || NULL == m_pRespData)
		{
			return false;
		}

		size_t iRow = m_pResHead->rows;
		size_t iColumn = m_pResHead->columns;
		size_t len = 0;
		size_t offset = 0;
		char pOutBuf[128] = {0};

		if (m_iCurrentRow < iRow && m_iCurrentColumn < iColumn && m_iCurrentRestLen > 0)
		{
			len = ReadLenField(m_pRespData + m_iCurrentGetDataPos, m_iCurrentRestLen, offset);

			if(offset && len <= m_iCurrentRestLen - offset)
			{
				if(len > 0)
				{
					memcpy(pOutBuf, m_pRespData + m_iCurrentGetDataPos + offset, len);
				}
				pOutBuf[len] = '\0';
				Column = string(pOutBuf);

				m_iCurrentGetDataPos += (offset + len);
				m_iCurrentRestLen -= (offset + len);

				// 更新行列信息
				m_iCurrentColumn++;
				if (m_iCurrentColumn == iColumn)
				{
					m_iCurrentRow++;
					m_iCurrentColumn = 0;
				}

				return true;
			}
			else 
			{
				return false;
			}
		}
		return false;
	}

	const size_t GetRecordRow()const
	{
		if (NULL != m_pResHead)
		{
			return m_pResHead->rows;
		}
		return 0;
	}

	const size_t GetRecordColumn()const
	{
		if (NULL != m_pResHead)
		{
			return m_pResHead->columns;
		}
		return 0;
	}

	bool IsEnd()
	{
		if (NULL != m_pResHead)
		{
			return true;
		}
		return GetRecordRow() == m_iCurrentRow && GetRecordColumn() == m_iCurrentColumn;
	}
};

// 大缓冲区
struct LargeMemoryCache
{
private:
	// 初始内存大小
	const int mInitialLen;
	// 内存区域
	char* mDataBuf;
	// 当前数据位置
	int mCurrentPos;
public:
	LargeMemoryCache(const int InitialLen)
		: mInitialLen(InitialLen),
		mDataBuf(NULL),
		mCurrentPos(0)
	{
		Initial();
	}

	virtual ~LargeMemoryCache()
	{
		if (NULL != mDataBuf)
		{
			delete []mDataBuf;
			mDataBuf = NULL;
		}
	}

	bool Initial()
	{
		if (mInitialLen <= 0)
		{
			return false;
		}
		if (NULL == mDataBuf)
		{
			mDataBuf = new char[mInitialLen];
			memset(mDataBuf, 0, mInitialLen);
		}
		return true;
	}

	void ClearAll()
	{
		if (NULL != mDataBuf)
		{
			memset(mDataBuf, 0, mInitialLen);
		}
		mCurrentPos = 0;
	}

	void ClearRange(const int startPos, const int length)
	{
		if (NULL != mDataBuf)
		{
			if (startPos >= 0 && startPos < mInitialLen && (startPos + length) <= mInitialLen)
			{
				memset(mDataBuf + startPos, 0, length);
			}
		}
	}

	// 获取固定大小的缓冲区
	char* GetAllocMem(const int length, int *warning = NULL)
	{
		char* outP = NULL;
		if (mCurrentPos >= 0 && mCurrentPos < mInitialLen && (mCurrentPos + length) <= mInitialLen)
		{
			outP = (char*)(mDataBuf + mCurrentPos);
		}
		mCurrentPos += length;
		if (mCurrentPos >= (mInitialLen - 256))
		{
			if (NULL != warning)
			{
				*warning = 1;
			}
		}
		return outP;
	}

	// 获取全部剩余缓冲区
	char* GetRemainMem(int &RemainLength, int *warning = NULL)
	{
		char* outP = NULL;
		if (mCurrentPos >= 0 && mCurrentPos < mInitialLen)
		{
			outP = (char*)(mDataBuf + mCurrentPos);
		}

		RemainLength = mInitialLen - mCurrentPos;
		if (mCurrentPos >= (mInitialLen - 256))
		{
			if (NULL != warning)
			{
				*warning = 1;
			}
		}
		return outP;
	}

	// 设置偏移量
	void SetRemainMemStart(const int offset, int *warning = NULL)
	{
		if (mCurrentPos >= 0 && mCurrentPos < mInitialLen && (mCurrentPos + offset) <= mInitialLen)
		{
			mCurrentPos += offset;
		}

		if (mCurrentPos >= (mInitialLen - 256))
		{
			if (NULL != warning)
			{
				*warning = 1;
			}
		}
	}

	// 获取剩余空闲容量
	const int GetUserMemory()const
	{
		return mCurrentPos;
	}

	// 清空剩余的数据部分
	void ClearRemainMem()const
	{
		if (NULL == mDataBuf)
		{
			return;
		}

		if (mCurrentPos >= 0 && mCurrentPos < mInitialLen && mInitialLen > 0)
		{
			memset(mDataBuf + mCurrentPos, 0, mInitialLen - mCurrentPos);
		}
	}
};
// 月统计数据
struct MonthStatmentTag
{
// 统计类型
int stType;
// head类型标识
int stHeadCmdType;
// 数据类型字符表示
string stKey;
};

#pragma pack()

// 常量定义
const char szServerIp[] = "10.15.107.158";				// 地址
const unsigned short nServerPort = 7000;			// 端口
const int MaxRequestSize = 1024 * 2;				// 请求数据最大长度
const int MaxResponseSize = 1024 * 1024 * 32;		// 响应数据长度
const int EmptyPackMark = -999;						// 空包标识
const int ZoomScaleNum = 100;						// 浮点数的放大倍数
const size_t MaxRecordPerCycle = 200000;			// 分次取数据时单次单次最大数据量
const size_t MaxCycleTimes = 100;					// 分次取数据最大循环次数
///---------------------------------------------------- /


// 响应数据缓存
static char SResponseDataCache[MaxResponseSize] = {0};
static LargeMemoryCache sLargeMemoryCache(1024 * 1024 * 50);
///---------------------------------------------------- /

// 解析定义
#define PARSE_D(tag)	if(!DataParser.GetNext(tag)){return false;}

//// 用于比较的相关定义//////////////////////////////////
// 红酒标识
struct RedWineMark
{
	string wNameCode;				// 名称代码
	WORD	wYear;					// 年份
	string	wCapacity;				// 容量
	WORD	wNumber;				// 规格数量
	WORD	nDayPriceMemCount;		// 日数据成员数量
	WORD	nMonthPriceMemCount;	// 月数据成员数量

public:
	RedWineMark(const string &NameCode, const WORD Year, const string &Capacity, const WORD Num,
		const WORD DayMemCount, const WORD MonthMemCount)
		: wNameCode(NameCode), wYear(Year), wCapacity(Capacity), wNumber(Num),
		nDayPriceMemCount(DayMemCount), nMonthPriceMemCount(MonthMemCount)
	{
	}

	RedWineMark()
	{
	}

	RedWineMark (const RedWineMark &item)
	{
		wNameCode = item.wNameCode;
		wYear = item.wYear;
		wCapacity = item.wCapacity;
		wNumber = item.wNumber;
		nDayPriceMemCount = item.nDayPriceMemCount;
		nMonthPriceMemCount = item.nMonthPriceMemCount;
	}

	RedWineMark& operator = (const RedWineMark &item)
	{
		wNameCode = item.wNameCode;
		wYear = item.wYear;
		wCapacity = item.wCapacity;
		wNumber = item.wNumber;
		nDayPriceMemCount = item.nDayPriceMemCount;
		nMonthPriceMemCount = item.nMonthPriceMemCount;
		return *this;
	}

	bool operator == (const RedWineMark &item)
	{
		if (wNameCode == item.wNameCode &&	wYear == item.wYear && wCapacity == item.wCapacity
			&& wNumber == item.wNumber)
		{
			return true;
		}
		return false;
	}
};


// 红酒名称
struct RedWineNameS
{
	std::string wNameCode;			// 名称代码
	std::string	wWineEngName;		// 红酒英文名称
	std::string	wWineChnName;		// 红酒中文名称
public:
	RedWineNameS(){}
	RedWineNameS(const std::string &NameCode, const std::string &EngName, const std::string &ChnName, const int index)
		: wNameCode(NameCode), wWineEngName(EngName), wWineChnName(ChnName)
	{
	}

	RedWineNameS& operator = (const RedWineNameS &item)
	{
		wNameCode = item.wNameCode;
		wWineEngName = item.wWineEngName;
		wWineChnName = item.wWineChnName;
		return *this;
	}
};

// 红酒容量
struct RedwineCapacityS
{
	std::string wCapacity;												// 红酒容量

public:
	RedwineCapacityS(){}
	RedwineCapacityS(const string &Capacity, const int index)
		: wCapacity(Capacity)
	{
	}

	RedwineCapacityS & operator = (const RedwineCapacityS &item)
	{
		wCapacity = item.wCapacity;
		return *this;
	}
};

// 帕克分数
struct RedwineParkRatinS
{
	DWORD nTime;				// 最近报价时间
	BYTE nHighRating;			// 最高分
	BYTE nLowRating;			// 最低分

public:
	RedwineParkRatinS(){}
	RedwineParkRatinS(const DWORD time, const BYTE hRating, const BYTE lRating)
		: nTime(time), nHighRating(hRating), nLowRating(lRating)
	{
	}
	RedwineParkRatinS & operator = (const RedwineParkRatinS &item)
	{
		nTime = item.nTime;
		nHighRating = item.nHighRating;
		nLowRating = item.nLowRating;
		return *this;
	}
};
// 拍卖查找结果
struct RedwineAuctionResMark
{
	string auAddr;			// 拍卖地点
	string auHouse;			// 拍卖行
	size_t count;			// 数量

public:
	RedwineAuctionResMark& operator = (const RedwineAuctionResMark &item)
	{
		auAddr = item.auAddr;
		auHouse = item.auHouse;
		count = item.count;
		return *this;
	}

	bool operator == (const RedwineAuctionResMark &item)
	{
		if (auAddr == item.auAddr && auHouse == item.auHouse)
		{
			return true;
		}
		return false;
	}
};


map<string, int> sParentAreaMap;										// 大产区Map
map<string, int> sChildAreaMap;											// 子产区Map
map<string, int> sCriticMap;											// 酒评家Map
map<string, int> sSaleAreaMap;											// 销售区域Map
typedef map<string, int>::const_iterator StringIntIter;
typedef pair<string, int> StringIntPair;

map<string, int> sRedWineNameUseStringKey;								// 红酒名称列表
map<int, RedWineNameS> sRedWineNameUseIntKey;							// 红酒名称列表

map<string, int> sRedwineCapacityUseStringKey;							// 红酒容量
map<int, RedwineCapacityS> sRedwineCapacityUseIntKey;					// 红酒容量

map<string,int> sRedwineBaseInforUseStringKey;							// 红酒基本信息
map<int, RedwineBaseInforTag> sRedwineBaseInforUseIntKey;				// 红酒基本信息

map<string, RedwineParkRatinS> sRedwineParkRatingMap;					// 红酒park分数

map<string, int> sRedWineLivMemStrKeyMap;								// 红酒Liv市场成分酒列表索引
map<int, RedWineMark> sRedWineLivMemIntKeyMap;							// 红酒Liv市场成分酒列表
typedef pair<int, RedWineMark> IntMarkPair;
typedef map<int, RedWineMark>::iterator IntMarkIter;
typedef map<string, int>* pRedWineMarkIntKeyMap;

map<string, int> sRedWineAuctionMemStrKeyMap;							// 红酒拍卖市场成分酒列表索引
map<int, RedWineMark> sRedWineAuctionMemIntKeyMap;						// 红酒拍卖市场成分酒列表

map<string, int> sRedWineRetailMemStrKeyMap;							// 红酒零售市场成分酒列表索引
map<int, RedWineMark> sRedWineRetailMemIntKeyMap;						// 红酒零售市场成分酒列表

map<string, ResponseDataBuf> sWineData;									// 红酒区分类型的各类数据集
typedef map<string, ResponseDataBuf>::const_iterator StringResIter;
typedef pair<string, ResponseDataBuf> StringResPair;

map<string, int> sRedWineSaleAddress;									// 红酒销售地点
map<string, int> sRedWineAuctionAddress;								// 红酒拍卖行

map<int, int> sRedWineMoneyUnit;										// 红酒货币单位

map<string, int> sRedWineAuctionResSrchMarkUseStrKeyMap;				// 红酒拍卖结果查询酒种类标识索引
map<int, RedWineMark> sRedWineAuctionResSrchMarkUseIntKeyMap;			// 红酒拍卖结果查询酒种类标识(日统计数目=记录条数)

map<string, int> sRedWineAuctionResAddHouseUseStrKeyMap;				// 红酒拍卖结果查询关键字索引
map<int, RedwineAuctionResMark> sRedWineAuctionResAddHouseUseIntKeyMap; // 红酒拍卖结果查询关键字
//
/////////////////////////////////////////////////////////

/// 取数据函数处理
typedef bool (*RequestDataFun)();										// 取数据函数指针
vector<RequestDataFun> sRequestFunVt;									// 取数据函数列表
//	-----------------------------------------------------------/

//静态成员初始化
CBlockTcp* CRedWine::m_pSocketServer = NULL;
bool CRedWine::m_bConnected = false;
int CRedWine::m_PackSerialNum = 0;
int CRedWine::m_LastestPackSerialNum = 0;
int CRedWine::m_iLastestErrorCode = EmptyPackMark;
CRedWine::CRedWine(void)
{
	
}

CRedWine::~CRedWine(void)
{
}

int 
CRedWine::InitialRedWine()
{
	if (NULL == m_pSocketServer)
	{
		m_pSocketServer = new CBlockTcp();
	}

	return 0;
}

void
CRedWine::ExitRedWineServer()
{
	if (NULL != m_pSocketServer)
	{
		m_pSocketServer->Close();
		delete m_pSocketServer;
		m_pSocketServer = NULL;
	}
}

// 发送请求数据
bool
CRedWine::SendRequestData(void *ReqData)
{
	RequestPack *pRequestPack = NULL;
	char cRequst[MaxRequestSize] = {0};
	CBinaryDataBuff DataS(cRequst, MaxRequestSize);
	Head10029 head;

	if (NULL == ReqData)
	{
		return false;
	}

	pRequestPack = (RequestPack*)ReqData;
	DataS.SeekPos(sizeof(Head10029));	
	DataS.PushBackShort(pRequestPack->Cmd);
	DataS.PushBackInt(pRequestPack->Seq);
	DataS.PushBackInt(pRequestPack->TableId);
	DataS.PushBackString(pRequestPack->TableIdList.c_str(), pRequestPack->TableIdList.length());
	DataS.PushBackInt(pRequestPack->Count);	
	for (size_t i = 0; i < pRequestPack->SqlList.size(); i++)
	{
		DataS.PushBackString(pRequestPack->SqlList[i].c_str(), pRequestPack->SqlList[i].length());
	}
	DataS.PushBackInt(pRequestPack->CacheTimeout);
	head.PackLength = htonl(DataS.GetDataLength());
	DataS.SeekPos(0);
	DataS.PushBackInt(head.PackLength);
	DataS.PushBackShort(head.Crc);

	if (!m_bConnected) 
	{
		m_pSocketServer->Connect(szServerIp, nServerPort);
		if (INVALID_SOCKET == (SOCKET)m_pSocketServer)
		{
			m_bConnected = false;
			return false;
		}
	}	

	return (DataS.GetDataLength() == m_pSocketServer->Send(DataS.GetDataCache(), DataS.GetDataLength()));
}

// 准备取数据环境
bool
CRedWine:: PrepareNetEnv()
{
	if (NULL == m_pSocketServer)
	{
		return false;
	}

	m_pSocketServer->Connect(szServerIp, nServerPort);
	if (INVALID_SOCKET == (SOCKET)m_pSocketServer)
	{
		m_bConnected = false;
	}
	else
	{
		m_bConnected = true;
	}

	InitialDatacache();
	RegisterRequestFun();
	return true;
}

// 释放取数据环境
void 
CRedWine::ReleaseNetEnv()
{
	if (NULL != m_pSocketServer)
	{
		m_pSocketServer->Close();
		m_bConnected = false;
	}
}

// 读取响应请求
int 
CRedWine::RecvResponseData()
{
	BaseResHead ResHead;
	memset(SResponseDataCache, 0, MaxResponseSize); 
	int iLength = 0;
	if (NULL == m_pSocketServer)
	{
		return -1;
	}

	if(m_pSocketServer->Recv(&ResHead, sizeof(BaseResHead), 180) != sizeof(BaseResHead))
	{
		return -2;
	}

	ResHead.head.PackLength = htonl(ResHead.head.PackLength);
	ResHead.Cmd = htons(ResHead.Cmd);
	ResHead.Seq = htonl(ResHead.Seq);
	ResHead.Result = htonl(ResHead.Result);

	iLength = ResHead.head.PackLength - sizeof(BaseResHead);
	if (0 == iLength)
	{
		return EmptyPackMark;
	}

	if (iLength >= MaxResponseSize)
	{
		printf("MaxResponseSize too small\n");
		return -3;
	}
	if (0 != ResHead.Result)
	{
		return -4;
	}

	if(ResHead.head.PackLength >sizeof(BaseResHead) && m_pSocketServer->Recv(SResponseDataCache, iLength) != iLength)
	{
		return -5;
	}

	return iLength;
}

// 数据区域初始化
void 
CRedWine::InitialDatacache()
{
	memset(SResponseDataCache, 0, MaxResponseSize);
	sParentAreaMap.clear();				
	sChildAreaMap.clear();
	sCriticMap.clear();
	sSaleAreaMap.clear();
	sRedWineNameUseStringKey.clear();
	sRedWineNameUseIntKey.clear();
	sRedWineMoneyUnit.clear();

	sRedWineAuctionResSrchMarkUseStrKeyMap.clear();
	sRedWineAuctionResSrchMarkUseIntKeyMap.clear();
	sRedWineAuctionResAddHouseUseStrKeyMap.clear();
	sRedWineAuctionResAddHouseUseIntKeyMap.clear();

	sRedwineCapacityUseStringKey.clear();
	sRedwineCapacityUseIntKey.clear();
	sRedwineBaseInforUseStringKey.clear();
	sRedwineBaseInforUseIntKey.clear();

	sRedwineParkRatingMap.clear();
	sRedWineLivMemStrKeyMap.clear();
	sRedWineLivMemIntKeyMap.clear();

	sRedWineAuctionMemStrKeyMap.clear();
	sRedWineAuctionMemIntKeyMap.clear();

	sRedWineRetailMemStrKeyMap.clear();
	sRedWineRetailMemIntKeyMap.clear();

	sWineData.clear();
	sRedWineSaleAddress.clear();
	sRedWineAuctionAddress.clear();

	sRedWineAuctionResAddHouseUseStrKeyMap.clear();
	sRedWineAuctionResAddHouseUseIntKeyMap.clear();
}

// 字符串转换成unsigned long
unsigned long 
CRedWine::StrToUl(const string & data)
{
	if (0 == data.length())
	{
		return 0;
	}
	return strtoul(data.c_str(), NULL, 10);
}

// 字符串转换成doulbe
double 
CRedWine::StrToDouble(const string & data)
{
	if (0 == data.length())
	{
		return 0;
	}
	return strtod(data.c_str(), NULL);
}

// 字符串拷贝
bool 
CRedWine::StrNCpy(char *buf, const int StartPos, const int MaxSize, const string & data)
{
	if (NULL == buf || StartPos < 0 || StartPos >= MaxSize)
	{
		return false;
	}
	if (0 == data.length())
	{
		memset(buf + StartPos, 0, MaxSize - StartPos);
	}
	strncpy(buf + StartPos, data.c_str(), MaxSize - StartPos);
	return true;
}

// 转换成int
int
CRedWine::StrToInt(const string & data)
{
	if (0 == data.length())
	{
		return 0;
	}
	return atoi(data.c_str());
}

// 是否为空串
bool 
CRedWine::IsEmptyString(const char* data)
{
	if (NULL == data)
	{
		return true;
	}
	return 0 == strlen(data);
}

// 设置货币单位到属性字段的最高位
bool 
CRedWine::SetHByteMoneyUnitToProperty(const unsigned char MoneyUnitValue, unsigned long &Property)
{
	size_t iMaxSize = sizeof(unsigned long);
	unsigned char cBaseValue = 0xFF;
	unsigned long uSetValue = MoneyUnitValue << ((iMaxSize - 1) * 8);
	unsigned long uMoveValue = 0;
	for (int i = iMaxSize - 2; i > -1; i--)
	{
		uMoveValue += cBaseValue << (i * 8);
	}
	Property &= uMoveValue;
	Property |= uSetValue;
	return true;
}

// 设置酒种类索引到属性字段的除最高位外的低位
bool 
CRedWine::SetExHByteSerialToProperty(const unsigned long MoneyUnitValue, unsigned long &Property)
{
	size_t iMaxSize = sizeof(unsigned long);
	unsigned char cBaseValue = 0xFF;
	unsigned long uMoveValue = cBaseValue << ((iMaxSize - 1) * 8);
	Property &= uMoveValue;
	Property |= MoneyUnitValue;
	return true;
}

// 获取特定的数据索引
// 类型关键字定义: 
//	'WineIndexList'					指数列表 
//	'WineIndexHistory'				指数历史数据
//	'RedWineCritic'					酒评家及机构数据
//	'RedWineAreaClassified'			产区分类数据
//	'RedWineParentArea'				红酒大产区数据
//	'RedWineChildArea'				红酒子产区数据
//	'EightWineries'					八大酒庄列表
//	'LivLastestPriceList'			Liv最新报价列表
//	'MemListIndex'					列表指数成分股索引数据
//	'RedWineSaleArea'				销售区域数据
//	'WineLivexDayPrice'				Liv价格周期为日的数据
//	'WineAuctionDayPrice'			拍卖价格周期为日的数据
//	'LivMonthPriceHisData'			按月统计Liv数据
//	'AuctionMonthPriceHisData'		按月统计拍卖数据
//	'RetailMonthPriceHisData'		按月统计零售数据
//	'RedwineMoneyUnit'				货币单位数据
//	'RedWineSaleAddress'			红酒销售地点数据
//	'RedWineAuctionHouse'			红酒拍卖行数据
//	'RedWineAuctionSchedule'		红酒拍卖会日程数据
//	'RedWineCapacityList'			红酒容量
//	'RedWineNameList'				红酒名称
//	'RedWineBaseInforList'			红酒基本信息
//	'RedWineLivMemList'				Liv市场的全部成分酒信息
//	'RedWineAutionMemList'			拍卖市场的全部成分酒信息
//	'RedWineRetailMemList'			零售市场全部成分酒
//	'RedwineRetailLatestPrice'		零售最新报价
//	'RedWineClassifiedNameListIndex'红酒按照市场分类的名称列表列表索引
//	'AuctionResSearchMemEnum'		拍卖结果查询酒标识列表
//	'AuctionResSearchKeyList'		拍卖结果查询关键字
//	'AuctionResSearchDataList'		拍卖结果查询全数据
//	'AuctionResByWineTypeDataList'	拍卖按照红酒种类归类查询全数据
const ResponseDataBuf * 
CRedWine::GetTypeWineData(const string &type)
{
	if (sWineData.size() < 1)
	{
		return NULL;
	}

	StringResIter findInterator = sWineData.find(type);
	if (findInterator != sWineData.end())
	{
		return &(findInterator->second);
	}
	return NULL;
}

// 获取发送序列号
int 
CRedWine::GetSendSeqNum(const int type)
{
	int iOut = 0;
	if (0 == type)
	{
		iOut = m_PackSerialNum;
		m_PackSerialNum++;
		if (m_PackSerialNum >= 2147483647)
		{
			m_PackSerialNum = 0;
		}

	}
	else 
	{
		iOut = type;
	}

	m_LastestPackSerialNum = iOut;
	return iOut;
}

// 获取最新包序列
int 
CRedWine::GetLastestSeqNum()
{
	return m_LastestPackSerialNum;
}


// 请求酒类指数列表
bool 
CRedWine::RequestWineIndexList_S()
{
	RequestPack request;
	int iLength = 0;

	// 初始化请求数据
	request.Seq = htonl(GetSendSeqNum());
	request.TableId = htonl(12501);
	request.TableIdList = "12501";
	request.Count = htonl(1);
	request.SqlList.push_back("SELECT DISTINCT CO, C2 FROM ST12501_main ORDER BY C2 ASC;");

	// 接收数据结构
	int iMaxLength = 0;
	int iWarning = 0;
	char *pStorteData = sLargeMemoryCache.GetRemainMem(iMaxLength, &iWarning);
	if (NULL == pStorteData || iWarning > 0)
	{
		return false;
	}
	ResponseDataBuf RespData(string("WineIndexList"), pStorteData, iMaxLength, sizeof(RedwineIndexListTag), 1);
	CWriteBuff bufWrite((char*)RespData.DataBuf, RespData.GetDataLength(), 0);

	if (SendRequestData(&request))
	{
		if ((iLength = RecvResponseData()) > 0)
		{
			if (!ParseWineIndexList(&request, SResponseDataCache, iLength, &bufWrite))
			{
				sLargeMemoryCache.ClearRemainMem();
				return false;
			}
			sLargeMemoryCache.SetRemainMemStart(bufWrite.GetNewdataLen());
			RespData.DataLength = bufWrite.GetNewdataLen();
			sWineData.insert(StringResPair(RespData.KeyType, RespData));
			return true;
		}
	}

	return false;
}

// 解析红酒指数列表
bool 
CRedWine::ParseWineIndexList(void *ReqData, char *data, const int length, CWriteBuff *bufWrite)
{
	if (NULL == ReqData || NULL == data	|| NULL == bufWrite)
	{
		return false;
	}
	
	CResDataParser DataParser(data, length, (RequestPack*)ReqData);
	if (!DataParser.CheckValid())
	{
		return false;
	}

	string strColumn;
	int iRow = DataParser.GetRecordRow();
	int iDatalength = sizeof(RedwineIndexListTag) * iRow;
		
	// Head
	ZXCMDHEAD DataHead;
	DataHead.m_wCmdType = ZXCMD_REDWINE_INDEX_LIST;
	DataHead.m_nAttr = 0;
	SetExHByteSerialToProperty(0, DataHead.m_nExData);
	DataHead.m_nLen = iDatalength;
	if (!bufWrite->Push_back(&DataHead, sizeof(ZXCMDHEAD)))
	{
		return false;
	}

	RedwineIndexListTag item;
	for (int i = 0; i < iRow; i++)
	{
		PARSE_D(strColumn)
		StrNCpy(item.wIndexCode, 0, REDWINE_INDEX_NAME_LEN, strColumn);
		
		PARSE_D(strColumn)
		StrNCpy(item.wIndexChnName, 0, REDWINE_INDEX_NAME_LEN, strColumn);

		if (!bufWrite->Push_back(&item, sizeof(RedwineIndexListTag)))
		{
			return false;
		}
	}

	return true;
}

// 请求酒类指数分类数据
bool
CRedWine::RequestWineIndexClassified(const int TotalCount, const int Serial, const char* KeyWords,
									 const int IndexPeriod, CWriteBuff *bufWrite)
{
	RequestPack request;
	int iLength = 0;
	string strSql("");
	char cConditon[128] = {0};

	sprintf(cConditon, " CO = '%s' AND C4 = %d ", KeyWords, IndexPeriod);

	// 初始化请求数据
	request.Seq = htonl(GetSendSeqNum());
	request.TableId = htonl(12501);
	request.TableIdList = "12501";
	request.Count = htonl(1);
	strSql = "SELECT CO, DATE_FORMAT(C1, '%Y%m%d') AS TIME, C2, C3, C4 FROM ST12501_main WHERE " +
	string(cConditon) + 
	" ORDER BY C1 ASC;";
	request.SqlList.push_back(strSql);

	if (SendRequestData(&request))
	{
		if ((iLength = RecvResponseData()) > 0)
		{
			if (ParseWineIndexClassified(TotalCount, Serial, KeyWords, &request, SResponseDataCache, 
				iLength, bufWrite))
			{
				return true;
			}
		}
	}

	return false;
}

// 解析红酒指数历史数据
bool 
CRedWine::ParseWineIndexClassified(const int TotalCount, const int Serial, const char* KeyWords, 
								   void *ReqData, char *data, const int length, CWriteBuff *bufWrite)
{
	if (NULL == ReqData || NULL == data || NULL == bufWrite)
	{
		return false;
	}

	CResDataParser DataParser(data, length, (RequestPack*)ReqData);
	if (!DataParser.CheckValid())
	{
		return false;
	}

	string strColumn;
	int iRow = DataParser.GetRecordRow();
	int iTypeSize = sizeof(RedwineIndexHistTag);
	int iDatalength = iTypeSize * iRow;
		
	// Head
	ZXCMDHEAD DataHead;
	DataHead.m_wCmdType = ZXCMD_REDWINE_INDEX_HIS_DATA;
	DataHead.m_nAttr = Serial == (TotalCount - 1) ? 0 : TotalCount;
	SetExHByteSerialToProperty(Serial, DataHead.m_nExData);
	DataHead.m_nLen = iDatalength;
	if (!bufWrite->Push_back(&DataHead, sizeof(ZXCMDHEAD)))
	{
		return false;
	}

	RedwineIndexHistTag item;
	for (int i = 0; i < iRow; i++)
	{
		PARSE_D(strColumn)

		PARSE_D(strColumn)
		item.m_nDate = StrToUl(strColumn);

		PARSE_D(strColumn)

		PARSE_D(strColumn)
		item.m_nValue = (DWORD)(StrToDouble(strColumn) * ZoomScaleNum);

		PARSE_D(strColumn)

		if (!bufWrite->Push_back(&item, iTypeSize))
		{
			return false;
		}
	}
	
	return true;
}

// 请求酒类指数全数据
bool 
CRedWine::GetWineIndexData_S()
{
	// 访问列表数据
	const ResponseDataBuf *pBaseRespData = GetTypeWineData("WineIndexList");
	if (NULL == pBaseRespData)
	{
		return false;
	}
	
	// 数据量
	int iDataCount = pBaseRespData->GetIndexDataCount(0);
	if (iDataCount < 1)
	{
		return false;
	}

	// 接收数据结构
	int iMaxLength = 0;
	int iWarning = 0;
	char *pStorteData = sLargeMemoryCache.GetRemainMem(iMaxLength, &iWarning);
	if (NULL == pStorteData || iWarning > 0)
	{
		return false;
	}
	ResponseDataBuf RespData(string("WineIndexHistory"), pStorteData, iMaxLength, sizeof(RedwineIndexHistTag), iDataCount);
	CWriteBuff bufWrite((char*)RespData.DataBuf, RespData.GetDataLength(), 0);
	
	// 类型定义
	const int ReqType[5] = {1, 1, 3, 1, 2};
	// 按照指数列表数据取全部指数数据
	for (int i = 0; i < iDataCount; i++)
	{
		RedwineIndexListTag *pData = (RedwineIndexListTag*)pBaseRespData->GetIndexData(0, i);
		if (NULL == pData || IsEmptyString(pData->wIndexCode))
		{
			sLargeMemoryCache.ClearRemainMem();
			return false;
		}
		if (RequestWineIndexClassified(iDataCount, i, pData->wIndexCode, (i < 5 ? ReqType[i] : 1),
			&bufWrite))
		{
			continue;
		}
		else 
		{
			sLargeMemoryCache.ClearRemainMem();
			return false;
		}
	}
	sLargeMemoryCache.SetRemainMemStart(bufWrite.GetNewdataLen(), &iWarning);
	RespData.DataLength = bufWrite.GetNewdataLen();
	sWineData.insert(StringResPair(RespData.KeyType, RespData));

	return true;
}

// 请求酒评家及机构数据
bool 
CRedWine::RequestRedWineCritic_S()
{
	RequestPack request;
	int iLength = 0;

	// 初始化请求数据
	request.Seq = htonl(GetSendSeqNum());
	request.TableId = htonl(12519);
	request.TableIdList = "12519";
	request.Count = htonl(1);
	request.SqlList.push_back("SELECT DISTINCT C3 FROM ST12519_main ORDER BY C3 ASC;");

	// 接收数据结构
	int iMaxLength = 0;
	int iWarning = 0;
	char *pStorteData = sLargeMemoryCache.GetRemainMem(iMaxLength, &iWarning);
	if (NULL == pStorteData || iWarning > 0)
	{
		return false;
	}
	ResponseDataBuf RespData(string("RedWineCritic"), pStorteData, iMaxLength, sizeof(RedWineCriticTag), 1);
	CWriteBuff bufWrite((char*)RespData.DataBuf, RespData.GetDataLength(), 0);

	if (SendRequestData(&request))
	{
		if ((iLength = RecvResponseData()) > 0)
		{
			if (!ParseRedWineCritic(&request, SResponseDataCache, iLength, &bufWrite))
			{
				sLargeMemoryCache.ClearRemainMem();
				return false;
			}
			sLargeMemoryCache.SetRemainMemStart(bufWrite.GetNewdataLen());
			RespData.DataLength = bufWrite.GetNewdataLen();
			sWineData.insert(StringResPair(RespData.KeyType, RespData));
			return true;
		}
	}

	return false;
}

// 解析酒评家及机构数据
bool 
CRedWine::ParseRedWineCritic(void *ReqData, char *data, const int length, CWriteBuff *bufWrite)
{
	if (NULL == ReqData || NULL == data	|| NULL == bufWrite)
	{
		return false;
	}
	
	CResDataParser DataParser(data, length, (RequestPack*)ReqData);
	if (!DataParser.CheckValid())
	{
		return false;
	}

	string strColumn;
	int iRow = DataParser.GetRecordRow();
	int iDatalength = sizeof(RedWineCriticTag) * iRow;
		
	// Head
	ZXCMDHEAD DataHead;
	DataHead.m_wCmdType = ZXCMD_REDWINE_CRITIC_LIST;
	DataHead.m_nAttr = 0;
	SetExHByteSerialToProperty(0, DataHead.m_nExData);
	DataHead.m_nLen = iDatalength;
	if (!bufWrite->Push_back(&DataHead, sizeof(ZXCMDHEAD)))
	{
		return false;
	}

	RedWineCriticTag item;
	sCriticMap.clear();
	for (int i = 0; i < iRow; i++)
	{
		PARSE_D(strColumn)
		StrNCpy(item.wCriticName, 0, REDWINE_CRITIC_NAME_LEN, strColumn);
		sCriticMap.insert(StringIntPair(strColumn, i));
		
		if (!bufWrite->Push_back(&item, sizeof(RedWineCriticTag)))
		{
			return false;
		}
	}

	return true;
}

// 查找指定的酒评家
int 
CRedWine::FindCritic(const std::string &Key)
{
	if (sCriticMap.size() < 1)
	{
		return -1;
	}

	StringIntIter findInterator = sCriticMap.find(Key);
	if (findInterator != sCriticMap.end())
	{
		return findInterator->second;
	}
	return -1;
}

// 请求红酒产区分类数据
bool 
CRedWine::RequestWineAreaClassified_S()
{
	// 接收数据结构
	int iMaxLength = 0;
	int iWarning = 0;
	char *pStorteData = sLargeMemoryCache.GetRemainMem(iMaxLength, &iWarning);
	if (NULL == pStorteData || iWarning > 0)
	{
		return false;
	}
	ResponseDataBuf RespData(string("RedWineAreaClassified"), pStorteData, iMaxLength, sizeof(RedWineAreaClassifiedTag), 1);
	CWriteBuff bufWrite((char*)RespData.DataBuf, RespData.GetDataLength(), 0);

	int iRow = 10;
	int iDatalength = sizeof(RedWineAreaClassifiedTag) * iRow;
		
	// Head
	ZXCMDHEAD DataHead;
	DataHead.m_wCmdType = ZXCMD_REDWINE_AREACLASSIFIED_LIST;
	DataHead.m_nAttr = 0;
	SetExHByteSerialToProperty(0, DataHead.m_nExData);
	DataHead.m_nLen = iDatalength;
	if (!bufWrite.Push_back(&DataHead, sizeof(ZXCMDHEAD)))
	{
		sLargeMemoryCache.ClearRemainMem();
		return false;
	}

	RedWineAreaClassifiedTag item;
	string strColumn[] = {"Italy", "Bordeaux (Red)", "Bordeaux (White)", "Bordeaux (Sweet White)","Burgundy (Red)",
					"Burgundy (White)",	"Rhone Valley",	"Champagne", "Port Vintage", "New World"};
	for (int i = 0; i < 10; i++)
	{
		StrNCpy(item.wClassifiedName, 0, REDWINE_AREACLASSIFIED_NAME_LEN, strColumn[i]);
		if (!bufWrite.Push_back(&item, sizeof(RedWineAreaClassifiedTag)))
		{
			sLargeMemoryCache.ClearRemainMem();
			return false;
		}
	}	

	sLargeMemoryCache.SetRemainMemStart(bufWrite.GetNewdataLen());
	RespData.DataLength = bufWrite.GetNewdataLen();
	sWineData.insert(StringResPair(RespData.KeyType, RespData));
	return true;
}

// 查找产区分类索引数据
int 
CRedWine::FindAreaClassified(const std::string &Key)
{
	return StrToInt(Key) - 1;
}

// 请求红酒大产区数据
bool 
CRedWine::RequestRedWineParentArea_S()
{
	RequestPack request;
	int iLength = 0;

	// 初始化请求数据
	request.Seq = htonl(GetSendSeqNum());
	request.TableId = htonl(12511);
	request.TableIdList = "12511";
	request.Count = htonl(1);
	request.SqlList.push_back("SELECT DISTINCT C4 FROM ST12511_main WHERE C4 != '' ORDER BY C4 ASC;");

	// 接收数据结构
	int iMaxLength = 0;
	int iWarning = 0;
	char *pStorteData = sLargeMemoryCache.GetRemainMem(iMaxLength, &iWarning);
	if (NULL == pStorteData || iWarning > 0)
	{
		return false;
	}
	ResponseDataBuf RespData(string("RedWineParentArea"), pStorteData, iMaxLength, sizeof(RedWineParentAreaTag), 1);
	CWriteBuff bufWrite((char*)RespData.DataBuf, RespData.GetDataLength(), 0);

	if (SendRequestData(&request))
	{
		if ((iLength = RecvResponseData()) > 0)
		{
			if (!ParseRedWineParentArea(&request, SResponseDataCache, iLength, &bufWrite))
			{
				sLargeMemoryCache.ClearRemainMem();
				return false;
			}
			sLargeMemoryCache.SetRemainMemStart(bufWrite.GetNewdataLen());
			RespData.DataLength = bufWrite.GetNewdataLen();
			sWineData.insert(StringResPair(RespData.KeyType, RespData));
			return true;
		}
	}

	return false;
}

// 解析红酒大产区数据
bool
CRedWine::ParseRedWineParentArea(void *ReqData, char *data, const int length, CWriteBuff *bufWrite)
{
	if (NULL == ReqData || NULL == data	|| NULL == bufWrite)
	{
		return false;
	}
	
	CResDataParser DataParser(data, length, (RequestPack*)ReqData);
	if (!DataParser.CheckValid())
	{
		return false;
	}

	string strColumn;
	int iRow = DataParser.GetRecordRow();
	int iDatalength = sizeof(RedWineParentAreaTag) * iRow;
		
	// Head
	ZXCMDHEAD DataHead;
	DataHead.m_wCmdType = ZXCMD_REDWINE_PARENTAREA_LIST;
	DataHead.m_nAttr = 0;
	SetExHByteSerialToProperty(0, DataHead.m_nExData);
	DataHead.m_nLen = iDatalength;
	if (!bufWrite->Push_back(&DataHead, sizeof(ZXCMDHEAD)))
	{
		return false;
	}

	RedWineParentAreaTag item;
	sParentAreaMap.clear();
	for (int i = 0; i < iRow; i++)
	{
		PARSE_D(strColumn)
		StrNCpy(item.wParentAreaName, 0, REDWINE_PARENTAREA_NAME_LEN, strColumn);
		sParentAreaMap.insert(StringIntPair(strColumn, i));
		if (!bufWrite->Push_back(&item, sizeof(RedWineParentAreaTag)))
		{
			return false;
		}
	}

	return true;
}

// 查找指定父产区的列表索引
int 
CRedWine::FindParentArea(const std::string &Key)
{
	if (sParentAreaMap.size() < 1)
	{
		return -1;
	}

	StringIntIter findInterator = sParentAreaMap.find(Key);
	if (findInterator != sParentAreaMap.end())
	{
		return findInterator->second;
	}
	return -1;
}

// 请求红酒子产区数据
bool 
CRedWine::RequestRedWineChildArea_S()
{
	RequestPack request;
	int iLength = 0;

	// 初始化请求数据
	request.Seq = htonl(GetSendSeqNum());
	request.TableId = htonl(12511);
	request.TableIdList = "12511";
	request.Count = htonl(1);
	request.SqlList.push_back("SELECT DISTINCT C2 FROM ST12511_main WHERE C2 != '' ORDER BY C2 ASC;");

	// 接收数据结构
	int iMaxLength = 0;
	int iWarning = 0;
	char *pStorteData = sLargeMemoryCache.GetRemainMem(iMaxLength, &iWarning);
	if (NULL == pStorteData || iWarning > 0)
	{
		return false;
	}
	ResponseDataBuf RespData(string("RedWineChildArea"), pStorteData, iMaxLength, sizeof(RedWineChildAreaTag), 1);
	CWriteBuff bufWrite((char*)RespData.DataBuf, RespData.GetDataLength(), 0);

	if (SendRequestData(&request))
	{
		if ((iLength = RecvResponseData()) > 0)
		{
			if (!ParseRedWineChildArea(&request, SResponseDataCache, iLength, &bufWrite))
			{
				sLargeMemoryCache.ClearRemainMem();
				return false;
			}
			sLargeMemoryCache.SetRemainMemStart(bufWrite.GetNewdataLen());
			RespData.DataLength = bufWrite.GetNewdataLen();
			sWineData.insert(StringResPair(RespData.KeyType, RespData));
			return true;
		}
	}

	return false;
}

// 解析红酒子产区数据
bool 
CRedWine::ParseRedWineChildArea(void *ReqData, char *data, const int length, CWriteBuff *bufWrite)
{
	if (NULL == ReqData || NULL == data	|| NULL == bufWrite)
	{
		return false;
	}
	
	CResDataParser DataParser(data, length, (RequestPack*)ReqData);
	if (!DataParser.CheckValid())
	{
		return false;
	}

	string strColumn;
	int iRow = DataParser.GetRecordRow();
	int iDatalength = sizeof(RedWineChildAreaTag) * iRow;
		
	// Head
	ZXCMDHEAD DataHead;
	DataHead.m_wCmdType = ZXCMD_REDWINE_PARENTAREA_LIST;
	DataHead.m_nAttr = 0;
	SetExHByteSerialToProperty(0, DataHead.m_nExData);
	DataHead.m_nLen = iDatalength;
	if (!bufWrite->Push_back(&DataHead, sizeof(ZXCMDHEAD)))
	{
		return false;
	}

	RedWineChildAreaTag item;
	sChildAreaMap.clear();
	for (int i = 0; i < iRow; i++)
	{
		PARSE_D(strColumn)
		StrNCpy(item.wChildAreaName, 0, REDWINE_CHILDAREA_NAME_LEN, strColumn);
		sChildAreaMap.insert(StringIntPair(strColumn, i));
		if (!bufWrite->Push_back(&item, sizeof(RedWineChildAreaTag)))
		{
			return false;
		}
	}

	return true;
}

// 查找指定子产区的列表索引
int 
CRedWine::FindChildArea(const std::string &Key)
{
	if (sChildAreaMap.size() < 1)
	{
		return -1;
	}

	StringIntIter findInterator = sChildAreaMap.find(Key);
	if (findInterator != sChildAreaMap.end())
	{
		return findInterator->second;
	}
	return -1;
}

// 获取是否已经成功获取了数据库数据
bool 
CRedWine::HaveGotTotalData()
{
	return 0 == m_iLastestErrorCode;
}

// 生成八大酒庄信息
bool 
CRedWine::GetEightWineriesData_S()
{
	struct TemMark
	{
		string EngName;
		string ChnName;
	};

	// 接收数据结构
	int iMaxLength = 0;
	int iWarning = 0;
	char *pStorteData = sLargeMemoryCache.GetRemainMem(iMaxLength, &iWarning);
	if (NULL == pStorteData || iWarning > 0)
	{
		return false;
	}
	ResponseDataBuf RespData(string("EightWineries"), pStorteData, iMaxLength, sizeof(RedwineManorListTag), 1);
	CWriteBuff bufWrite((char*)RespData.DataBuf, RespData.GetDataLength(), 0);

	int iRow = 8;
	int iDatalength = sizeof(RedwineManorListTag) * iRow;
		
	// Head
	ZXCMDHEAD DataHead;
	DataHead.m_wCmdType = ZXCMD_REDWINE_EIGHTWINERIES_LIST;
	DataHead.m_nAttr = 0;
	SetExHByteSerialToProperty(0, DataHead.m_nExData);
	DataHead.m_nLen = iDatalength;
	if (!bufWrite.Push_back(&DataHead, sizeof(ZXCMDHEAD)))
	{
		sLargeMemoryCache.ClearRemainMem();
		return false;
	}

	const TemMark DataMark[] = {{"Lafite", "拉菲"}, {"Latour", "拉图"}, {"Huat Brion", "奥比安"},
	{"Margaux", "玛歌"}, {"Mouton", "木桐"}, {"Cheval Blanc", "白马"}, {"Ausone", "欧颂"}, {"Petrus", "柏翠斯"}};

	RedwineManorListTag item;
	for (int i = 0; i < 8; i++)
	{
		StrNCpy(item.wWineryEngName, 0, REDWINE_MANOR_NAME_LEN, DataMark[i].EngName);
		StrNCpy(item.wWineryChnName, 0, REDWINE_MANOR_NAME_LEN, DataMark[i].ChnName);
		if (!bufWrite.Push_back(&item, sizeof(RedwineManorListTag)))
		{
			sLargeMemoryCache.ClearRemainMem();
			return false;
		}
	}	

	sLargeMemoryCache.SetRemainMemStart(bufWrite.GetNewdataLen());
	RespData.DataLength = bufWrite.GetNewdataLen();
	sWineData.insert(StringResPair(RespData.KeyType, RespData));
	return true;	
}

// 请求Liv报价最新列表
bool 
CRedWine::RequestLivLatestPriceList_S()
{
	RequestPack request;
	int iLength = 0;

	// 初始化请求数据
	request.Seq = htonl(GetSendSeqNum());
	request.TableId = htonl(12521);
	request.TableIdList = "12521";
	request.Count = htonl(1);
	request.SqlList.push_back("SELECT  N.CO, N.C4, N.C20, N.C21, DATE_FORMAT(N.C1, '%Y%m%d') AS aTime, N.C6, \
		N.C7, N.C8, N.C9, N.C10, N.C11, N.C12, N.C13, N.C18, N.C19 \
		FROM ST12521_main AS N \
		ORDER BY N.CO ASC, N.C4 ASC, N.C20 ASC, N.C21 ASC, aTime ASC;");

	// 接收数据结构
	int iMaxLength = 0;
	int iWarning = 0;
	char *pStorteData = sLargeMemoryCache.GetRemainMem(iMaxLength, &iWarning);
	if (NULL == pStorteData || iWarning > 0)
	{
		return false;
	}
	ResponseDataBuf RespData(string("LivLastestPriceList"), pStorteData, iMaxLength, sizeof(RedwineLatestLivListTag), 1);
	CWriteBuff bufWrite((char*)RespData.DataBuf, RespData.GetDataLength(), 0);

	if (SendRequestData(&request))
	{
		if ((iLength = RecvResponseData()) > 0)
		{
			if (!ParseLivLatestPriceList(&request, SResponseDataCache, iLength, &bufWrite))
			{
				sLargeMemoryCache.ClearRemainMem();
				return false;
			}
			sLargeMemoryCache.SetRemainMemStart(bufWrite.GetNewdataLen());
			RespData.DataLength = bufWrite.GetNewdataLen();
			sWineData.insert(StringResPair(RespData.KeyType, RespData));

			return true;
		}
	}

	return false;
}

// 解析Liv报价最新列表
bool 
CRedWine::ParseLivLatestPriceList(void *ReqData, char *data, const int length, CWriteBuff *bufWrite)
{
	if (NULL == ReqData || NULL == data	|| NULL == bufWrite)
	{
		return false;
	}
	
	CResDataParser DataParser(data, length, (RequestPack*)ReqData);
	if (!DataParser.CheckValid())
	{
		return false;
	}

	string strColumn;
	int iRow = DataParser.GetRecordRow();
	int iDatalength = sizeof(RedwineLatestLivListTag) * iRow;
		
	// Head
	ZXCMDHEAD DataHead;
	DataHead.m_wCmdType = ZXCMD_REDWINE_LIV30_LIST;
	DataHead.m_nAttr = 0;
	SetExHByteSerialToProperty(0, DataHead.m_nExData);
	DataHead.m_nLen = iDatalength;
	if (!bufWrite->Push_back(&DataHead, sizeof(ZXCMDHEAD)))
	{
		return false;
	}

	RedwineLatestLivListTag item;
	for (int i = 0; i < iRow; i++)
	{
		PARSE_D(strColumn)
		PARSE_D(strColumn)
		PARSE_D(strColumn)
		PARSE_D(strColumn)

		PARSE_D(strColumn)
		item.wLastestDate = StrToUl(strColumn);

		PARSE_D(strColumn)
		item.wLowestPriceDate = StrToUl(strColumn);

		PARSE_D(strColumn)
		item.wLowestPrice = (DWORD)(StrToDouble(strColumn) * ZoomScaleNum);

		PARSE_D(strColumn)
		item.wAvgPrice = (DWORD)(StrToDouble(strColumn) * ZoomScaleNum);

		PARSE_D(strColumn)
		item.tUnit = StrToUl(strColumn);

		PARSE_D(strColumn)
		item.nPrice = (DWORD)(StrToDouble(strColumn) * ZoomScaleNum);

		PARSE_D(strColumn)
		item.nAmount = StrToUl(strColumn);

		PARSE_D(strColumn)
		item.wAvgAuctionPrice = (DWORD)(StrToDouble(strColumn) * ZoomScaleNum);

		PARSE_D(strColumn)
		item.wAuctionAmount = StrToUl(strColumn);


		PARSE_D(strColumn)
		item.lobPrice = (DWORD)(StrToDouble(strColumn) * ZoomScaleNum);

		PARSE_D(strColumn)
		item.ltbPrice = (DWORD)(StrToDouble(strColumn) * ZoomScaleNum);

		if (!bufWrite->Push_back(&item, sizeof(RedwineLatestLivListTag)))
		{
			return false;
		}
	}

	return true;	
}

// 请求单只指数成分股在List中的索引
bool
CRedWine::RequestSingleIndexMemPosList_S(const int TotalCount, const int Serial, 
					const char* KeyWords, CWriteBuff *bufWrite)
{
	RequestPack request;
	int iLength = 0;
	string strSql("");

	// 初始化请求数据
	request.Seq = htonl(GetSendSeqNum());
	request.TableId = htonl(12521);
	request.TableIdList = "12521:12502";
	request.Count = htonl(2);
	strSql = "SET @mycnt = -1;";
	request.SqlList.push_back(strSql);
	strSql = "SELECT ROWNUM FROM \
		(SELECT M.C2, N.ROWNUM FROM \
		(SELECT CO, C2, C3, C4, C8, C9, C10 FROM ST12502_main WHERE C5 = 1 AND CO = '"
		+ string(KeyWords)
		+ "' ORDER BY C3 ASC, C4 ASC, C9 ASC, C10 ASC) AS M \
		 RIGHT JOIN (SELECT (@mycnt := @mycnt + 1) AS ROWNUM, CO, C3, C4, C5, C1 FROM ST12521_main ORDER BY CO ASC, C4 ASC, C20 ASC, C21 ASC, C1 ASC) AS N \
		 ON M.C3 = N.CO AND M.C4 = N.C4 AND M.C8 = N.C5) AS T WHERE T.C2 IS NOT NULL;";
	request.SqlList.push_back(strSql);

	if (SendRequestData(&request))
	{
		if ((iLength = RecvResponseData()) > 0)
		{
			if (ParseSingleIndexMemPosList_S(TotalCount, Serial, KeyWords, &request, SResponseDataCache, 
				iLength, bufWrite))
			{
				return true;
			}
		}
	}

	return false;
}

// 解析单只指数成分股在List中的索引
bool 
CRedWine::ParseSingleIndexMemPosList_S(const int TotalCount, const int Serial, const char* KeyWords, 
					void *ReqData, char *data, const int length, CWriteBuff *bufWrite)
{
	if (NULL == ReqData || NULL == data || NULL == bufWrite)
	{
		return false;
	}

	CResDataParser DataParser(data, length, (RequestPack*)ReqData);
	if (!DataParser.CheckValid())
	{
		return false;
	}

	string strColumn;
	int iRow = DataParser.GetRecordRow();
	int iTypeSize = sizeof(WORD);
	int iDatalength = iTypeSize * iRow;
		
	// Head
	ZXCMDHEAD DataHead;
	DataHead.m_wCmdType = ZXCMD_REDWINE_INDEX_ELEMENT;
	DataHead.m_nAttr = Serial == (TotalCount - 1) ? 0 : TotalCount;
	SetExHByteSerialToProperty(Serial, DataHead.m_nExData);
	DataHead.m_nLen = iDatalength;
	if (!bufWrite->Push_back(&DataHead, sizeof(ZXCMDHEAD)))
	{
		return false;
	}

	WORD item;
	for (int i = 0; i < iRow; i++)
	{
		PARSE_D(strColumn)
		item = StrToInt(strColumn);

		if (!bufWrite->Push_back(&item, iTypeSize))
		{
			return false;
		}
	}
	
	return true;
}

// 请求全部指数成分股索引数据
bool
CRedWine::RequestAllIndexMemPosList_S()
{
	// 访问列表数据
	const ResponseDataBuf *pBaseRespData = GetTypeWineData("WineIndexList");
	if (NULL == pBaseRespData)
	{
		return false;
	}

	// 数据量
	int iDataCount = pBaseRespData->GetIndexDataCount(0);
	if (iDataCount < 1)
	{
		return false;
	}

	// 接收数据结构
	int iMaxLength = 0;
	int iWarning = 0;
	char *pStorteData = sLargeMemoryCache.GetRemainMem(iMaxLength, &iWarning);
	if (NULL == pStorteData || iWarning > 0)
	{
		return false;
	}
	ResponseDataBuf RespData(string("MemListIndex"), pStorteData, iMaxLength, sizeof(WORD), iDataCount);
	CWriteBuff bufWrite((char*)RespData.DataBuf, RespData.GetDataLength(), 0);
	
	// 按照指数列表数据取全部指数数据
	for (int i = 0; i < iDataCount; i++)
	{
		RedwineIndexListTag *pData = (RedwineIndexListTag*)pBaseRespData->GetIndexData(0, i);
		if (NULL == pData || IsEmptyString(pData->wIndexCode))
		{
			sLargeMemoryCache.ClearRemainMem();
			return false;
		}
		if (RequestSingleIndexMemPosList_S(iDataCount, i, pData->wIndexCode, &bufWrite))
		{
			continue;
		}
		else 
		{
			sLargeMemoryCache.ClearRemainMem();
			return false;
		}
	}
	sLargeMemoryCache.SetRemainMemStart(bufWrite.GetNewdataLen(), &iWarning);
	RespData.DataLength = bufWrite.GetNewdataLen();
	sWineData.insert(StringResPair(RespData.KeyType, RespData));

	return true;
}

// 请求销售区域数据
bool 
CRedWine::RequestSaleArea_S()
{
	// 接收数据结构
	int iMaxLength = 0;
	int iWarning = 0;
	char *pStorteData = sLargeMemoryCache.GetRemainMem(iMaxLength, &iWarning);
	if (NULL == pStorteData || iWarning > 0)
	{
		return false;
	}
	ResponseDataBuf RespData(string("RedWineSaleArea"), pStorteData, iMaxLength, sizeof(RedWineSaleAreaTag), 1);
	CWriteBuff bufWrite((char*)RespData.DataBuf, RespData.GetDataLength(), 0);

	int iRow = 6;
	int iDatalength = sizeof(RedWineSaleAreaTag) * iRow;
		
	// Head
	ZXCMDHEAD DataHead;
	DataHead.m_wCmdType = ZXCMD_REDWINE_SALEAREA;
	DataHead.m_nAttr = 0;
	SetExHByteSerialToProperty(0, DataHead.m_nExData);
	DataHead.m_nLen = iDatalength;
	if (!bufWrite.Push_back(&DataHead, sizeof(ZXCMDHEAD)))
	{
		sLargeMemoryCache.ClearRemainMem();
		return false;
	}

	RedWineSaleAreaTag item;
	string strColumn[] = {"中国大陆", "中国香港", "北美", "欧洲", "其他","所有"};
	sSaleAreaMap.clear();
	for (int i = 0; i < iRow; i++)
	{
		StrNCpy(item.wSaleAreaName, 0, REDWINE_SALEAREA_LEN, strColumn[i]);
		sSaleAreaMap.insert(StringIntPair(strColumn[i], i));

		if (!bufWrite.Push_back(&item, sizeof(RedWineSaleAreaTag)))
		{
			sLargeMemoryCache.ClearRemainMem();
			return false;
		}
	}	

	sLargeMemoryCache.SetRemainMemStart(bufWrite.GetNewdataLen());
	RespData.DataLength = bufWrite.GetNewdataLen();
	sWineData.insert(StringResPair(RespData.KeyType, RespData));
	return true;
}

// 查找销售区域
int 
CRedWine::FindSaleArea(const std::string &Key)
{
	if (sSaleAreaMap.size() < 1)
	{
		return -1;
	}

	StringIntIter findInterator = sSaleAreaMap.find(Key);
	if (findInterator != sSaleAreaMap.end())
	{
		return findInterator->second;
	}
	return -1;
}

// 解析Liv-Ex价格日周期全部数据
bool
CRedWine::ParseLivexDayPrice(const int TotalCount, int &Serial, void *ReqData, char *data, const int length, 
							 CWriteBuff *bufWrite, size_t &ItemCount)
{
	if (NULL == ReqData || NULL == data || NULL == bufWrite)
	{
		return false;
	}

	CResDataParser DataParser(data, length, (RequestPack*)ReqData);
	if (!DataParser.CheckValid())
	{
		return false;
	}

	int iTypeSize = sizeof(RedwinePriceLivDayHistDataTag);
	int iRow = DataParser.GetRecordRow();
	ItemCount = iRow;
	
	RedwinePriceLivDayHistDataTag item;
	string strColumn;
	RedWineMark *pMark = NULL;
	static int iSerialCount = 0;
	bool bFindCurrentSeriaData = false;
	BYTE uMoneyUnit = 0;
	RedWineMark tempItem;
	for (int i = 0; i < iRow; i++)
	{
		PARSE_D(strColumn)
		tempItem.wNameCode = strColumn;

		PARSE_D(strColumn)
		tempItem.wYear = StrToInt(strColumn);

		PARSE_D(strColumn)
		tempItem.wCapacity = strColumn;

		PARSE_D(strColumn)
		tempItem.wNumber = StrToInt(strColumn);

		PARSE_D(strColumn)
		item.qTime = CRedWine::StrToUl(strColumn);
		
		PARSE_D(strColumn)
		item.wAvgPrice = (DWORD)(StrToDouble(strColumn) * ZoomScaleNum);

		PARSE_D(strColumn)
		item.wLowestPrice = (DWORD)(StrToDouble(strColumn) * ZoomScaleNum);

		PARSE_D(strColumn)
		item.wAvgAuctionPrice = (DWORD)(StrToDouble(strColumn) * ZoomScaleNum);

		PARSE_D(strColumn)
		uMoneyUnit = FindMoneyUnit(strColumn);

		PARSE_D(strColumn)
		item.wVolume = StrToUl(strColumn);

		if (0 == iSerialCount)
		{
			while(Serial < TotalCount)
			{
				pMark = FindLivMemMark(Serial);
				if (NULL == pMark)
				{
					return false;
				}
				iSerialCount = pMark->nDayPriceMemCount;

				// Head
				ZXCMDHEAD DataHead;
				DataHead.m_wCmdType = ZXCMD_REDWINE_LIV_DAY_DATA;
				DataHead.m_nAttr = Serial == (TotalCount - 1) ? 0 : TotalCount;
				SetHByteMoneyUnitToProperty(uMoneyUnit, DataHead.m_nExData);
				SetExHByteSerialToProperty(Serial++, DataHead.m_nExData);
				DataHead.m_nLen = iSerialCount * iTypeSize;
				if (!bufWrite->Push_back(&DataHead, sizeof(ZXCMDHEAD)))
				{
					return false;
				}

				if (0 == iSerialCount)	// 插入空包
				{
					continue;
				}
				else if (iSerialCount > 0)
				{
					if (*pMark == tempItem)
					{
						bFindCurrentSeriaData = true;						
					}
					else 
					{
						bFindCurrentSeriaData = false;
					}
					break;
				}
			}
		}
		else if (iSerialCount > 0 && NULL == pMark)	// 接前包
		{
			pMark = FindLivMemMark(Serial - 1);
			if (NULL == pMark)
			{
				return false;
			}
		}

		printf("Serial = %d, [main %s %6d] [tem %s %6d]\n", Serial, pMark->wNameCode.c_str(), pMark->wYear, tempItem.wNameCode.c_str(), tempItem.wYear);
		if (!bFindCurrentSeriaData)
		{
			if (*pMark == tempItem)
			{
				bFindCurrentSeriaData = true;
			}
			else 
			{
				bFindCurrentSeriaData = false;
				continue;
			}			
		}

		// item
		if (!bufWrite->Push_back(&item, iTypeSize))
		{
			return false;
		}
		iSerialCount--;
		
	}
	
	return true;
}

// 获取Liv-Ex价格日周期全部数据
bool
CRedWine::RequestLivexDayPrice_S()
{
	RequestPack request;
	int iLength = 0;
	string strSql("");	
	char cCondtion[128] = {0};

	// 基本请求语句
	string strBaseSql = "SELECT N.CO, N.C4, N.C14, N.C15, DATE_FORMAT(C1, '%Y%m%d') AS qTime, N.C8 AS avgPrice, \
		N.C7 AS wLPrice, N.C12 AS aPrice, 5 AS wMoneyUnit, N.C9 AS wVolume \
		FROM ST12503_main AS N \
		ORDER BY N.CO ASC, N.C4 ASC, N.C14 ASC, N.C15 ASC, qTime ASC ";

	// 数据量
	int iTotalSegCount = sRedWineLivMemStrKeyMap.size();
	if (iTotalSegCount < 1)
	{
		return false;
	}

	// 接收数据结构
	int iMaxLength = 0;
	int iWarning = 0;
	char *pStorteData = sLargeMemoryCache.GetRemainMem(iMaxLength, &iWarning);
	if (NULL == pStorteData || iWarning > 0)
	{
		return false;
	}
	ResponseDataBuf RespData(string("WineLivexDayPrice"), pStorteData, iMaxLength, sizeof(RedwinePriceLivDayHistDataTag), iTotalSegCount);
	CWriteBuff bufWrite((char*)RespData.DataBuf, RespData.GetDataLength(), 0);

	// 分次循环请求数据
	int iSerial = 0;
	size_t iCount = 0;
	for (size_t i = 0; i < MaxCycleTimes; i++)
	{
		// 初始化请求数据
		request.SqlList.clear();
		request.Seq = htonl(GetSendSeqNum());
		request.TableId = htonl(12503);
		request.TableIdList = "12503";
		request.Count = htonl(1);

		// 限定数目条件
		sprintf(cCondtion, " LIMIT %d, %d;", i * MaxRecordPerCycle, MaxRecordPerCycle);

		strSql = strBaseSql + string(cCondtion);
		request.SqlList.push_back(strSql);
		
		if (SendRequestData(&request))
		{
			if ((iLength = RecvResponseData()) > 0)
			{
				if (!ParseLivexDayPrice(iTotalSegCount, iSerial, &request, SResponseDataCache, iLength, 
					&bufWrite, iCount))
				{
					sLargeMemoryCache.ClearRemainMem();
					return false;
				}
				else if (iCount < MaxRecordPerCycle)
				{
					break;
				}
			}
			else 
			{
				sLargeMemoryCache.ClearRemainMem();
				return false;
			}
		}
	}

	sLargeMemoryCache.SetRemainMemStart(bufWrite.GetNewdataLen(), &iWarning);
	RespData.DataLength = bufWrite.GetNewdataLen();
	sWineData.insert(StringResPair(RespData.KeyType, RespData));

	return true;
}

// 拍卖价格日周期全部数据源 
bool 
CRedWine::RequestWineAuctionDayPrice_S()
{
	RequestPack request;
	int iLength = 0;
	string strSql("");
	char cCondtion[128] = {0};

	// 基本请求语句
	string strBaseSql = "SELECT CO, C7, C8, DATE_FORMAT(C3, '%Y%m%d') AS qTime, C13 AS avgPrice, C12 AS wLPrice, \
        C11 AS wHPrice, 3 AS wMoneyUnit, C10 AS wVolume, C5 AS SaleAddr, C1 AS wAuctionAddr \
		FROM ST12525_main \
		ORDER BY CO ASC, C7 ASC, C8 ASC, qTime ASC ";

	// 数据量
	int iTotalSegCount = sRedWineAuctionMemStrKeyMap.size();
	if (iTotalSegCount < 1)
	{
		return false;
	}

	// 接收数据结构
	int iMaxLength = 0;
	int iWarning = 0;
	char *pStorteData = sLargeMemoryCache.GetRemainMem(iMaxLength, &iWarning);
	if (NULL == pStorteData || iWarning > 0)
	{
		return false;
	}
	ResponseDataBuf RespData(string("WineAuctionDayPrice"), pStorteData, iMaxLength, sizeof(RedwinePriceAuctionDayHistTag), iTotalSegCount);
	CWriteBuff bufWrite((char*)RespData.DataBuf, RespData.GetDataLength(), 0);

	// 分次循环请求数据
	int iSerial = 0;
	size_t iCount = 0;
	for (size_t i = 0; i < MaxCycleTimes; i++)
	{
		// 初始化请求数据
		request.SqlList.clear();
		request.Seq = htonl(GetSendSeqNum());
		request.TableId = htonl(12525);
		request.TableIdList = "12525";
		request.Count = htonl(1);

		// 限定数目条件
		sprintf(cCondtion, " LIMIT %d, %d;", i * MaxRecordPerCycle, MaxRecordPerCycle);

		strSql = strBaseSql + string(cCondtion);
		request.SqlList.push_back(strSql);
		
		if (SendRequestData(&request))
		{
			if ((iLength = RecvResponseData()) > 0)
			{
				if (!ParseWineAuctionDayPrice(iTotalSegCount, iSerial, &request, SResponseDataCache, iLength, 
					&bufWrite, iCount))
				{
					sLargeMemoryCache.ClearRemainMem();
					return false;
				}
				else if (iCount < MaxRecordPerCycle)
				{
					break;
				}
			}
			else 
			{
				sLargeMemoryCache.ClearRemainMem();
				return false;
			}
		}
	}

	sLargeMemoryCache.SetRemainMemStart(bufWrite.GetNewdataLen(), &iWarning);
	RespData.DataLength = bufWrite.GetNewdataLen();
	sWineData.insert(StringResPair(RespData.KeyType, RespData));

	return true;	
}

// 解析拍卖日周期数据
bool 
CRedWine::ParseWineAuctionDayPrice(const int TotalCount, int &Serial, void *ReqData, char *data, const int length, 
			   CWriteBuff *bufWrite, size_t &ItemCount)
{
	if (NULL == ReqData || NULL == data || NULL == bufWrite)
	{
		return false;
	}

	CResDataParser DataParser(data, length, (RequestPack*)ReqData);
	if (!DataParser.CheckValid())
	{
		return false;
	}

	int iTypeSize = sizeof(RedwinePriceAuctionDayHistTag);
	int iRow = DataParser.GetRecordRow();
	ItemCount = iRow;
	
	RedwinePriceAuctionDayHistTag item;
	string strColumn;
	RedWineMark *pMark = NULL;
	static int iSerialCount = 0;
	bool bFindCurrentSeriaData = false;
	BYTE uMoneyUnit = 0;
	RedWineMark tempItem;
	for (int i = 0; i < iRow; i++)
	{
		PARSE_D(strColumn)
		tempItem.wNameCode = strColumn;

		PARSE_D(strColumn)
		tempItem.wYear = StrToInt(strColumn);

		PARSE_D(strColumn)
		tempItem.wCapacity = strColumn;

		tempItem.wNumber = 1;

		PARSE_D(strColumn)
		item.qTime = StrToUl(strColumn);
		
		PARSE_D(strColumn)
		item.wAvgPrice = (DWORD)(StrToDouble(strColumn) * ZoomScaleNum);

		PARSE_D(strColumn)
		item.wLowestPrice = (DWORD)(StrToDouble(strColumn) * ZoomScaleNum);

		PARSE_D(strColumn)
		item.wHighestPrice = (DWORD)(StrToDouble(strColumn) * ZoomScaleNum);

		PARSE_D(strColumn)
		uMoneyUnit = FindMoneyUnit(strColumn);

		PARSE_D(strColumn)
		item.wVolume = StrToUl(strColumn);

		PARSE_D(strColumn)
		item.wSaleAddr = FindSaleAddress(strColumn);	

		PARSE_D(strColumn)
		item.wAuctionAddr = FindAuctionHouse(strColumn);

		if (0 == iSerialCount)
		{
			while(Serial < TotalCount)
			{
				pMark = FindAuctionMemMark(Serial);
				if (NULL == pMark)
				{
					return false;
				}
				iSerialCount = pMark->nDayPriceMemCount;

				// Head
				ZXCMDHEAD DataHead;
				DataHead.m_wCmdType = ZXCMD_REDWINE_AUCTION_DAY_DATA;
				DataHead.m_nAttr = Serial == (TotalCount - 1) ? 0 : TotalCount;
				SetHByteMoneyUnitToProperty(uMoneyUnit, DataHead.m_nExData);
				SetExHByteSerialToProperty(Serial++, DataHead.m_nExData);
				DataHead.m_nLen = iSerialCount * iTypeSize;
				if (!bufWrite->Push_back(&DataHead, sizeof(ZXCMDHEAD)))
				{
					return false;
				}

				if (0 == iSerialCount)	// 插入空包
				{
					continue;
				}
				else if (iSerialCount > 0)
				{
					if (*pMark == tempItem)
					{
						bFindCurrentSeriaData = true;
					}
					else 
					{
						bFindCurrentSeriaData = false;
					}
					break;
				}
			}
		}
		else if (iSerialCount > 0 && NULL == pMark)	// 接前包
		{
			pMark = FindAuctionMemMark(Serial - 1);
			if (NULL == pMark)
			{
				return false;
			}
		}
		
		if (!bFindCurrentSeriaData)
		{
			if (*pMark == tempItem)
			{
				bFindCurrentSeriaData = true;
			}
			else 
			{
				bFindCurrentSeriaData = false;
				continue;
			}			
		}

		printf("Serial = %d, [main %s %6d] [tem %s %6d]\n", Serial, pMark->wNameCode.c_str(), pMark->wYear, tempItem.wNameCode.c_str(), tempItem.wYear);
		// item
		if (!bufWrite->Push_back(&item, iTypeSize))
		{
			return false;
		}
		iSerialCount--;
		
	}
	
	return true;
}

// 请求月统计数据
bool 
CRedWine::RequestMonthPrice(const MonthStatmentTag *MarkConditon)
{
	RequestPack request;
	int iLength = 0;
	string strSql("");	
	char cCondtion[128] = {0};
	char cTypeCondtion[16] = {0};

	sprintf(cTypeCondtion, " C4 = %d ", MarkConditon->stType);
	// 基本请求语句
	string strBaseSql = "SELECT CO, C3, C6, C7, C1 AS qTime, C8 AS avgPrice, C10 AS wLPrice, C9 AS wHPrice, \
	    3 AS wMoneyUnit, C11 AS wVolume, C5 AS wArea \
		FROM ST12527_main WHERE " +  string(cTypeCondtion) + 
		"ORDER BY CO ASC, C3 ASC, C6 ASC, C7 ASC, qTime ASC, C5 ASC ";

	// 数据源
	pRedWineMarkIntKeyMap DataMap;
	FindTypeMemMark MarkFun;
	if (1 == MarkConditon->stType)
	{
		DataMap = (pRedWineMarkIntKeyMap)&sRedWineAuctionMemIntKeyMap;
		MarkFun = FindAuctionMemMark;
	}
	else if (2 == MarkConditon->stType)
	{
		DataMap = (pRedWineMarkIntKeyMap)&sRedWineRetailMemIntKeyMap;
		MarkFun = FindRetailMemMark;
	}
	else if (3 == MarkConditon->stType)
	{
		DataMap = (pRedWineMarkIntKeyMap)&sRedWineLivMemIntKeyMap;
		MarkFun = FindLivMemMark;
	}
	else 
	{
		return false;
	}

	// 数据量
	int iTotalSegCount = DataMap->size();
	if (iTotalSegCount < 1)
	{
		return false;
	}

	// 接收数据结构
	int iMaxLength = 0;
	int iWarning = 0;
	char *pStorteData = sLargeMemoryCache.GetRemainMem(iMaxLength, &iWarning);
	if (NULL == pStorteData || iWarning > 0)
	{
		return false;
	}
	ResponseDataBuf RespData(MarkConditon->stKey, pStorteData, iMaxLength, sizeof(RedwinePriceHistDataTag), iTotalSegCount);
	CWriteBuff bufWrite((char*)RespData.DataBuf, RespData.GetDataLength(), 0);

	// 分次循环请求数据
	int iSerial = 0;
	size_t iCount = 0;
	for (size_t i = 0; i < MaxCycleTimes; i++)
	{
		// 初始化请求数据
		request.SqlList.clear();
		request.Seq = htonl(GetSendSeqNum());
		request.TableId = htonl(12527);
		request.TableIdList = "12527";
		request.Count = htonl(1);

		// 限定数目条件
		sprintf(cCondtion, " LIMIT %d, %d;", i * MaxRecordPerCycle, MaxRecordPerCycle);

		strSql = strBaseSql + string(cCondtion);
		request.SqlList.push_back(strSql);
		
		if (SendRequestData(&request))
		{
			if ((iLength = RecvResponseData()) > 0)
			{
				bool bParRes = false;
				if (3 == MarkConditon->stType)
				{
					bParRes = ParseLivMonthPrice(iTotalSegCount, iSerial, &request, SResponseDataCache, iLength, 
							MarkConditon->stHeadCmdType, MarkFun, &bufWrite, iCount);
				}
				else
				{
					bParRes = ParseMonthPrice(iTotalSegCount, iSerial, &request, SResponseDataCache, iLength, 
							MarkConditon->stHeadCmdType, MarkFun, &bufWrite, iCount);
				}
				
				if (!bParRes)
				{
					sLargeMemoryCache.ClearRemainMem();
					return false;
				}
				else if (iCount < MaxRecordPerCycle)
				{
					break;
				}
			}
			else 
			{
				sLargeMemoryCache.ClearRemainMem();
				return false;
			}
		}
	}

	sLargeMemoryCache.SetRemainMemStart(bufWrite.GetNewdataLen(), &iWarning);
	RespData.DataLength = bufWrite.GetNewdataLen();
	sWineData.insert(StringResPair(RespData.KeyType, RespData));

	return true;
}

// 解析月统计数据
bool 
CRedWine::ParseMonthPrice(const int TotalCount, int &Serial, void *ReqData, char *data, const int length, 
			   const int CmdType, FindTypeMemMark markFun, CWriteBuff *bufWrite, size_t &ItemCount)
{
	if (NULL == ReqData || NULL == data || NULL == bufWrite
		|| NULL == markFun)
	{
		return false;
	}

	CResDataParser DataParser(data, length, (RequestPack*)ReqData);
	if (!DataParser.CheckValid())
	{
		return false;
	}

	int iTypeSize = sizeof(RedwinePriceHistDataTag);
	int iRow = DataParser.GetRecordRow();
	ItemCount = iRow;
	
	RedwinePriceHistDataTag item;
	string strColumn;
	RedWineMark *pMark = NULL;
	static int iSerialCount = 0;
	bool bFindCurrentSeriaData = false;
	BYTE uMoneyUnit = 0;
	RedWineMark tempItem;
	for (int i = 0; i < iRow; i++)
	{
		PARSE_D(strColumn)
		tempItem.wNameCode = strColumn;

		PARSE_D(strColumn)
		tempItem.wYear = StrToInt(strColumn);

		PARSE_D(strColumn)
		tempItem.wCapacity = strColumn;

		PARSE_D(strColumn)
		tempItem.wNumber = StrToInt(strColumn);

		PARSE_D(strColumn)
		item.qTime = StrToUl(strColumn);
		
		PARSE_D(strColumn)
		item.wAvgPrice = (DWORD)(StrToDouble(strColumn) * ZoomScaleNum);

		PARSE_D(strColumn)
		item.wLowestPrice = (DWORD)(StrToDouble(strColumn) * ZoomScaleNum);

		PARSE_D(strColumn)
		item.wHighestPrice = (DWORD)(StrToDouble(strColumn) * ZoomScaleNum);

		PARSE_D(strColumn)
		uMoneyUnit = FindMoneyUnit(strColumn);

		PARSE_D(strColumn)
		item.wVolume = StrToUl(strColumn);

		PARSE_D(strColumn)//与sSaleAreaMap 值部分相对应
		item.cSaleArea = StrToInt(strColumn) - 1;

		if (0 == iSerialCount)
		{
			while(Serial < TotalCount)
			{
				pMark = (*markFun)(Serial);
				if (NULL == pMark)
				{
					return false;
				}
				iSerialCount = pMark->nMonthPriceMemCount;

				// Head
				ZXCMDHEAD DataHead;
				DataHead.m_wCmdType = CmdType;
				DataHead.m_nAttr = Serial == (TotalCount - 1) ? 0 : TotalCount;
				SetHByteMoneyUnitToProperty(uMoneyUnit, DataHead.m_nExData);
				SetExHByteSerialToProperty(Serial++, DataHead.m_nExData);
				DataHead.m_nLen = iSerialCount * iTypeSize;
				if (!bufWrite->Push_back(&DataHead, sizeof(ZXCMDHEAD)))
				{
					return false;
				}

				if (0 == iSerialCount)	// 插入空包
				{
					continue;
				}
				else if (iSerialCount > 0)
				{
					if (*pMark == tempItem)
					{
						bFindCurrentSeriaData = true;						
					}
					else 
					{
						bFindCurrentSeriaData = false;
					}
					break;
				}
			}
		}
		else if (iSerialCount > 0 && NULL == pMark)	// 接前包
		{
			pMark = (*markFun)(Serial - 1);
			if (NULL == pMark)
			{
				return false;
			}
		}
		
		if (!bFindCurrentSeriaData)
		{
			if (*pMark == tempItem)
			{
				bFindCurrentSeriaData = true;
			}
			else 
			{
				bFindCurrentSeriaData = false;
				continue;
			}			
		}
		printf("Serial = %d, [main %s %6d] [tem %s %6d]\n", Serial, pMark->wNameCode.c_str(), pMark->wYear, tempItem.wNameCode.c_str(), tempItem.wYear);
		// item
		if (!bufWrite->Push_back(&item, iTypeSize))
		{
			return false;
		}
		iSerialCount--;
		
	}
	
	return true;
}

// 解析Liv月统计数据
bool 
CRedWine::ParseLivMonthPrice(const int TotalCount, int &Serial, void *ReqData, char *data, const int length, 
			   const int CmdType, FindTypeMemMark markFun, CWriteBuff *bufWrite, size_t &ItemCount)
{
	if (NULL == ReqData || NULL == data || NULL == bufWrite
		|| NULL == markFun)
	{
		return false;
	}

	CResDataParser DataParser(data, length, (RequestPack*)ReqData);
	if (!DataParser.CheckValid())
	{
		return false;
	}

	int iTypeSize = sizeof(RedwinePriceLivMonthHistDataTag);
	int iRow = DataParser.GetRecordRow();
	ItemCount = iRow;
	
	RedwinePriceLivMonthHistDataTag item;
	string strColumn;
	RedWineMark *pMark = NULL;
	static int iSerialCount = 0;
	bool bFindCurrentSeriaData = false;
	BYTE uMoneyUnit = 0;
	RedWineMark tempItem;
	for (int i = 0; i < iRow; i++)
	{
		PARSE_D(strColumn)
		tempItem.wNameCode = strColumn;

		PARSE_D(strColumn)
		tempItem.wYear = StrToInt(strColumn);

		PARSE_D(strColumn)
		tempItem.wCapacity = strColumn;

		PARSE_D(strColumn)
		tempItem.wNumber = StrToInt(strColumn);

		PARSE_D(strColumn)
		item.qTime = StrToUl(strColumn);
		
		PARSE_D(strColumn)
		item.wAvgPrice = (DWORD)(StrToDouble(strColumn) * ZoomScaleNum);

		PARSE_D(strColumn)
		item.wLowestPrice = (DWORD)(StrToDouble(strColumn) * ZoomScaleNum);

		PARSE_D(strColumn)
		item.wHighestPrice = (DWORD)(StrToDouble(strColumn) * ZoomScaleNum);

		PARSE_D(strColumn)
		uMoneyUnit = FindMoneyUnit(strColumn);

		PARSE_D(strColumn)
		item.wVolume = StrToUl(strColumn);

		PARSE_D(strColumn)

		if (0 == iSerialCount)
		{
			while(Serial < TotalCount)
			{
				pMark = (*markFun)(Serial);
				if (NULL == pMark)
				{
					return false;
				}
				iSerialCount = pMark->nMonthPriceMemCount;

				// Head
				ZXCMDHEAD DataHead;
				DataHead.m_wCmdType = CmdType;
				DataHead.m_nAttr = Serial == (TotalCount - 1) ? 0 : TotalCount;
				SetHByteMoneyUnitToProperty(uMoneyUnit, DataHead.m_nExData);
				SetExHByteSerialToProperty(Serial++, DataHead.m_nExData);
				DataHead.m_nLen = iSerialCount * iTypeSize;
				if (!bufWrite->Push_back(&DataHead, sizeof(ZXCMDHEAD)))
				{
					return false;
				}

				if (0 == iSerialCount)	// 插入空包
				{
					continue;
				}
				else if (iSerialCount > 0)
				{
					if (*pMark == tempItem)
					{
						bFindCurrentSeriaData = true;						
					}
					else 
					{
						bFindCurrentSeriaData = false;
					}
					break;
				}
			}
		}
		else if (iSerialCount > 0 && NULL == pMark)	// 接前包
		{
			pMark = (*markFun)(Serial - 1);
			if (NULL == pMark)
			{
				return false;
			}
		}

		if (!bFindCurrentSeriaData)
		{
			if (*pMark == tempItem)
			{
				bFindCurrentSeriaData = true;
			}
			else 
			{
				bFindCurrentSeriaData = false;
				continue;
			}			
		}
		printf("Serial = %d, [main %s %6d] [tem %s %6d]\n", Serial, pMark->wNameCode.c_str(), pMark->wYear, tempItem.wNameCode.c_str(), tempItem.wYear);
		// item
		if (!bufWrite->Push_back(&item, iTypeSize))
		{
			return false;
		}
		iSerialCount--;
		
	}
	
	return true;
}

// 请求全部月统计数据
bool 
CRedWine::RequestMonthPrice_S()
{
	const MonthStatmentTag tMonthStatmentTag[] = {
		{1, ZXCMD_REDWINE_AUCTION_MONTH_DATA, "AuctionMonthPriceHisData"}, 
		{2, ZXCMD_REDWINE_RETAIL_MONTH_DATA, "RetailMonthPriceHisData"},
		{3, ZXCMD_REDWINE_LIV_MONTH_DATA, "LivMonthPriceHisData"}
	};

	for (int i = 0; i < 3; i++)
	{
		if(!RequestMonthPrice(&tMonthStatmentTag[i]))
		{
			return false;
		}
	}
	return true;
}

// 请求货币单位数据
bool 
CRedWine::ReqeustMoneyUnit_S()
{
	// 接收数据结构
	int iMaxLength = 0;
	int iWarning = 0;
	char *pStorteData = sLargeMemoryCache.GetRemainMem(iMaxLength, &iWarning);
	if (NULL == pStorteData || iWarning > 0)
	{
		return false;
	}
	ResponseDataBuf RespData(string("RedwineMoneyUnit"), pStorteData, iMaxLength, sizeof(RedwineMoneyUnitTag), 1);
	CWriteBuff bufWrite((char*)RespData.DataBuf, RespData.GetDataLength(), 0);

	int iRow = 12;
	int iTypeSize = sizeof(RedwineMoneyUnitTag);
	int iDatalength = iTypeSize * iRow;
		
	// Head
	ZXCMDHEAD DataHead;
	DataHead.m_wCmdType = ZXCMD_REDWINE_MONEY_UNIT_DATA;
	DataHead.m_nAttr = 0;
	SetExHByteSerialToProperty(0, DataHead.m_nExData);
	DataHead.m_nLen = iDatalength;
	if (!bufWrite.Push_back(&DataHead, sizeof(ZXCMDHEAD)))
	{
		sLargeMemoryCache.ClearRemainMem();
		return false;
	}

	const string MoneyUnit[] = {"港币", "美元", "人民币", "欧元", "英镑", "日元", "加元", "澳元", "克朗", "卢布", "瑞士法郎", "库纳"};

	RedwineMoneyUnitTag item;
	sRedWineMoneyUnit.clear();
	for (int i = 0; i < iRow; i++)
	{
		StrNCpy(item.wMoneyUnitName, 0, REDWINE_MONEYUNIT_LEN, MoneyUnit[i]);
		sRedWineMoneyUnit.insert(pair<int,int>(i + 1, i));

		if (!bufWrite.Push_back(&item, iTypeSize))
		{
			sLargeMemoryCache.ClearRemainMem();
			return false;
		}
	}	

	sLargeMemoryCache.SetRemainMemStart(bufWrite.GetNewdataLen());
	RespData.DataLength = bufWrite.GetNewdataLen();
	sWineData.insert(StringResPair(RespData.KeyType, RespData));
	return true;
}

// 查找货币单位
int 
CRedWine::FindMoneyUnit(const string &Key)
{
	int iKey = StrToInt(Key);
	if (sRedWineMoneyUnit.size() < 1)
	{
		return -1;
	}

	map<int,int>::const_iterator findInterator = sRedWineMoneyUnit.find(iKey);
	if (findInterator != sRedWineMoneyUnit.end())
	{
		return findInterator->second;
	}
	return -1;
}

// 请求红酒销售地点信息
bool 
CRedWine::RequestSaleAddress_S()
{
	RequestPack request;
	int iLength = 0;
	string strSql("");

	// 初始化请求数据
	request.Seq = htonl(GetSendSeqNum());
	request.TableId = htonl(12526);
	request.TableIdList = "12526";
	request.Count = htonl(1);
	strSql = "SELECT DISTINCT C2, C5 FROM ST12526_main WHERE C2 != '' AND C2 IS NOT NULL ORDER BY C2 ASC;";
	request.SqlList.push_back(strSql);

	// 接收数据结构
	int iMaxLength = 0;
	int iWarning = 0;
	char *pStorteData = sLargeMemoryCache.GetRemainMem(iMaxLength, &iWarning);
	if (NULL == pStorteData || iWarning > 0)
	{
		return false;
	}
	ResponseDataBuf RespData(string("RedWineSaleAddress"), pStorteData, iMaxLength, sizeof(RedwineSaleAddressTag), 1);
	CWriteBuff bufWrite((char*)RespData.DataBuf, RespData.GetDataLength(), 0);

	if (SendRequestData(&request))
	{
		if ((iLength = RecvResponseData()) > 0)
		{
			if (!ParseSaleAddress(&request, SResponseDataCache, iLength, &bufWrite))
			{
				sLargeMemoryCache.ClearRemainMem();
				return false;
			}
			sLargeMemoryCache.SetRemainMemStart(bufWrite.GetNewdataLen());
			RespData.DataLength = bufWrite.GetNewdataLen();
			sWineData.insert(StringResPair(RespData.KeyType, RespData));
			return true;
		}
	}

	return false;
}

// 解析红酒销售地点信息
bool 
CRedWine::ParseSaleAddress(void *ReqData, char *data, const int length, CWriteBuff *bufWrite)
{
	if (NULL == ReqData || NULL == data	|| NULL == bufWrite)
	{
		return false;
	}
	
	CResDataParser DataParser(data, length, (RequestPack*)ReqData);
	if (!DataParser.CheckValid())
	{
		return false;
	}

	string strColumn;
	int iRow = DataParser.GetRecordRow();
	int iTypeSize = sizeof(RedwineSaleAddressTag);
	int iDatalength = iTypeSize * iRow;
		
	// Head
	ZXCMDHEAD DataHead;
	DataHead.m_wCmdType = ZXCMD_REDWINE_SALE_ADDRESS_DATA;
	DataHead.m_nAttr = 0;
	SetExHByteSerialToProperty(0, DataHead.m_nExData);
	DataHead.m_nLen = iDatalength;
	if (!bufWrite->Push_back(&DataHead, sizeof(ZXCMDHEAD)))
	{
		return false;
	}

	RedwineSaleAddressTag item;
	sRedWineSaleAddress.clear();
	for (int i = 0; i < iRow; i++)
	{
		PARSE_D(strColumn)
		StrNCpy(item.wSaleAddrChnName, 0, REDWINE_SALE_ADDR_LEN, strColumn);
		sRedWineSaleAddress.insert(StringIntPair(strColumn, i));

		PARSE_D(strColumn)
		item.nBelongArea = FindSaleArea(strColumn);

		if (!bufWrite->Push_back(&item, iTypeSize))
		{
			return false;
		}
	}

	return true;
}

// 查找销售地点索引
int 
CRedWine::FindSaleAddress(const std::string &Key)
{
	if (sRedWineSaleAddress.size() < 1)
	{
		return -1;
	}

	StringIntIter findInterator = sRedWineSaleAddress.find(Key);
	if (findInterator != sRedWineSaleAddress.end())
	{
		return findInterator->second;
	}
	return -1;
}

// 获取指定的销售地点数据
int
CRedWine::GetSaleAddressBelongArea(const std::string &Key)
{
	int iIndex = FindSaleAddress(Key);
	if (iIndex < 0)
	{
		return -1;
	}

	// 访问列表数据
	const ResponseDataBuf *pBaseRespData = GetTypeWineData("RedWineSaleAddress");
	if (NULL == pBaseRespData)
	{
		return -1;
	}
	
	// 数据量
	int iDataCount = pBaseRespData->GetIndexDataCount(0);
	if (iDataCount < 1)
	{
		return -1;
	}

	RedwineSaleAddressTag *pData = (RedwineSaleAddressTag*)pBaseRespData->GetIndexData(0, iIndex);
	if (NULL == pData)
	{
		return -1;
	}

	return pData->nBelongArea;
}

// 请求红酒拍卖行数据
bool 
CRedWine::RequestAuctionHouse_S()
{
	RequestPack request;
	int iLength = 0;
	string strSql("");

	// 初始化请求数据
	request.Seq = htonl(GetSendSeqNum());
	request.TableId = htonl(12524);
	request.TableIdList = "12524";
	request.Count = htonl(1);
	strSql = "SELECT DISTINCT C1 FROM ST12524_main WHERE C1 != '' AND C1 IS NOT NULL ORDER BY C1 ASC;";
	request.SqlList.push_back(strSql);

	// 接收数据结构
	int iMaxLength = 0;
	int iWarning = 0;
	char *pStorteData = sLargeMemoryCache.GetRemainMem(iMaxLength, &iWarning);
	if (NULL == pStorteData || iWarning > 0)
	{
		return false;
	}
	ResponseDataBuf RespData(string("RedWineAuctionHouse"), pStorteData, iMaxLength, sizeof(RedwineAuctionHouseTag), 1);
	CWriteBuff bufWrite((char*)RespData.DataBuf, RespData.GetDataLength(), 0);

	if (SendRequestData(&request))
	{
		if ((iLength = RecvResponseData()) > 0)
		{
			if (!ParseAuctionHouse(&request, SResponseDataCache, iLength, &bufWrite))
			{
				sLargeMemoryCache.ClearRemainMem();
				return false;
			}
			sLargeMemoryCache.SetRemainMemStart(bufWrite.GetNewdataLen());
			RespData.DataLength = bufWrite.GetNewdataLen();
			sWineData.insert(StringResPair(RespData.KeyType, RespData));
			return true;
		}
	}

	return false;
}

// 解析红酒拍卖行数据
bool 
CRedWine::ParseAuctionHouse(void *ReqData, char *data, const int length, CWriteBuff *bufWrite)
{
	if (NULL == ReqData || NULL == data	|| NULL == bufWrite)
	{
		return false;
	}
	
	CResDataParser DataParser(data, length, (RequestPack*)ReqData);
	if (!DataParser.CheckValid())
	{
		return false;
	}

	string strColumn;
	int iRow = DataParser.GetRecordRow();
	int iTypeSize = sizeof(RedwineAuctionHouseTag);
	int iDatalength = iTypeSize * iRow;
		
	// Head
	ZXCMDHEAD DataHead;
	DataHead.m_wCmdType = ZXCMD_REDWINE_AUCTION_HOUSE_DATA;
	DataHead.m_nAttr = 0;
	SetExHByteSerialToProperty(0, DataHead.m_nExData);
	DataHead.m_nLen = iDatalength;
	if (!bufWrite->Push_back(&DataHead, sizeof(ZXCMDHEAD)))
	{
		return false;
	}

	RedwineAuctionHouseTag item;
	sRedWineAuctionAddress.clear();
	for (int i = 0; i < iRow; i++)
	{
		PARSE_D(strColumn)
		StrNCpy(item.wAuctionAddrName, 0, REDWINE_AUCTION_HOUSE_LEN, strColumn);
		sRedWineAuctionAddress.insert(StringIntPair(strColumn, i));

		if (!bufWrite->Push_back(&item, iTypeSize))
		{
			return false;
		}
	}

	return true;
}

// 查找拍卖行索引
int 
CRedWine::FindAuctionHouse(const std::string &Key)
{
	if (sRedWineAuctionAddress.size() < 1)
	{
		return -1;
	}

	StringIntIter findInterator = sRedWineAuctionAddress.find(Key);
	if (findInterator != sRedWineAuctionAddress.end())
	{
		return findInterator->second;
	}
	return -1;
}

// 请求红酒拍卖会日程数据
bool 
CRedWine::RequestAuctionSchedule_S()
{
	RequestPack request;
	int iLength = 0;
	string strSql("");

	// 初始化请求数据
	request.Seq = htonl(GetSendSeqNum());
	request.TableId = htonl(12524);
	request.TableIdList = "12524";
	request.Count = htonl(1);
	strSql = "SELECT DATE_FORMAT(C6, '%Y%m%d') AS qTime, C1, C2, C4, C5, C16, C15, C19 FROM ST12524_main ORDER BY C5 ASC, C1 ASC, C6 ASC LIMIT 0, 500;";
	request.SqlList.push_back(strSql);

	// 接收数据结构
	int iMaxLength = 0;
	int iWarning = 0;
	char *pStorteData = sLargeMemoryCache.GetRemainMem(iMaxLength, &iWarning);
	if (NULL == pStorteData || iWarning > 0)
	{
		return false;
	}
	ResponseDataBuf RespData(string("RedWineAuctionSchedule"), pStorteData, iMaxLength, sizeof(RedwineAuctionScheduleTag), 1);
	CWriteBuff bufWrite((char*)RespData.DataBuf, RespData.GetDataLength(), 0);

	if (SendRequestData(&request))
	{
		if ((iLength = RecvResponseData()) > 0)
		{
			if (!ParseAuctionSchedule(&request, SResponseDataCache, iLength, &bufWrite))
			{
				sLargeMemoryCache.ClearRemainMem();
				return false;
			}
			sLargeMemoryCache.SetRemainMemStart(bufWrite.GetNewdataLen());
			RespData.DataLength = bufWrite.GetNewdataLen();
			sWineData.insert(StringResPair(RespData.KeyType, RespData));
			return true;
		}
	}

	return false;
}

// 解析红酒拍卖会日程数据
bool 
CRedWine::ParseAuctionSchedule(void *ReqData, char *data, const int length, CWriteBuff *bufWrite)
{
	if (NULL == ReqData || NULL == data	|| NULL == bufWrite)
	{
		return false;
	}
	
	CResDataParser DataParser(data, length, (RequestPack*)ReqData);
	if (!DataParser.CheckValid())
	{
		return false;
	}

	string strColumn;
	int iRow = DataParser.GetRecordRow();
	int iTypeSize = sizeof(RedwineAuctionScheduleTag);
	int iDatalength = iTypeSize * iRow;
		
	// Head
	ZXCMDHEAD DataHead;
	DataHead.m_wCmdType = ZXCMD_REDWINE_AUCTION_SCHEDULE_DATA;
	DataHead.m_nAttr = 0;
	SetExHByteSerialToProperty(0, DataHead.m_nExData);
	DataHead.m_nLen = iDatalength;
	if (!bufWrite->Push_back(&DataHead, sizeof(ZXCMDHEAD)))
	{
		return false;
	}

	RedwineAuctionScheduleTag item;
	for (int i = 0; i < iRow; i++)
	{
		PARSE_D(strColumn)
		item.qTime = StrToUl(strColumn);

		PARSE_D(strColumn)
		item.wAuctionHouse = FindAuctionHouse(strColumn);

		PARSE_D(strColumn)
		StrNCpy(item.cAuctionName, 0, REDWINE_AUCTION_NAME_LEN, strColumn);

		PARSE_D(strColumn)
		StrNCpy(item.cAuctionSpecialName, 0, REDWINE_AUCTION_NAME_LEN, strColumn);

		PARSE_D(strColumn)
		item.wAddress = FindSaleAddress(strColumn);

		PARSE_D(strColumn)
		item.wAmount = (DWORD)(StrToDouble(strColumn) * ZoomScaleNum);

		PARSE_D(strColumn)
		item.wMoneyUnit = FindMoneyUnit(strColumn);

		PARSE_D(strColumn)
		item.wAuctionType = StrToInt(strColumn);
		
		if (!bufWrite->Push_back(&item, iTypeSize))
		{
			return false;
		}
	}

	return true;
}

// 请求红酒名称列表数据
bool 
CRedWine::RequestRedwineNameList_S()
{
	RequestPack request;
	int iLength = 0;
	string strSql("");

	// 初始化请求数据
	request.Seq = htonl(GetSendSeqNum());
	request.TableId = htonl(12513);
	request.TableIdList = "12513";
	request.Count = htonl(1);
	strSql = "SELECT CO, C2, C34 FROM ST12513_main WHERE C2 != '' AND C2 IS NOT NULL GROUP BY CO ORDER BY C2 ASC;";
	request.SqlList.push_back(strSql);

	// 接收数据结构
	int iMaxLength = 0;
	int iWarning = 0;
	char *pStorteData = sLargeMemoryCache.GetRemainMem(iMaxLength, &iWarning);
	if (NULL == pStorteData || iWarning > 0)
	{
		return false;
	}
	ResponseDataBuf RespData(string("RedWineNameList"), pStorteData, iMaxLength, sizeof(RedwineNameTag), 1);
	CWriteBuff bufWrite((char*)RespData.DataBuf, RespData.GetDataLength(), 0);

	if (SendRequestData(&request))
	{
		if ((iLength = RecvResponseData()) > 0)
		{
			if (!ParseRedwineNameList(&request, SResponseDataCache, iLength, &bufWrite))
			{
				sLargeMemoryCache.ClearRemainMem();
				return false;
			}
			sLargeMemoryCache.SetRemainMemStart(bufWrite.GetNewdataLen());
			RespData.DataLength = bufWrite.GetNewdataLen();
			sWineData.insert(StringResPair(RespData.KeyType, RespData));
			return true;
		}
	}

	return false;
}

// 解析红酒名称列表数据
bool 
CRedWine::ParseRedwineNameList(void *ReqData, char *data, const int length, CWriteBuff *bufWrite)
{
	if (NULL == ReqData || NULL == data	|| NULL == bufWrite)
	{
		return false;
	}
	
	CResDataParser DataParser(data, length, (RequestPack*)ReqData);
	if (!DataParser.CheckValid())
	{
		return false;
	}

	string strColumn;
	int iRow = DataParser.GetRecordRow();
	int iTypeSize = sizeof(RedwineNameTag);
	int iDatalength = iTypeSize * iRow;
		
	// Head
	ZXCMDHEAD DataHead;
	DataHead.m_wCmdType = ZXCMD_REDWINE_NAME_LIST_DATA;
	DataHead.m_nAttr = 0;
	SetExHByteSerialToProperty(0, DataHead.m_nExData);
	DataHead.m_nLen = iDatalength;
	if (!bufWrite->Push_back(&DataHead, sizeof(ZXCMDHEAD)))
	{
		return false;
	}

	RedwineNameTag item;
	sRedWineNameUseStringKey.clear();
	sRedWineNameUseIntKey.clear();
	RedWineNameS localItem;
	for (int i = 0; i < iRow; i++)
	{
		PARSE_D(strColumn)
		localItem.wNameCode = strColumn;

		PARSE_D(strColumn)
		StrNCpy(item.wWineEngName, 0, REDWINE_NAME_LEN, strColumn);
		localItem.wWineEngName = strColumn;

		PARSE_D(strColumn)
		StrNCpy(item.wWineChnName, 0, REDWINE_NAME_LEN, strColumn);
		localItem.wWineChnName = strColumn;

		sRedWineNameUseStringKey.insert(StringIntPair(localItem.wNameCode, i));
		sRedWineNameUseIntKey.insert(pair<int, RedWineNameS>(i, localItem));

		if (!bufWrite->Push_back(&item, iTypeSize))
		{
			return false;
		}
	}

	return true;
}

// 查找红酒名称
int 
CRedWine::FindRedwineName(const std::string &Key)
{
	if (sRedWineNameUseStringKey.size() < 1)
	{
		return -1;
	}

	StringIntIter findInterator = sRedWineNameUseStringKey.find(Key);
	if (findInterator != sRedWineNameUseStringKey.end())
	{
		return findInterator->second;
	}
	return -1;
}

// 获取指定索引位置的名称
const RedWineNameS* 
CRedWine::GetIndexRedWineName(const int index)
{
	if (sRedWineNameUseIntKey.size() < 1)
	{
		return NULL;
	}

	map<int, RedWineNameS>::const_iterator findInterator = sRedWineNameUseIntKey.find(index);
	if (findInterator != sRedWineNameUseIntKey.end())
	{
		return &(findInterator->second);
	}
	return NULL;
}

// 请求红酒容量列表数据
bool
CRedWine::RequestRedwineCapacityList_S()
{
	RequestPack request;
	int iLength = 0;
	string strSql("");

	// 初始化请求数据
	request.Seq = htonl(GetSendSeqNum());
	request.TableId = htonl(12527);
	request.TableIdList = "12527";
	request.Count = htonl(1);
	strSql = "SELECT DISTINCT C6 FROM ST12527_main WHERE C6 != '' AND C6 IS NOT NULL ORDER BY C6 ASC;";
	request.SqlList.push_back(strSql);

	// 接收数据结构
	int iMaxLength = 0;
	int iWarning = 0;
	char *pStorteData = sLargeMemoryCache.GetRemainMem(iMaxLength, &iWarning);
	if (NULL == pStorteData || iWarning > 0)
	{
		return false;
	}
	ResponseDataBuf RespData(string("RedWineCapacityList"), pStorteData, iMaxLength, sizeof(RedwineCapacityTag), 1);
	CWriteBuff bufWrite((char*)RespData.DataBuf, RespData.GetDataLength(), 0);

	if (SendRequestData(&request))
	{
		if ((iLength = RecvResponseData()) > 0)
		{
			if (!ParseRedwineCapacityList(&request, SResponseDataCache, iLength, &bufWrite))
			{
				sLargeMemoryCache.ClearRemainMem();
				return false;
			}
			sLargeMemoryCache.SetRemainMemStart(bufWrite.GetNewdataLen());
			RespData.DataLength = bufWrite.GetNewdataLen();
			sWineData.insert(StringResPair(RespData.KeyType, RespData));
			return true;
		}
	}

	return false;
}

// 解析红酒容量列表数据
bool 
CRedWine::ParseRedwineCapacityList(void *ReqData, char *data, const int length, CWriteBuff *bufWrite)
{
	if (NULL == ReqData || NULL == data	|| NULL == bufWrite)
	{
		return false;
	}
	
	CResDataParser DataParser(data, length, (RequestPack*)ReqData);
	if (!DataParser.CheckValid())
	{
		return false;
	}

	string strColumn;
	int iRow = DataParser.GetRecordRow();
	int iTypeSize = sizeof(RedwineCapacityTag);
	int iDatalength = iTypeSize * iRow;
		
	// Head
	ZXCMDHEAD DataHead;
	DataHead.m_wCmdType = ZXCMD_REDWINE_CAPACITY_LIST_DATA;
	DataHead.m_nAttr = 0;
	SetExHByteSerialToProperty(0, DataHead.m_nExData);
	DataHead.m_nLen = iDatalength;
	if (!bufWrite->Push_back(&DataHead, sizeof(ZXCMDHEAD)))
	{
		return false;
	}

	RedwineCapacityTag item;	
	RedwineCapacityS localItem;
	sRedwineCapacityUseStringKey.clear();
	sRedwineCapacityUseIntKey.clear();
	for (int i = 0; i < iRow; i++)
	{
		PARSE_D(strColumn)
		StrNCpy(item.wCapacity, 0, REDWINE_CAPACITY_LEN, strColumn);
		localItem.wCapacity = strColumn;

		sRedwineCapacityUseStringKey.insert(StringIntPair(localItem.wCapacity, i));
		sRedwineCapacityUseIntKey.insert(pair<int, RedwineCapacityS>(i, localItem));

		if (!bufWrite->Push_back(&item, iTypeSize))
		{
			return false;
		}
	}

	return true;
}

// 查找红酒容量
int 
CRedWine::FindRedwineCapacity(const std::string &Key)
{
	if (sRedwineCapacityUseStringKey.size() < 1)
	{
		return -1;
	}

	StringIntIter findInterator = sRedwineCapacityUseStringKey.find(Key);
	if (findInterator != sRedwineCapacityUseStringKey.end())
	{
		return findInterator->second;
	}
	return -1;
}

// 获取指定索引位置的容量
const RedwineCapacityS* 
CRedWine::GetIndexRedWineCapacity(const int index)
{
	if (sRedwineCapacityUseIntKey.size() < 1)
	{
		return NULL;
	}

	map<int, RedwineCapacityS>::const_iterator findInterator = sRedwineCapacityUseIntKey.find(index);
	if (findInterator != sRedwineCapacityUseIntKey.end())
	{
		return &(findInterator->second);
	}
	return NULL;
}

// 请求帕克分数
bool 
CRedWine::RequestRedwineParkRating()
{
	RequestPack request;
	int iLength = 0;
	string strSql("");

	// 初始化请求数据
	request.Seq = htonl(GetSendSeqNum());
	request.TableId = htonl(12509);
	request.TableIdList = "12509";
	request.Count = htonl(1);
	strSql = "SELECT DATE_FORMAT(M.C1,'%Y%m%d') AS qTime, N.CO, N.C3, N.wLRating, N.wHRating FROM \
		(SELECT CO, C2, C3, C1 FROM ST12509_main WHERE C4 = '帕克' GROUP BY C2, C3, C1) AS M, \
		(SELECT CO, C2, C3, C1, C5 AS wLRating, C11 AS wHRating FROM ST12509_main WHERE C4 = '帕克') AS N \
		WHERE M.CO = N.CO AND M.C1 = N.C1 AND M.C2 = N.C2 AND M.C3 = N.C3 \
		ORDER BY N.CO ASC, N.C3 ASC;";
	request.SqlList.push_back(strSql);

	if (SendRequestData(&request))
	{
		if ((iLength = RecvResponseData()) > 0)
		{
			if (!ParseRedwineParkRating(&request, SResponseDataCache, iLength))
			{
				return false;
			}
			
			return true;
		}
	}

	return false;
}

// 解析帕克分数
bool 
CRedWine::ParseRedwineParkRating(void *ReqData, char *data, const int length)
{
	if (NULL == ReqData || NULL == data)
	{
		return false;
	}
	
	CResDataParser DataParser(data, length, (RequestPack*)ReqData);
	if (!DataParser.CheckValid())
	{
		return false;
	}

	string strColumn;
	string strKey("");
	int iRow = DataParser.GetRecordRow();
			
	RedwineParkRatinS item;	
	sRedwineParkRatingMap.clear();
	for (int i = 0; i < iRow; i++)
	{
		PARSE_D(strColumn)
		item.nTime = StrToUl(strColumn);

		PARSE_D(strColumn)
		strKey = strColumn;

		PARSE_D(strColumn)
		strKey += strColumn;

		PARSE_D(strColumn)
		item.nLowRating = StrToInt(strColumn);

		PARSE_D(strColumn)
		item.nHighRating = StrToInt(strColumn);

		sRedwineParkRatingMap.insert(pair<string, RedwineParkRatinS>(strKey, item));
	}

	return true;
}

// 查找帕克分数
const RedwineParkRatinS* 
CRedWine::FindRedwineParkRating(const std::string &Key)
{
	if (sRedwineParkRatingMap.size() < 1)
	{
		return NULL;
	}

	map<string, RedwineParkRatinS>::const_iterator findInterator = sRedwineParkRatingMap.find(Key);
	if (findInterator != sRedwineParkRatingMap.end())
	{
		return &(findInterator->second);
	}
	return NULL;
}

// 请求红酒基本信息
bool 
CRedWine::RequestRedwineBaseInfor_S()
{
	RequestPack request;
	int iLength = 0;
	string strSql("");

	// 初始化请求数据
	request.Seq = htonl(GetSendSeqNum());
	request.TableId = htonl(12513);
	request.TableIdList = "12513:12511";
	request.Count = htonl(1);
	strSql = "SELECT V.CO, V.C3, U.C4 AS ParentArea, V.C4, \
		CASE V.C38 WHEN '中国白酒' THEN '1' ELSE '2' END AS wType,\
		CASE WHEN LOWER(V.C8) LIKE '%lafite%' THEN 0 \
		WHEN LOWER(V.C8) LIKE '%latour%' THEN 1  \
		WHEN LOWER(V.C8) LIKE '%huat brion%' THEN 2 \
		WHEN LOWER(V.C8) LIKE '%margaux%' THEN 3 \
		WHEN LOWER(V.C8) LIKE '%mouton%' THEN 4 \
		WHEN LOWER(V.C8) LIKE '%cheval blanc%' THEN 5 \
		WHEN LOWER(V.C8) LIKE '%ausone%' THEN 6 \
		WHEN LOWER(V.C8) LIKE '%petrus%' THEN 7 \
		ELSE -1 END AS EightWineriesMark \
    FROM (SELECT C2, C4 FROM ST12511_main) AS U RIGHT JOIN ST12513_main AS V \
    ON V.C4 = U.C2 \
    ORDER BY V.C2 ASC;";
	request.SqlList.push_back(strSql);

	// 接收数据结构
	int iMaxLength = 0;
	int iWarning = 0;
	char *pStorteData = sLargeMemoryCache.GetRemainMem(iMaxLength, &iWarning);
	if (NULL == pStorteData || iWarning > 0)
	{
		return false;
	}
	ResponseDataBuf RespData(string("RedWineBaseInforList"), pStorteData, iMaxLength, sizeof(RedwineBaseInforTag), 1);
	CWriteBuff bufWrite((char*)RespData.DataBuf, RespData.GetDataLength(), 0);

	if (SendRequestData(&request))
	{
		if ((iLength = RecvResponseData()) > 0)
		{
			if (!ParseRedwineBaseInfor(&request, SResponseDataCache, iLength, &bufWrite))
			{
				sLargeMemoryCache.ClearRemainMem();
				return false;
			}
			sLargeMemoryCache.SetRemainMemStart(bufWrite.GetNewdataLen());
			RespData.DataLength = bufWrite.GetNewdataLen();
			sWineData.insert(StringResPair(RespData.KeyType, RespData));
			return true;
		}
	}

	return false;
}

// 解析红酒基本信息
bool 
CRedWine::ParseRedwineBaseInfor(void *ReqData, char *data, const int length, CWriteBuff *bufWrite)
{
	if (NULL == ReqData || NULL == data	|| NULL == bufWrite)
	{
		return false;
	}
	
	CResDataParser DataParser(data, length, (RequestPack*)ReqData);
	if (!DataParser.CheckValid())
	{
		return false;
	}

	string strColumn;
	string strBaseInforKey("");
	int iRow = DataParser.GetRecordRow();
	int iTypeSize = sizeof(RedwineBaseInforTag);
	int iDatalength = iTypeSize * iRow;
		
	// Head
	ZXCMDHEAD DataHead;
	DataHead.m_wCmdType = ZXCMD_REDWINE_BASE_INFOR_DATA;
	DataHead.m_nAttr = 0;
	SetExHByteSerialToProperty(0, DataHead.m_nExData);
	DataHead.m_nLen = iDatalength;
	if (!bufWrite->Push_back(&DataHead, sizeof(ZXCMDHEAD)))
	{
		return false;
	}

	RedwineBaseInforTag item;
	sRedwineBaseInforUseStringKey.clear();
	sRedwineBaseInforUseIntKey.clear();
	for (int i = 0; i < iRow; i++)
	{
		PARSE_D(strColumn)
		item.nRedwineName = FindRedwineName(strColumn);
		strBaseInforKey = strColumn;		

		PARSE_D(strColumn)
		item.wArea = FindAreaClassified(strColumn);

		PARSE_D(strColumn)
		item.wParentArea = FindParentArea(strColumn);

		PARSE_D(strColumn)
		item.wChildArea = FindChildArea(strColumn);

		PARSE_D(strColumn)
		item.wRedwineType = StrToInt(strColumn);

		PARSE_D(strColumn)
		item.wEightWineriesIdx = StrToInt(strColumn);

		sRedwineBaseInforUseStringKey.insert(StringIntPair(strBaseInforKey, i));
		sRedwineBaseInforUseIntKey.insert(pair<int, RedwineBaseInforTag>(i, item));

		if (!bufWrite->Push_back(&item, iTypeSize))
		{
			return false;
		}
	}

	return true;
}

// 查找红酒基本信息
int 
CRedWine::FindRedwineBaseInfor(const std::string &Key)
{
	if (sRedwineBaseInforUseStringKey.size() < 1)
	{
		return -1;
	}

	StringIntIter findInterator = sRedwineBaseInforUseStringKey.find(Key);
	if (findInterator != sRedwineBaseInforUseStringKey.end())
	{
		return findInterator->second;
	}
	return -1;
}

// 获取指定索引的红酒基本信息
const RedwineBaseInforTag* 
CRedWine::GetIndexBaseInfor(const int index)
{
	if (sRedwineBaseInforUseIntKey.size() < 1)
	{
		return NULL;
	}

	map<int, RedwineBaseInforTag>::const_iterator findInterator = sRedwineBaseInforUseIntKey.find(index);
	if (findInterator != sRedwineBaseInforUseIntKey.end())
	{
		return &(findInterator->second);
	}
	return NULL;
}

// 请求Liv市场全部成分酒数据
bool 
CRedWine::RequestLivMemberList_S()
{
	RequestPack request;
	int iLength = 0;
	string strSql("");

	// 初始化请求数据
	request.Seq = htonl(GetSendSeqNum());
	request.TableId = htonl(12521);
	request.TableIdList = "12521";
	request.Count = htonl(1);
	strSql = "SELECT DISTINCT CO, C4, C20, C21, '欧洲' AS wArea FROM ST12521_main \
		WHERE CO != '' AND CO IS NOT NULL \
		ORDER BY CO ASC, C4 ASC, C20 ASC, C21 ASC;";
	request.SqlList.push_back(strSql);

	// 接收数据结构
	int iMaxLength = 0;
	int iWarning = 0;
	char *pStorteData = sLargeMemoryCache.GetRemainMem(iMaxLength, &iWarning);
	if (NULL == pStorteData || iWarning > 0)
	{
		return false;
	}
	ResponseDataBuf RespData(string("RedWineLivMemList"), pStorteData, iMaxLength, sizeof(RedwineLivMemEnumTag), 1);
	CWriteBuff bufWrite((char*)RespData.DataBuf, RespData.GetDataLength(), 0);

	if (SendRequestData(&request))
	{
		if ((iLength = RecvResponseData()) > 0)
		{
			if (!ParseLivMemberList(&request, SResponseDataCache, iLength, &bufWrite))
			{
				sLargeMemoryCache.ClearRemainMem();
				return false;
			}
			sLargeMemoryCache.SetRemainMemStart(bufWrite.GetNewdataLen());
			RespData.DataLength = bufWrite.GetNewdataLen();
			sWineData.insert(StringResPair(RespData.KeyType, RespData));
			return true;
		}
	}

	return false;
}

// 解析Liv市场全部成分酒数据
bool 
CRedWine::ParseLivMemberList(void *ReqData, char *data, const int length, CWriteBuff *bufWrite)
{
	if (NULL == ReqData || NULL == data	|| NULL == bufWrite)
	{
		return false;
	}
	
	CResDataParser DataParser(data, length, (RequestPack*)ReqData);
	if (!DataParser.CheckValid())
	{
		return false;
	}

	string strColumn;
	string strKey("");
	string strMarkKey("");
	string strParkKey("");
	int iRow = DataParser.GetRecordRow();
	int iTypeSize = sizeof(RedwineLivMemEnumTag);

	// 跳过Head(开始位置为零，不加偏移量)
	bufWrite->SeekPos(sizeof(ZXCMDHEAD));
			
	// Head
	ZXCMDHEAD DataHead;
	DataHead.m_wCmdType = ZXCMD_REDWINE_LIV_MEMBER_DATA;
	DataHead.m_nAttr = 0;
	SetExHByteSerialToProperty(0, DataHead.m_nExData);
	
	int iBaseIndex = -1;
	int iMatchIndex = 0;
	RedwineLivMemEnumTag item;
	RedWineMark localItem;
	sRedWineLivMemStrKeyMap.clear();
	sRedWineLivMemIntKeyMap.clear();
	for (int i = 0; i < iRow; i++)
	{
		PARSE_D(strColumn)
		localItem.wNameCode = strColumn;
		strKey = strColumn;
		strMarkKey = strColumn;
		strParkKey = strColumn;		
		
		PARSE_D(strColumn)
		item.wYear = StrToInt(strColumn);
		localItem.wYear = item.wYear;
		strMarkKey += strColumn;
		strParkKey += strColumn;

		PARSE_D(strColumn)
		item.nRedwineCapacity = FindRedwineCapacity(strColumn);
		localItem.wCapacity = strColumn;
		strMarkKey += strColumn;		

		PARSE_D(strColumn)
		item.wNumber = StrToInt(strColumn);
		localItem.wNumber = item.wNumber;
		strMarkKey += strColumn;

		PARSE_D(strColumn)
		item.cSaleArea = FindSaleArea(strColumn);

		iBaseIndex = FindRedwineBaseInfor(strKey);
		// 过滤掉基准信息不存在的数据
		if (-1 == iBaseIndex)
		{
			continue;
		}
		item.nRedwineBaseInfor = iBaseIndex;
		
		// 帕克分数
		const RedwineParkRatinS *pParkRating = FindRedwineParkRating(strParkKey);
		item.nParkLRating = 0;
		item.nParkHRating = 0;
		if (NULL != pParkRating)
		{
			item.nParkLRating = pParkRating->nLowRating;
			item.nParkHRating = pParkRating->nHighRating;
		}

		localItem.nDayPriceMemCount = 0;
		localItem.nMonthPriceMemCount = 0;
		sRedWineLivMemStrKeyMap.insert(StringIntPair(strMarkKey, iMatchIndex));
		sRedWineLivMemIntKeyMap.insert(IntMarkPair(iMatchIndex++, localItem));

		if (!bufWrite->Push_back(&item, iTypeSize))
		{
			return false;
		}
	}

	// 写入Head
	bufWrite->SeekToBegin();
	DataHead.m_nLen = sRedWineLivMemStrKeyMap.size() * iTypeSize;
	if (!bufWrite->Push_set(&DataHead, sizeof(ZXCMDHEAD)))
	{
		return false;
	}
	bufWrite->SeekToEnd();

	return true;
}

// 查找指定酒标识的索引
int 
CRedWine::FindLivMemMarkIndex(const std::string &Key)
{
	if (sRedWineLivMemStrKeyMap.size() < 1)
	{
		return -1;
	}

	StringIntIter findInterator = sRedWineLivMemStrKeyMap.find(Key);
	if (findInterator != sRedWineLivMemStrKeyMap.end())
	{
		return findInterator->second;
	}
	return -1;
}

// 查找指定酒的标识ID
RedWineMark* 
CRedWine::FindLivMemMark(const int index)
{
	if (sRedWineLivMemIntKeyMap.size() < 1)
	{
		return NULL;
	}

	IntMarkIter findInterator = sRedWineLivMemIntKeyMap.find(index);
	if (findInterator != sRedWineLivMemIntKeyMap.end())
	{
		return &(findInterator->second);
	}
	return NULL;
}

// 请求Liv市场成员的日周期历史数据数目
bool 
CRedWine::RequestLivMemberDayPriceElement()
{
	RequestPack request;
	int iLength = 0;
	string strSql("");

	// 初始化请求数据
	request.Seq = htonl(GetSendSeqNum());
	request.TableId = htonl(12503);
	request.TableIdList = "12503";
	request.Count = htonl(1);
	strSql = "SELECT CO, C4, C14, C15, COUNT(*) FROM ST12503_main \
		WHERE C3 != '' AND C3 IS NOT NULL \
		GROUP BY CO, C4, C14, C15 ORDER BY CO ASC, C4 ASC, C14 ASC, C15 ASC;";
	request.SqlList.push_back(strSql);

	if (SendRequestData(&request))
	{
		if ((iLength = RecvResponseData()) > 0)
		{
			if (!ParseLivMemberDayPriceElement(&request, SResponseDataCache, iLength))
			{
				return false;
			}
			
			return true;
		}
	}

	return false;
}

// 解析Liv市场成员的日周期历史数据数目
bool 
CRedWine::ParseLivMemberDayPriceElement(void *ReqData, char *data, const int length)
{
	if (NULL == ReqData || NULL == data)
	{
		return false;
	}
	
	CResDataParser DataParser(data, length, (RequestPack*)ReqData);
	if (!DataParser.CheckValid())
	{
		return false;
	}

	string strColumn;
	string strMarkKey("");
	int iRow = DataParser.GetRecordRow();
	
	int iCount = 0;
	RedWineMark* pMark = NULL;
	int iMarkIndex = -1;
	for (int i = 0; i < iRow; i++)
	{
		PARSE_D(strColumn)
		strMarkKey = strColumn;

		PARSE_D(strColumn)
		strMarkKey += strColumn;
		
		PARSE_D(strColumn)
		strMarkKey += strColumn;

		PARSE_D(strColumn)
		strMarkKey += strColumn;

		PARSE_D(strColumn)
		iCount = StrToInt(strColumn);

		iMarkIndex = FindLivMemMarkIndex(strMarkKey);
		if (-1 == iMarkIndex)
		{
			iCount = 0;
			pMark = NULL;
			continue;
		}
		pMark = FindLivMemMark(iMarkIndex);
		if (NULL == pMark)
		{
			return false;
		}

		pMark->nDayPriceMemCount = iCount;
	}

	return true;
}

// 请求Liv市场成员的月周期历史数据数目
bool 
CRedWine::RequestLivMemberMonthPriceElement()
{
	RequestPack request;
	int iLength = 0;
	string strSql("");

	// 初始化请求数据
	request.Seq = htonl(GetSendSeqNum());
	request.TableId = htonl(12527);
	request.TableIdList = "12527";
	request.Count = htonl(1);
	strSql = "SELECT CO, C3, C6, C7, COUNT(*)  FROM ST12527_main \
		WHERE C4 = 3 AND C2 != '' AND C2 IS NOT NULL \
		GROUP BY CO, C3, C6, C7 ORDER BY CO ASC, C3 ASC, C6 ASC, C7 ASC;";
	request.SqlList.push_back(strSql);

	if (SendRequestData(&request))
	{
		if ((iLength = RecvResponseData()) > 0)
		{
			if (!ParseLivMemberMonthPriceElement(&request, SResponseDataCache, iLength))
			{
				return false;
			}
			
			return true;
		}
	}

	return false;
}

// 解析Liv市场成员的月周期历史数据数目
bool 
CRedWine::ParseLivMemberMonthPriceElement(void *ReqData, char *data, const int length)
{
	if (NULL == ReqData || NULL == data)
	{
		return false;
	}
	
	CResDataParser DataParser(data, length, (RequestPack*)ReqData);
	if (!DataParser.CheckValid())
	{
		return false;
	}

	string strColumn;
	string strMarkKey("");
	int iRow = DataParser.GetRecordRow();
	
	int iCount = 0;
	RedWineMark* pMark = NULL;
	int iMarkIndex = -1;
	for (int i = 0; i < iRow; i++)
	{
		PARSE_D(strColumn)
		strMarkKey = strColumn;

		PARSE_D(strColumn)
		strMarkKey += strColumn;
		
		PARSE_D(strColumn)
		strMarkKey += strColumn;

		PARSE_D(strColumn)
		strMarkKey += strColumn;

		PARSE_D(strColumn)
		iCount = StrToInt(strColumn);

		iMarkIndex = FindLivMemMarkIndex(strMarkKey);
		if (-1 == iMarkIndex)
		{
			iCount = 0;
			pMark = NULL;
			continue;
		}
		pMark = FindLivMemMark(iMarkIndex);
		if (NULL == pMark)
		{
			return false;
		}

		pMark->nMonthPriceMemCount = iCount;
	}

	return true;
}

// 请求拍卖市场全部成分酒数据
bool 
CRedWine::RequestAutionMemberList_S()
{
	RequestPack request;
	int iLength = 0;
	string strSql("");

	// 初始化请求数据
	request.Seq = htonl(GetSendSeqNum());
	request.TableId = htonl(12525);
	request.TableIdList = "12525";
	request.Count = htonl(1);
	strSql = "SELECT DISTINCT CO, C7, C8 FROM ST12525_main \
		WHERE CO != '' AND CO IS NOT NULL \
		AND C6 != '' AND C6 IS NOT NULL \
		ORDER BY CO ASC, C7 ASC, C8 ASC;";
	request.SqlList.push_back(strSql);

	// 接收数据结构
	int iMaxLength = 0;
	int iWarning = 0;
	char *pStorteData = sLargeMemoryCache.GetRemainMem(iMaxLength, &iWarning);
	if (NULL == pStorteData || iWarning > 0)
	{
		return false;
	}
	ResponseDataBuf RespData(string("RedWineAutionMemList"), pStorteData, iMaxLength, sizeof(RedwineAuctionMemEnumTag), 1);
	CWriteBuff bufWrite((char*)RespData.DataBuf, RespData.GetDataLength(), 0);

	if (SendRequestData(&request))
	{
		if ((iLength = RecvResponseData()) > 0)
		{
			if (!ParseAuctionMemberList(&request, SResponseDataCache, iLength, &bufWrite))
			{
				sLargeMemoryCache.ClearRemainMem();
				return false;
			}
			sLargeMemoryCache.SetRemainMemStart(bufWrite.GetNewdataLen());
			RespData.DataLength = bufWrite.GetNewdataLen();
			sWineData.insert(StringResPair(RespData.KeyType, RespData));
			return true;
		}
	}

	return false;
}

// 解析拍卖市场全部成分酒数据
bool 
CRedWine::ParseAuctionMemberList(void *ReqData, char *data, const int length, CWriteBuff *bufWrite)
{
	if (NULL == ReqData || NULL == data	|| NULL == bufWrite)
	{
		return false;
	}
	
	CResDataParser DataParser(data, length, (RequestPack*)ReqData);
	if (!DataParser.CheckValid())
	{
		return false;
	}

	string strColumn;
	string strKey("");
	string strParkKey("");
	string strMarkKey("");
	int iRow = DataParser.GetRecordRow();
	int iTypeSize = sizeof(RedwineAuctionMemEnumTag);
		
	// Head
	ZXCMDHEAD DataHead;
	DataHead.m_wCmdType = ZXCMD_REDWINE_AUCTION_MEMBER_DATA;
	DataHead.m_nAttr = 0;
	SetExHByteSerialToProperty(0, DataHead.m_nExData);
	
	// 跳过Head(开始位置为零，不加偏移量)
	bufWrite->SeekPos(sizeof(ZXCMDHEAD));

	int iBaseIndex = -1;
	int iMatchIndex = 0;
	RedwineAuctionMemEnumTag item;
	RedWineMark localItem;
	sRedWineAuctionMemStrKeyMap.clear();
	sRedWineAuctionMemIntKeyMap.clear();
	for (int i = 0; i < iRow; i++)
	{
		PARSE_D(strColumn)
		localItem.wNameCode = strColumn;
		strKey = strColumn;
		strParkKey = strColumn;
		strMarkKey = strColumn;
		
		PARSE_D(strColumn)
		item.wYear = StrToInt(strColumn);
		localItem.wYear = item.wYear;
		strParkKey += strColumn;
		strMarkKey += strColumn;

		PARSE_D(strColumn)
		item.nRedwineCapacity = FindRedwineCapacity(strColumn);
		localItem.wCapacity = strColumn;
		strMarkKey += strColumn;

		localItem.wNumber = 1;

		iBaseIndex = FindRedwineBaseInfor(strKey);
		// 过滤掉基准信息不存在的数据
		if (-1 == iBaseIndex)
		{
			continue;
		}
		item.nRedwineBaseInfor = iBaseIndex;

		// 帕克分数
		const RedwineParkRatinS *pParkRating = FindRedwineParkRating(strParkKey);
		item.nParkLRating = 0;
		item.nParkHRating = 0;
		if (NULL != pParkRating)
		{
			item.nParkLRating = pParkRating->nLowRating;
			item.nParkHRating = pParkRating->nHighRating;
		}


		localItem.nDayPriceMemCount = 0;
		localItem.nMonthPriceMemCount = 0;
		sRedWineAuctionMemStrKeyMap.insert(StringIntPair(strMarkKey, iMatchIndex));
		sRedWineAuctionMemIntKeyMap.insert(IntMarkPair(iMatchIndex++, localItem));

		if (!bufWrite->Push_back(&item, iTypeSize))
		{
			return false;
		}
	}

	// 写入Head
	bufWrite->SeekToBegin();
	DataHead.m_nLen = sRedWineAuctionMemStrKeyMap.size() * iTypeSize;
	if (!bufWrite->Push_set(&DataHead, sizeof(ZXCMDHEAD)))
	{
		return false;
	}
	bufWrite->SeekToEnd();

	return true;
}

// 查找指定酒标识的索引
int 
CRedWine::FindAuctionMemMarkIndex(const std::string &Key)  // 名称+年份+容量 作为关键字
{
	if (sRedWineAuctionMemStrKeyMap.size() < 1)
	{
		return -1;
	}

	StringIntIter findInterator = sRedWineAuctionMemStrKeyMap.find(Key);
	if (findInterator != sRedWineAuctionMemStrKeyMap.end())
	{
		return findInterator->second;
	}
	return -1;
}

// 查找指定酒的标识ID
RedWineMark* 
CRedWine::FindAuctionMemMark(const int index)
{
	if (sRedWineAuctionMemIntKeyMap.size() < 1)
	{
		return NULL;
	}

	IntMarkIter findInterator = sRedWineAuctionMemIntKeyMap.find(index);
	if (findInterator != sRedWineAuctionMemIntKeyMap.end())
	{
		return &(findInterator->second);
	}
	return NULL;
}

// 请求拍卖市场成员的日周期历史数据数目
bool 
CRedWine::RequestAuctionMemberDayPriceElement()
{
	RequestPack request;
	int iLength = 0;
	string strSql("");

	// 初始化请求数据
	request.Seq = htonl(GetSendSeqNum());
	request.TableId = htonl(12525);
	request.TableIdList = "12525";
	request.Count = htonl(1);
		strSql = "SELECT CO, C7, C8, COUNT(*) FROM ST12525_main \
		GROUP BY CO, C7, C8 ORDER BY CO ASC, C7 ASC, C8 ASC;";
	request.SqlList.push_back(strSql);

	if (SendRequestData(&request))
	{
		if ((iLength = RecvResponseData()) > 0)
		{
			if (!ParseAuctionMemberDayPriceElement(&request, SResponseDataCache, iLength))
			{
				return false;
			}
			
			return true;
		}
	}

	return false;
}

// 解析拍卖市场成员的日周期历史数据数目
bool 
CRedWine::ParseAuctionMemberDayPriceElement(void *ReqData, char *data, const int length)
{
	if (NULL == ReqData || NULL == data)
	{
		return false;
	}
	
	CResDataParser DataParser(data, length, (RequestPack*)ReqData);
	if (!DataParser.CheckValid())
	{
		return false;
	}

	string strColumn;
	string strMarkKey("");
	int iRow = DataParser.GetRecordRow();
	
	int iCount = 0;
	RedWineMark* pMark = NULL;
	int iMarkIndex = -1;
	for (int i = 0; i < iRow; i++)
	{
		PARSE_D(strColumn)
		strMarkKey = strColumn;

		PARSE_D(strColumn)
		strMarkKey += strColumn;
		
		PARSE_D(strColumn)
		strMarkKey += strColumn;

		PARSE_D(strColumn)
		iCount = StrToInt(strColumn);

		iMarkIndex = FindAuctionMemMarkIndex(strMarkKey);
		if (-1 == iMarkIndex)
		{
			iCount = 0;
			pMark = NULL;
			continue;
		}
		pMark = FindAuctionMemMark(iMarkIndex);
		if (NULL == pMark)
		{
			return false;
		}

		pMark->nDayPriceMemCount = iCount;
	}

	return true;
}

// 请求拍卖市场成员的月周期历史数据数目
bool 
CRedWine::RequestAuctionMemberMonthPriceElement()
{
	RequestPack request;
	int iLength = 0;
	string strSql("");

	// 初始化请求数据
	request.Seq = htonl(GetSendSeqNum());
	request.TableId = htonl(12527);
	request.TableIdList = "12527";
	request.Count = htonl(1);
	strSql = "SELECT CO, C3, C6, C7, COUNT(*)  FROM ST12527_main \
		WHERE C4 = 1 AND C2 != '' AND C2 IS NOT NULL \
		GROUP BY CO, C3, C6, C7 ORDER BY CO ASC, C3 ASC, C6 ASC, C7 ASC;";
	request.SqlList.push_back(strSql);

	if (SendRequestData(&request))
	{
		if ((iLength = RecvResponseData()) > 0)
		{
			if (!ParseAuctionMemberMonthPriceElement(&request, SResponseDataCache, iLength))
			{
				return false;
			}
			
			return true;
		}
	}

	return false;
}

// 解析拍卖市场成员的月周期历史数据数目
bool 
CRedWine::ParseAuctionMemberMonthPriceElement(void *ReqData, char *data, const int length)
{
	if (NULL == ReqData || NULL == data)
	{
		return false;
	}
	
	CResDataParser DataParser(data, length, (RequestPack*)ReqData);
	if (!DataParser.CheckValid())
	{
		return false;
	}

	string strColumn;
	string strMarkKey("");
	int iRow = DataParser.GetRecordRow();
	
	int iCount = 0;
	RedWineMark* pMark = NULL;
	int iMarkIndex = -1;
	for (int i = 0; i < iRow; i++)
	{
		PARSE_D(strColumn)
		strMarkKey = strColumn;

		PARSE_D(strColumn)
		strMarkKey += strColumn;
		
		PARSE_D(strColumn)
		strMarkKey += strColumn;

		PARSE_D(strColumn)
		
		PARSE_D(strColumn)
		iCount = StrToInt(strColumn);

		iMarkIndex = FindAuctionMemMarkIndex(strMarkKey);
		if (-1 == iMarkIndex)
		{
			iCount = 0;
			pMark = NULL;
			continue;
		}
		pMark = FindAuctionMemMark(iMarkIndex);
		if (NULL == pMark)
		{
			return false;
		}

		pMark->nMonthPriceMemCount = iCount;
	}

	return true;
}

// 请求零售市场全部成分酒数据
bool 
CRedWine::RequestRetailMemberList_S()
{
	RequestPack request;
	int iLength = 0;
	string strSql("");

	// 初始化请求数据
	request.Seq = htonl(GetSendSeqNum());
	request.TableId = htonl(12527);
	request.TableIdList = "12527";
	request.Count = htonl(1);
	strSql = "SELECT CO, C3, C6, C7, COUNT(*) FROM ST12527_main \
		WHERE C4 = 2 AND C2 != '' AND C2 IS NOT NULL \
		GROUP BY CO, C3, C6, C7 \
		ORDER BY CO ASC, C3 ASC, C6 ASC, C7 ASC;";
	request.SqlList.push_back(strSql);

	// 接收数据结构
	int iMaxLength = 0;
	int iWarning = 0;
	char *pStorteData = sLargeMemoryCache.GetRemainMem(iMaxLength, &iWarning);
	if (NULL == pStorteData || iWarning > 0)
	{
		return false;
	}
	ResponseDataBuf RespData(string("RedWineRetailMemList"), pStorteData, iMaxLength, sizeof(RedwineRetailMemEnumTag), 1);
	CWriteBuff bufWrite((char*)RespData.DataBuf, RespData.GetDataLength(), 0);

	if (SendRequestData(&request))
	{
		if ((iLength = RecvResponseData()) > 0)
		{
			if (!ParseRetailMemberList(&request, SResponseDataCache, iLength, &bufWrite))
			{
				sLargeMemoryCache.ClearRemainMem();
				return false;
			}
			sLargeMemoryCache.SetRemainMemStart(bufWrite.GetNewdataLen());
			RespData.DataLength = bufWrite.GetNewdataLen();
			sWineData.insert(StringResPair(RespData.KeyType, RespData));
			return true;
		}
	}

	return false;
}

// 解析零售市场全部成分酒数据
bool 
CRedWine::ParseRetailMemberList(void *ReqData, char *data, const int length, CWriteBuff *bufWrite)
{
	if (NULL == ReqData || NULL == data	|| NULL == bufWrite)
	{
		return false;
	}
	
	CResDataParser DataParser(data, length, (RequestPack*)ReqData);
	if (!DataParser.CheckValid())
	{
		return false;
	}

	string strColumn;
	string strKey("");
	string strMarkKey("");
	string strParkKey("");
	int iRow = DataParser.GetRecordRow();
	int iTypeSize = sizeof(RedwineRetailMemEnumTag);
		
	// Head
	ZXCMDHEAD DataHead;
	DataHead.m_wCmdType = ZXCMD_REDWINE_RETAIL_MEMBER_DATA;
	DataHead.m_nAttr = 0;
	SetExHByteSerialToProperty(0, DataHead.m_nExData);
	
	// 跳过Head(开始位置为零，不加偏移量)
	bufWrite->SeekPos(sizeof(ZXCMDHEAD));

	int iBaseIndex = -1;
	int iMatchIndex = 0;
	RedwineRetailMemEnumTag item;
	RedWineMark localItem;
	sRedWineRetailMemStrKeyMap.clear();
	sRedWineRetailMemIntKeyMap.clear();
	for (int i = 0; i < iRow; i++)
	{
		PARSE_D(strColumn)
		localItem.wNameCode = strColumn;
		strKey = strColumn;
		strParkKey = strColumn;
		strMarkKey = strColumn;
		
		PARSE_D(strColumn)		
		item.wYear = StrToInt(strColumn);
		localItem.wYear = item.wYear;
		strParkKey += strColumn;
		strMarkKey += strColumn;

		PARSE_D(strColumn)
		item.nRedwineCapacity = FindRedwineCapacity(strColumn);
		localItem.wCapacity = strColumn;
		strMarkKey += strColumn;

		PARSE_D(strColumn)
		item.wNumber = StrToInt(strColumn);
		localItem.wNumber = item.wNumber;
		strMarkKey += strColumn;

		PARSE_D(strColumn)
		localItem.nMonthPriceMemCount = StrToInt(strColumn);
		localItem.nDayPriceMemCount = 0;

		iBaseIndex = FindRedwineBaseInfor(strKey);
		// 过滤掉基准信息不存在的数据
		if (-1 == iBaseIndex)
		{
			continue;
		}
		item.nRedwineBaseInfor = iBaseIndex;

		// 帕克分数
		const RedwineParkRatinS *pParkRating = FindRedwineParkRating(strParkKey);
		item.nParkLRating = 0;
		item.nParkHRating = 0;
		if (NULL != pParkRating)
		{
			item.nParkLRating = pParkRating->nLowRating;
			item.nParkHRating = pParkRating->nHighRating;
		}

		sRedWineRetailMemStrKeyMap.insert(StringIntPair(strMarkKey, iMatchIndex));
		sRedWineRetailMemIntKeyMap.insert(IntMarkPair(iMatchIndex++, localItem));
		if (!bufWrite->Push_back(&item, iTypeSize))
		{
			return false;
		}
	}

	// 写入Head
	bufWrite->SeekToBegin();
	DataHead.m_nLen = sRedWineRetailMemStrKeyMap.size() * iTypeSize;
	if (!bufWrite->Push_set(&DataHead, sizeof(ZXCMDHEAD)))
	{
		return false;
	}
	bufWrite->SeekToEnd();

	return true;
}

// 查找指定酒标识的索引
int 
CRedWine::FindRetailMemMarkIndex(const std::string &Key)
{
	if (sRedWineRetailMemStrKeyMap.size() < 1)
	{
		return -1;
	}

	StringIntIter findInterator = sRedWineRetailMemStrKeyMap.find(Key);
	if (findInterator != sRedWineRetailMemStrKeyMap.end())
	{
		return findInterator->second;
	}
	return -1;
}

// 查找指定酒的标识ID
RedWineMark* 
CRedWine::FindRetailMemMark(const int index)
{
	if (sRedWineRetailMemIntKeyMap.size() < 1)
	{
		return NULL;
	}

	IntMarkIter findInterator = sRedWineRetailMemIntKeyMap.find(index);
	if (findInterator != sRedWineRetailMemIntKeyMap.end())
	{
		return &(findInterator->second);
	}
	return NULL;
}

// 请求零售最新报价
bool
CRedWine::RequestRetailLatestPrice_S()
{
	RequestPack request;
	int iLength = 0;
	string strSql("");	
	char cCondtion[128] = {0};

	// 基本请求语句
	string strBaseSql = "SELECT CO, C3, C13, C14, DATE_FORMAT(C1, '%Y%m%d'), C5, C7 FROM ST12506_main WHERE C6 != 1 \
		ORDER BY CO ASC, C3 ASC, C13 ASC, C14 ASC, C1 DESC ";

	// 数据量
	int iTotalSegCount = sRedWineRetailMemIntKeyMap.size();
	if (iTotalSegCount < 1)
	{
		return false;
	}

	// 类型长度
	int iTypeSize = sizeof(RedwineRetailLatestPriceTag);

	// 接收数据结构
	int iMaxLength = 0;
	int iWarning = 0;
	char *pStorteData = sLargeMemoryCache.GetRemainMem(iMaxLength, &iWarning);
	if (NULL == pStorteData || iWarning > 0)
	{
		return false;
	}
	ResponseDataBuf RespData(string("RedwineRetailLatestPrice"), pStorteData, iMaxLength, iTypeSize, iTotalSegCount);
	CWriteBuff bufWrite((char*)RespData.DataBuf, RespData.GetDataLength(), 0);

	// Head
	ZXCMDHEAD DataHead;
	DataHead.m_wCmdType = ZXCMD_REDWINE_RETAIL_LATEST_PRICE;
	DataHead.m_nAttr = 0;
	SetExHByteSerialToProperty(0, DataHead.m_nExData);
	DataHead.m_nLen = iTotalSegCount * iTypeSize;
	if (!bufWrite.Push_back(&DataHead, sizeof(ZXCMDHEAD)))
	{
		return false;
	}

	// 分次循环请求数据
	int iSerial = 0;
	size_t iCount = 0;
	for (size_t i = 0; i < MaxCycleTimes; i++)
	{
		// 初始化请求数据
		request.SqlList.clear();
		request.Seq = htonl(GetSendSeqNum());
		request.TableId = htonl(12506);
		request.TableIdList = "12506";
		request.Count = htonl(1);

		// 限定数目条件
		sprintf(cCondtion, " LIMIT %d, %d;", i * MaxRecordPerCycle, MaxRecordPerCycle);

		strSql = strBaseSql + string(cCondtion);
		request.SqlList.push_back(strSql);
		
		if (SendRequestData(&request))
		{
			if ((iLength = RecvResponseData()) > 0)
			{
				if (!ParseRetailLatestPrice(iSerial, &request, SResponseDataCache, iLength, &bufWrite, iCount))
				{
					sLargeMemoryCache.ClearRemainMem();
					return false;
				}
				else if (iCount < MaxRecordPerCycle)
				{
					break;
				}
			}
			else 
			{
				sLargeMemoryCache.ClearRemainMem();
				return false;
			}
		}
	}

	if (iSerial != iTotalSegCount)
	{
		sLargeMemoryCache.ClearRemainMem();
		return false;
	}

	sLargeMemoryCache.SetRemainMemStart(bufWrite.GetNewdataLen(), &iWarning);
	RespData.DataLength = bufWrite.GetNewdataLen();
	sWineData.insert(StringResPair(RespData.KeyType, RespData));

	return true;
}

// 解析零售最新报价
bool
CRedWine::ParseRetailLatestPrice(int &Serial, void *ReqData, char *data, const int length, CWriteBuff *bufWrite, size_t &ItemCount)
{
	if (NULL == ReqData || NULL == data || NULL == bufWrite)
	{
		return false;
	}

	CResDataParser DataParser(data, length, (RequestPack*)ReqData);
	if (!DataParser.CheckValid())
	{
		return false;
	}

	int iTypeSize = sizeof(RedwineRetailLatestPriceTag);
	int iRow = DataParser.GetRecordRow();
	ItemCount = iRow;
		
	RedwineRetailLatestPriceTag item;
	string strColumn;
	string strKey("");
	static string strPreKey("");

	if (0 == Serial)
	{
		strPreKey = "";
	}
	for (int i = 0; i < iRow; i++)
	{
		PARSE_D(strColumn)
		strKey = strColumn;

		PARSE_D(strColumn)
		strKey += strColumn;

		PARSE_D(strColumn)
		strKey += strColumn;

		PARSE_D(strColumn)
		strKey += strColumn;

		PARSE_D(strColumn)
		item.qTime = StrToUl(strColumn);

		PARSE_D(strColumn)
		item.wMoneyUnit = FindMoneyUnit(strColumn);
		
		PARSE_D(strColumn)
		item.wQuotePrice = (DWORD)(CRedWine::StrToDouble(strColumn) * ZoomScaleNum);

		if (strKey != strPreKey) // found
		{
			strPreKey = strKey;

			int iFindSerial = FindRetailMemMarkIndex(strKey);
			if (iFindSerial < 0)
			{
				continue;
			}

			if (iFindSerial < Serial)
			{
				continue;
			}
			else if (iFindSerial > Serial)
			{
				// 此时插入空数据
				while(Serial < iFindSerial)
				{
					RedwineRetailLatestPriceTag itemTemp;
					itemTemp.qTime = 0;
					itemTemp.wMoneyUnit = 0;
					itemTemp.wQuotePrice = 0;

					// item
					if (!bufWrite->Push_back(&itemTemp, iTypeSize))
					{
						return false;
					}
					Serial++;
				}
			}

			// item
			if (!bufWrite->Push_back(&item, iTypeSize))
			{
				return false;
			}
			Serial++;			
		}
	}
	
	return true;
}

// 请求红酒三个市场酒名称索引列表
bool 
CRedWine::RequestClassifiedNameListIndex_S()
{
	RequestPack request;
	int iLength = 0;
	string strSql("");
	int iSerial = 0;

	// 初始化请求数据(Liv市场)
	request.Seq = htonl(GetSendSeqNum());
	request.TableId = htonl(12521);
	request.TableIdList = "12521";
	request.Count = htonl(1);
	strSql = "SELECT CO FROM ST12521_main GROUP BY CO ORDER BY C3 ASC;";
	request.SqlList.push_back(strSql);

	// 接收数据结构
	int iMaxLength = 0;
	int iWarning = 0;
	char *pStorteData = sLargeMemoryCache.GetRemainMem(iMaxLength, &iWarning);
	if (NULL == pStorteData || iWarning > 0)
	{
		return false;
	}
	ResponseDataBuf RespData(string("RedWineClassifiedNameListIndex"), pStorteData, iMaxLength, sizeof(WORD), 3);
	CWriteBuff bufWrite((char*)RespData.DataBuf, RespData.GetDataLength(), 0);

	if (!SendRequestData(&request))
	{
		return false;
	}
	if ((iLength = RecvResponseData()) > 0)
	{
		if (!ParseClassifiedNameListIndex(RespData.SegCount, iSerial, &request, SResponseDataCache, iLength, &bufWrite))
		{
			sLargeMemoryCache.ClearRemainMem();
			return false;
		}		
	}
	else
	{
		sLargeMemoryCache.ClearRemainMem();
		return false;
	}

	// 初始化请求数据(拍卖市场)
	request.Seq = htonl(GetSendSeqNum());
	request.TableId = htonl(12525);
	request.TableIdList = "12525";
	request.Count = htonl(1);
	strSql = "SELECT CO FROM ST12525_main GROUP BY CO ORDER BY C6 ASC;";
	request.SqlList.clear();
	request.SqlList.push_back(strSql);

	if (!SendRequestData(&request))
	{
		return false;
	}
	if ((iLength = RecvResponseData()) > 0)
	{
		if (!ParseClassifiedNameListIndex(RespData.SegCount, iSerial, &request, SResponseDataCache, iLength, &bufWrite))
		{
			sLargeMemoryCache.ClearRemainMem();
			return false;
		}		
	}
	else
	{
		sLargeMemoryCache.ClearRemainMem();
		return false;
	}

	// 初始化请求数据(零售市场)
	request.Seq = htonl(GetSendSeqNum());
	request.TableId = htonl(12527);
	request.TableIdList = "12527";
	request.Count = htonl(1);
	strSql = "SELECT CO FROM ST12527_main WHERE C4 = 2 GROUP BY CO ORDER BY C2 ASC;";
	request.SqlList.clear();
	request.SqlList.push_back(strSql);

	if (!SendRequestData(&request))
	{
		return false;
	}
	if ((iLength = RecvResponseData()) > 0)
	{
		if (!ParseClassifiedNameListIndex(RespData.SegCount, iSerial, &request, SResponseDataCache, iLength, &bufWrite))
		{
			sLargeMemoryCache.ClearRemainMem();
			return false;
		}		
	}
	else
	{
		sLargeMemoryCache.ClearRemainMem();
		return false;
	}

	sLargeMemoryCache.SetRemainMemStart(bufWrite.GetNewdataLen());
	RespData.DataLength = bufWrite.GetNewdataLen();
	sWineData.insert(StringResPair(RespData.KeyType, RespData));
	return true;
}

// 解析红酒三个市场酒名称索引列表
bool 
CRedWine::ParseClassifiedNameListIndex(const int TotalCount, int &Serial, void *ReqData, char *data, 
									   const int length, CWriteBuff *bufWrite)
{
	if (NULL == ReqData || NULL == data	|| NULL == bufWrite)
	{
		return false;
	}
	
	CResDataParser DataParser(data, length, (RequestPack*)ReqData);
	if (!DataParser.CheckValid())
	{
		return false;
	}

	string strColumn;
	int iRow = DataParser.GetRecordRow();
	int iTypeSize = sizeof(WORD);
	int iDatalength = iTypeSize * iRow;
		
	// Head
	ZXCMDHEAD DataHead;
	DataHead.m_wCmdType = ZXCMD_REDWINE_CLASSIFIED_NAME_LIST;
	DataHead.m_nAttr = Serial == (TotalCount - 1) ? 0 : TotalCount;
	SetExHByteSerialToProperty(Serial++, DataHead.m_nExData);
	DataHead.m_nLen = iDatalength;
	if (!bufWrite->Push_back(&DataHead, sizeof(ZXCMDHEAD)))
	{
		return false;
	}

	WORD item;
	for (int i = 0; i < iRow; i++)
	{
		PARSE_D(strColumn)
		item = FindRedwineName(strColumn);
		
		if (!bufWrite->Push_back(&item, iTypeSize))
		{
			return false;
		}
	}

	return true;
}

// 请求红酒拍卖结果查询结果数据酒标识列表
bool 
CRedWine::RequestAuctionResSearchMemEnum_S()
{
	RequestPack request;
	int iLength = 0;
	string strSql("");

	// 初始化请求数据
	request.Seq = htonl(GetSendSeqNum());
	request.TableId = htonl(12505);
	request.TableIdList = "12505";
	request.Count = htonl(1);
	strSql = "SELECT CO, C3, C25, COUNT(*) FROM ST12505_main WHERE C24 = 1 \
      GROUP BY CO, C3, C25 \
      ORDER BY CO ASC, C3 ASC, C25 ASC;";
	request.SqlList.push_back(strSql);

	// 接收数据结构
	int iMaxLength = 0;
	int iWarning = 0;
	char *pStorteData = sLargeMemoryCache.GetRemainMem(iMaxLength, &iWarning);
	if (NULL == pStorteData || iWarning > 0)
	{
		return false;
	}
	ResponseDataBuf RespData(string("AuctionResSearchMemEnum"), pStorteData, iMaxLength, sizeof(RedwineAuctionResSearchMemEnumTag), 1);
	CWriteBuff bufWrite((char*)RespData.DataBuf, RespData.GetDataLength(), 0);

	if (SendRequestData(&request))
	{
		if ((iLength = RecvResponseData()) > 0)
		{
			if (!ParseAuctionResSearchMemEnum(&request, SResponseDataCache, iLength, &bufWrite))
			{
				sLargeMemoryCache.ClearRemainMem();
				return false;
			}
			sLargeMemoryCache.SetRemainMemStart(bufWrite.GetNewdataLen());
			RespData.DataLength = bufWrite.GetNewdataLen();
			sWineData.insert(StringResPair(RespData.KeyType, RespData));
			return true;
		}
	}

	return false;
}

// 解析红酒拍卖结果查询结果数据酒标识列表
bool 
CRedWine::ParseAuctionResSearchMemEnum(void *ReqData, char *data, const int length, CWriteBuff *bufWrite)
{
	if (NULL == ReqData || NULL == data	|| NULL == bufWrite)
	{
		return false;
	}
	
	CResDataParser DataParser(data, length, (RequestPack*)ReqData);
	if (!DataParser.CheckValid())
	{
		return false;
	}

	string strColumn;
	string strKey("");
	string strMarkKey("");
	int iRow = DataParser.GetRecordRow();
	int iTypeSize = sizeof(RedwineAuctionResSearchMemEnumTag);
		
	// Head
	ZXCMDHEAD DataHead;
	DataHead.m_wCmdType = ZXCMD_REDWINE_AUCTION_SEARCH_MEM_DATA;
	DataHead.m_nAttr = 0;
	SetExHByteSerialToProperty(0, DataHead.m_nExData);
	
	// 跳过Head(开始位置为零，不加偏移量)
	bufWrite->SeekPos(sizeof(ZXCMDHEAD));

	int iBaseIndex = -1;
	int iMatchIndex = 0;
	RedwineAuctionResSearchMemEnumTag item;
	RedWineMark localItem;
	sRedWineAuctionResSrchMarkUseStrKeyMap.clear();
	sRedWineAuctionResSrchMarkUseIntKeyMap.clear();
	for (int i = 0; i < iRow; i++)
	{
		PARSE_D(strColumn)
		strKey = strColumn;
		strMarkKey = strColumn;
		localItem.wNameCode = strColumn;
		
		PARSE_D(strColumn)
		item.wYear = StrToInt(strColumn);
		strMarkKey += strColumn;
		localItem.wYear = item.wYear;

		PARSE_D(strColumn)
		item.nRedwineCapacity = FindRedwineCapacity(strColumn);
		strMarkKey += strColumn;
		localItem.wCapacity = strColumn;

		PARSE_D(strColumn)
		localItem.nDayPriceMemCount = StrToInt(strColumn);

		localItem.wNumber = 1;
		localItem.nMonthPriceMemCount = 0;

		iBaseIndex = FindRedwineBaseInfor(strKey);
		// 过滤掉基准信息不存在的数据
		if (-1 == iBaseIndex)
		{
			continue;
		}
		item.nRedwineBaseInfor = iBaseIndex;

		sRedWineAuctionResSrchMarkUseStrKeyMap.insert(StringIntPair(strMarkKey, iMatchIndex));
		sRedWineAuctionResSrchMarkUseIntKeyMap.insert(IntMarkPair(iMatchIndex++, localItem));
		if (!bufWrite->Push_back(&item, iTypeSize))
		{
			return false;
		}
	}

	// 写入Head
	bufWrite->SeekToBegin();
	DataHead.m_nLen = sRedWineAuctionResSrchMarkUseStrKeyMap.size() * iTypeSize;
	if (!bufWrite->Push_set(&DataHead, sizeof(ZXCMDHEAD)))
	{
		return false;
	}
	bufWrite->SeekToEnd();

	return true;
}

// 查找指定酒标识的索引
int 
CRedWine::FindAuctionResSearchMemMarkIndex(const std::string &Key)  // 酒名称代码+年份+容量 作为关键字
{
	if (sRedWineAuctionResSrchMarkUseStrKeyMap.size() < 1)
	{
		return -1;
	}

	StringIntIter findInterator = sRedWineAuctionResSrchMarkUseStrKeyMap.find(Key);
	if (findInterator != sRedWineAuctionResSrchMarkUseStrKeyMap.end())
	{
		return findInterator->second;
	}
	return -1;
}

// 返回拍卖指定酒标识
RedWineMark* 
CRedWine::FindAuctionResSearchMemMark(const int index)
{
	if (sRedWineAuctionResSrchMarkUseIntKeyMap.size() < 1)
	{
		return NULL;
	}

	IntMarkIter findInterator = sRedWineAuctionResSrchMarkUseIntKeyMap.find(index);
	if (findInterator != sRedWineAuctionResSrchMarkUseIntKeyMap.end())
	{
		return &(findInterator->second);
	}
	return NULL;
}

// 请求红酒拍卖结果查询关键字 (包含数量统计)
bool 
CRedWine::RequestAuctionResSearchKey_S()
{
	RequestPack request;
	int iLength = 0;
	string strSql("");

	// 初始化请求数据
	request.Seq = htonl(GetSendSeqNum());
	request.TableId = htonl(12505);
	request.TableIdList = "12505";
	request.Count = htonl(1);
	strSql = "SELECT C14, C12, COUNT(*) FROM ST12505_main WHERE C14 IS NOT NULL AND C12 IS NOT NULL AND C24 = 1 \
      GROUP BY C14, C12 \
      ORDER BY C14 ASC, C12 ASC;";
	request.SqlList.push_back(strSql);

	// 接收数据结构
	int iMaxLength = 0;
	int iWarning = 0;
	char *pStorteData = sLargeMemoryCache.GetRemainMem(iMaxLength, &iWarning);
	if (NULL == pStorteData || iWarning > 0)
	{
		return false;
	}
	ResponseDataBuf RespData(string("AuctionResSearchKeyList"), pStorteData, iMaxLength, sizeof(RedwineAuctionResSearchKeyTag), 1);
	CWriteBuff bufWrite((char*)RespData.DataBuf, RespData.GetDataLength(), 0);

	if (SendRequestData(&request))
	{
		if ((iLength = RecvResponseData()) > 0)
		{
			if (!ParseAuctionResSearchKey(&request, SResponseDataCache, iLength, &bufWrite))
			{
				sLargeMemoryCache.ClearRemainMem();
				return false;
			}
			sLargeMemoryCache.SetRemainMemStart(bufWrite.GetNewdataLen());
			RespData.DataLength = bufWrite.GetNewdataLen();
			sWineData.insert(StringResPair(RespData.KeyType, RespData));
			return true;
		}
	}

	return false;
}

// 解析红酒拍卖结果查询关键字(地点+拍卖机构)组合
bool 
CRedWine::ParseAuctionResSearchKey(void *ReqData, char *data, const int length, CWriteBuff *bufWrite)
{
	if (NULL == ReqData || NULL == data	|| NULL == bufWrite)
	{
		return false;
	}
	
	CResDataParser DataParser(data, length, (RequestPack*)ReqData);
	if (!DataParser.CheckValid())
	{
		return false;
	}

	string strColumn;
	string strKey("");
	int iRow = DataParser.GetRecordRow();
	int iTypeSize = sizeof(RedwineAuctionResSearchKeyTag);
	int iDatalength = iTypeSize * iRow;
		
	// Head
	ZXCMDHEAD DataHead;
	DataHead.m_wCmdType = ZXCMD_REDWINE_AUCTION_SEARCH_KEY_DATA;
	DataHead.m_nAttr = 0;
	SetExHByteSerialToProperty(0, DataHead.m_nExData);
	DataHead.m_nLen = iDatalength;
	if (!bufWrite->Push_back(&DataHead, sizeof(ZXCMDHEAD)))
	{
		return false;
	}

	RedwineAuctionResSearchKeyTag item;	
	RedwineAuctionResMark localItem;
	sRedWineAuctionResAddHouseUseStrKeyMap.clear();
	sRedWineAuctionResAddHouseUseIntKeyMap.clear();
	for (int i = 0; i < iRow; i++)
	{
		PARSE_D(strColumn)
		item.wSaleAddr = FindSaleAddress(strColumn);
		localItem.auAddr = strColumn;
		strKey = strColumn;

		PARSE_D(strColumn)
		item.wAuctionAddr = FindAuctionHouse(strColumn);
		localItem.auHouse = strColumn;
		strKey += strColumn;

		PARSE_D(strColumn)
		localItem.count = StrToInt(strColumn);

		sRedWineAuctionResAddHouseUseStrKeyMap.insert(StringIntPair(strKey, i));
		sRedWineAuctionResAddHouseUseIntKeyMap.insert(pair<int, RedwineAuctionResMark>(i, localItem));

		if (!bufWrite->Push_back(&item, iTypeSize))
		{
			return false;
		}
	}

	return true;
}

// 查找拍卖结果查询关键字的索引 拍卖地点+拍卖行 作为关键字
int 
CRedWine::FindAuctionResSearchKeyIndex(const std::string &Key)
{
	if (sRedWineAuctionResAddHouseUseStrKeyMap.size() < 1)
	{
		return -1;
	}

	StringIntIter findInterator = sRedWineAuctionResAddHouseUseStrKeyMap.find(Key);
	if (findInterator != sRedWineAuctionResAddHouseUseStrKeyMap.end())
	{
		return findInterator->second;
	}
	return -1;
}

// 查找拍卖结果查询关键字ID
RedwineAuctionResMark* 
CRedWine::FindAuctionResSearchKey(const int index)
{
	if (sRedWineAuctionResAddHouseUseIntKeyMap.size() < 1)
	{
		return NULL;
	}

	map<int, RedwineAuctionResMark>::iterator findInterator = sRedWineAuctionResAddHouseUseIntKeyMap.find(index);
	if (findInterator != sRedWineAuctionResAddHouseUseIntKeyMap.end())
	{
		return &(findInterator->second);
	}
	return NULL;
}

// 获取拍卖结果全部数据
bool 
CRedWine::RequestAuctionResSearchData_S()
{
	RequestPack request;
	int iLength = 0;
	string strSql("");	
	char cCondtion[128] = {0};

	// 基本请求语句
	string strBaseSql = "SELECT C14, C12, CO, C3, C25, DATE_FORMAT(C1, '%Y%m%d') AS qTime, C5 AS uMoneyUnit, C7 AS wAmount, \
      ROUND(C7 / C20, 2) AS wSingleUnitPrice, C21 AS lValuation, C22 AS hValuation, C20 AS nQuantity \
      FROM ST12505_main WHERE C24 = 1 \
      ORDER BY C14 ASC, C12 ASC, CO ASC, C3 ASC, C25 ASC, qTime ASC ";

	// 数据量
	int iTotalSegCount = sRedWineAuctionResAddHouseUseStrKeyMap.size();
	if (iTotalSegCount < 1)
	{
		return false;
	}

	// 接收数据结构
	int iMaxLength = 0;
	int iWarning = 0;
	char *pStorteData = sLargeMemoryCache.GetRemainMem(iMaxLength, &iWarning);
	if (NULL == pStorteData || iWarning > 0)
	{
		return false;
	}
	ResponseDataBuf RespData(string("AuctionResSearchDataList"), pStorteData, iMaxLength, sizeof(RedwineAuctionResSearchDataTag), iTotalSegCount);
	CWriteBuff bufWrite((char*)RespData.DataBuf, RespData.GetDataLength(), 0);

	// 分次循环请求数据
	int iSerial = 0;
	size_t iCount = 0;
	for (size_t i = 0; i < MaxCycleTimes; i++)
	{
		// 初始化请求数据
		request.SqlList.clear();
		request.Seq = htonl(GetSendSeqNum());
		request.TableId = htonl(12505);
		request.TableIdList = "12505";
		request.Count = htonl(1);

		// 限定数目条件
		sprintf(cCondtion, " LIMIT %d, %d;", i * MaxRecordPerCycle, MaxRecordPerCycle);

		strSql = strBaseSql + string(cCondtion);
		request.SqlList.push_back(strSql);
		
		if (SendRequestData(&request))
		{
			if ((iLength = RecvResponseData()) > 0)
			{
				if (!ParseAuctionResSearchData(iTotalSegCount, iSerial, &request, SResponseDataCache, iLength, 
					&bufWrite, iCount))
				{
					sLargeMemoryCache.ClearRemainMem();
					return false;
				}
				else if (iCount < MaxRecordPerCycle)
				{
					break;
				}
			}
			else 
			{
				sLargeMemoryCache.ClearRemainMem();
				return false;
			}
		}
	}

	sLargeMemoryCache.SetRemainMemStart(bufWrite.GetNewdataLen(), &iWarning);
	RespData.DataLength = bufWrite.GetNewdataLen();
	sWineData.insert(StringResPair(RespData.KeyType, RespData));

	return true;
}

// 解析拍卖结果全部数据
bool 
CRedWine::ParseAuctionResSearchData(const int TotalCount, int &Serial, void *ReqData, char *data, const int length, 
			   CWriteBuff *bufWrite, size_t &ItemCount)
{
	if (NULL == ReqData || NULL == data || NULL == bufWrite)
	{
		return false;
	}

	CResDataParser DataParser(data, length, (RequestPack*)ReqData);
	if (!DataParser.CheckValid())
	{
		return false;
	}

	int iTypeSize = sizeof(RedwineAuctionResSearchDataTag);
	int iRow = DataParser.GetRecordRow();
	ItemCount = iRow;
	
	RedwineAuctionResSearchDataTag item;
	string strColumn;
	string strKey("");
	RedwineAuctionResMark *pMark = NULL;
	static int iSerialCount = 0;
	bool bFindCurrentSeriaData = false;
	RedwineAuctionResMark tempItem;
	for (int i = 0; i < iRow; i++)
	{
		PARSE_D(strColumn)
		tempItem.auAddr = strColumn;

		PARSE_D(strColumn)
		tempItem.auHouse = strColumn;

		PARSE_D(strColumn)
		strKey = strColumn;

		PARSE_D(strColumn)
		strKey += strColumn;

		PARSE_D(strColumn)
		strKey += strColumn;

		PARSE_D(strColumn)
		item.qTime = StrToUl(strColumn);

		PARSE_D(strColumn)
		item.wMoneyUnit = FindMoneyUnit(strColumn);
		
		PARSE_D(strColumn)
		item.wAmount = (DWORD)(StrToDouble(strColumn) * ZoomScaleNum);

		PARSE_D(strColumn)
		item.wSingleUnitPrice = (DWORD)(CRedWine::StrToDouble(strColumn) * ZoomScaleNum);

		PARSE_D(strColumn)
		item.wLValuationPrice = (DWORD)(CRedWine::StrToDouble(strColumn) * ZoomScaleNum);

		PARSE_D(strColumn)
		item.wHValuationPrice = (DWORD)(CRedWine::StrToDouble(strColumn) * ZoomScaleNum);

		PARSE_D(strColumn)
		item.nAuctionQuantity = StrToInt(strColumn);

		if (0 == iSerialCount)
		{
			while(Serial < TotalCount)
			{
				pMark = FindAuctionResSearchKey(Serial);
				if (NULL == pMark)
				{
					return false;
				}
				iSerialCount = pMark->count;

				// Head
				ZXCMDHEAD DataHead;
				DataHead.m_wCmdType = ZXCMD_REDWINE_AUCTION_SEARCH_HIS_DATA;
				DataHead.m_nAttr = Serial == (TotalCount - 1) ? 0 : TotalCount;
				SetExHByteSerialToProperty(Serial++, DataHead.m_nExData);
				DataHead.m_nLen = iSerialCount * iTypeSize;
				if (!bufWrite->Push_back(&DataHead, sizeof(ZXCMDHEAD)))
				{
					return false;
				}

				if (0 == iSerialCount)	// 插入空包
				{
					continue;
				}
				else if (iSerialCount > 0)
				{
					if (*pMark == tempItem)
					{
						bFindCurrentSeriaData = true;						
					}
					else 
					{
						bFindCurrentSeriaData = false;
					}
					break;
				}
			}
		}
		else if (iSerialCount > 0 && NULL == pMark)	// 接前包
		{
			pMark = FindAuctionResSearchKey(Serial - 1);
			if (NULL == pMark)
			{
				return false;
			}
		}

		if (!bFindCurrentSeriaData)
		{
			if (*pMark == tempItem)
			{
				bFindCurrentSeriaData = true;
			}
			else 
			{
				bFindCurrentSeriaData = false;
				continue;
			}			
		}
		printf("Serial = %d \n", Serial);

		// item
		if (!bufWrite->Push_back(&item, iTypeSize))
		{
			return false;
		}
		iSerialCount--;
		
	}
	
	return true;
}

// 请求拍卖结果按照酒种类划分的全部数据
bool 
CRedWine::RequestAuctionResSingleTypeData_S()
{
	RequestPack request;
	int iLength = 0;
	string strSql("");	
	char cCondtion[128] = {0};

	// 基本请求语句
	string strBaseSql = "SELECT CO, C3, C25, C14, C12, C20 AS nQuantity, C7 AS wAmount, C5 AS uMoneyUnit, DATE_FORMAT(C1, '%Y%m%d') AS qTime \
      FROM ST12505_main WHERE C24 = 1 \
      ORDER BY CO ASC, C3 ASC, C25 ASC, qTime ASC ";

	// 数据量
	int iTotalSegCount = sRedWineAuctionResSrchMarkUseIntKeyMap.size();
	if (iTotalSegCount < 1)
	{
		return false;
	}

	// 接收数据结构
	int iMaxLength = 0;
	int iWarning = 0;
	char *pStorteData = sLargeMemoryCache.GetRemainMem(iMaxLength, &iWarning);
	if (NULL == pStorteData || iWarning > 0)
	{
		return false;
	}
	ResponseDataBuf RespData(string("AuctionResByWineTypeDataList"), pStorteData, iMaxLength, sizeof(RedwineAuctBySingleTypeDataTag), iTotalSegCount);
	CWriteBuff bufWrite((char*)RespData.DataBuf, RespData.GetDataLength(), 0);

	// 分次循环请求数据
	int iSerial = 0;
	size_t iCount = 0;
	for (size_t i = 0; i < MaxCycleTimes; i++)
	{
		// 初始化请求数据
		request.SqlList.clear();
		request.Seq = htonl(GetSendSeqNum());
		request.TableId = htonl(12505);
		request.TableIdList = "12505";
		request.Count = htonl(1);

		// 限定数目条件
		sprintf(cCondtion, " LIMIT %d, %d;", i * MaxRecordPerCycle, MaxRecordPerCycle);

		strSql = strBaseSql + string(cCondtion);
		request.SqlList.push_back(strSql);
		
		if (SendRequestData(&request))
		{
			if ((iLength = RecvResponseData()) > 0)
			{
				if (!ParseAuctionResSingleTypeData(iTotalSegCount, iSerial, &request, SResponseDataCache, iLength, 
					&bufWrite, iCount))
				{
					sLargeMemoryCache.ClearRemainMem();
					return false;
				}
				else if (iCount < MaxRecordPerCycle)
				{
					break;
				}
			}
			else 
			{
				sLargeMemoryCache.ClearRemainMem();
				return false;
			}
		}
	}

	sLargeMemoryCache.SetRemainMemStart(bufWrite.GetNewdataLen(), &iWarning);
	RespData.DataLength = bufWrite.GetNewdataLen();
	sWineData.insert(StringResPair(RespData.KeyType, RespData));

	return true;
}

// 解析拍卖结果按照酒种类划分的全部数据
bool 
CRedWine::ParseAuctionResSingleTypeData(const int TotalCount, int &Serial, void *ReqData, char *data, const int length, 
			   CWriteBuff *bufWrite, size_t &ItemCount)
{
	if (NULL == ReqData || NULL == data || NULL == bufWrite)
	{
		return false;
	}

	CResDataParser DataParser(data, length, (RequestPack*)ReqData);
	if (!DataParser.CheckValid())
	{
		return false;
	}

	int iTypeSize = sizeof(RedwineAuctBySingleTypeDataTag);
	int iRow = DataParser.GetRecordRow();
	ItemCount = iRow;
	
	RedwineAuctBySingleTypeDataTag item;
	string strColumn;
	string strKey("");
	RedWineMark *pMark = NULL;
	static int iSerialCount = 0;
	bool bFindCurrentSeriaData = false;
	RedWineMark tempItem;
	for (int i = 0; i < iRow; i++)
	{
		PARSE_D(strColumn)
		tempItem.wNameCode = strColumn;

		PARSE_D(strColumn)
		tempItem.wYear = StrToInt(strColumn);

		PARSE_D(strColumn)
		tempItem.wCapacity = strColumn;

		tempItem.wNumber = 1;

		PARSE_D(strColumn)
		strKey = strColumn;

		PARSE_D(strColumn)
		strKey += strColumn;

		item.nAuctAddKeyIndex = FindAuctionResSearchKeyIndex(strKey);

		PARSE_D(strColumn)
		item.nAuctionQuantity = StrToInt(strColumn);

		PARSE_D(strColumn)
		item.wAmount = (DWORD)(CRedWine::StrToDouble(strColumn) * ZoomScaleNum);

		PARSE_D(strColumn)
		item.wMoneyUnit = FindMoneyUnit(strColumn);

		PARSE_D(strColumn)
		item.qTime = StrToUl(strColumn);

		if (0 == iSerialCount)
		{
			while(Serial < TotalCount)
			{
				pMark = FindAuctionResSearchMemMark(Serial);
				if (NULL == pMark)
				{
					return false;
				}
				iSerialCount = pMark->nDayPriceMemCount;

				// Head
				ZXCMDHEAD DataHead;
				DataHead.m_wCmdType = ZXCMD_REDWINE_AUCTION_SINGLE_TYPE_DATA;
				DataHead.m_nAttr = Serial == (TotalCount - 1) ? 0 : TotalCount;
				SetExHByteSerialToProperty(Serial++, DataHead.m_nExData);
				DataHead.m_nLen = iSerialCount * iTypeSize;
				if (!bufWrite->Push_back(&DataHead, sizeof(ZXCMDHEAD)))
				{
					return false;
				}

				if (0 == iSerialCount)	// 插入空包
				{
					continue;
				}
				else if (iSerialCount > 0)
				{
					if (*pMark == tempItem)
					{
						bFindCurrentSeriaData = true;
					}
					else 
					{
						bFindCurrentSeriaData = false;
					}
					break;
				}
			}
		}
		else if (iSerialCount > 0 && NULL == pMark)	// 接前包
		{
			pMark = FindAuctionResSearchMemMark(Serial - 1);
			if (NULL == pMark)
			{
				return false;
			}
		}

		if (!bFindCurrentSeriaData)
		{
			if (*pMark == tempItem)
			{
				bFindCurrentSeriaData = true;
			}
			else 
			{
				bFindCurrentSeriaData = false;
				continue;
			}			
		}
		printf("[%d, %s ]Serial = %d \n", __LINE__, __FUNCTION__, Serial);

		// item
		if (!bufWrite->Push_back(&item, iTypeSize))
		{
			return false;
		}
		iSerialCount--;
		
	}
	
	return true;
}

// 当请求的数据不存在的时候，插入空包头
bool 
CRedWine::AddEmptyHeadData(const int TotalCount, const int Serial, const int HeadCmdType, CWriteBuff *bufWrite)
{
	// Head
	ZXCMDHEAD DataHead;
	DataHead.m_wCmdType = HeadCmdType;
	DataHead.m_nAttr = Serial == (TotalCount - 1) ? 0 : TotalCount;
	SetExHByteSerialToProperty(Serial, DataHead.m_nExData);
	DataHead.m_nLen = 0;
	if (!bufWrite->Push_back(&DataHead, sizeof(ZXCMDHEAD)))
	{
		return false;
	}
	return true;
}

// 注册取数据函数
void 
CRedWine::RegisterRequestFun()
{
	sRequestFunVt.clear();
	// 获取指数列表
	sRequestFunVt.push_back(CRedWine::RequestWineIndexList_S);
	
	// 获取酒类指数历史数据
	sRequestFunVt.push_back(CRedWine::GetWineIndexData_S);
	
	// 请求酒评家数据
	sRequestFunVt.push_back(CRedWine::RequestRedWineCritic_S);

	// 请求产区分类
	sRequestFunVt.push_back(CRedWine::RequestWineAreaClassified_S);
	
	// 请求大产区数据
	sRequestFunVt.push_back(CRedWine::RequestRedWineParentArea_S);
	
	// 请求子产区数据
	sRequestFunVt.push_back(CRedWine::RequestRedWineChildArea_S);
	
	// 八大酒庄数据
	sRequestFunVt.push_back(CRedWine::GetEightWineriesData_S);

	// 销售区域数据
	sRequestFunVt.push_back(CRedWine::RequestSaleArea_S);

	// 货币单位数据
	sRequestFunVt.push_back(CRedWine::ReqeustMoneyUnit_S);

	// 销售地点数据
	sRequestFunVt.push_back(CRedWine::RequestSaleAddress_S);

	// 拍卖行数据
	sRequestFunVt.push_back(CRedWine::RequestAuctionHouse_S);

	// 帕克分数
	sRequestFunVt.push_back(CRedWine::RequestRedwineParkRating);

	// 红酒名称列表数据
	sRequestFunVt.push_back(CRedWine::RequestRedwineNameList_S);

	// 红酒容量列表数据
	sRequestFunVt.push_back(CRedWine::RequestRedwineCapacityList_S);

	// 红酒基本信息
	sRequestFunVt.push_back(CRedWine::RequestRedwineBaseInfor_S);

	// 红酒三个市场酒名称索引列表
	sRequestFunVt.push_back(CRedWine::RequestClassifiedNameListIndex_S);

	// Liv市场成分酒数据
	sRequestFunVt.push_back(CRedWine::RequestLivMemberList_S);

	// Liv市场成员的日周期历史数据数目 
	sRequestFunVt.push_back(CRedWine::RequestLivMemberDayPriceElement);

	// Liv市场成员的月周期历史数据数目  
	sRequestFunVt.push_back(CRedWine::RequestLivMemberMonthPriceElement);

	// 拍卖市场成分酒数据
	sRequestFunVt.push_back(CRedWine::RequestAutionMemberList_S);

	// 拍卖市场成员的日周期历史数据数目 
	sRequestFunVt.push_back(CRedWine::RequestAuctionMemberDayPriceElement);

	// 拍卖市场成员的月周期历史数据数目  
	sRequestFunVt.push_back(CRedWine::RequestAuctionMemberMonthPriceElement);

	// 零售市场成分酒数据
	sRequestFunVt.push_back(CRedWine::RequestRetailMemberList_S);

	// 零售最新报价数据
	sRequestFunVt.push_back(RequestRetailLatestPrice_S);

	// Liv最新列表
	sRequestFunVt.push_back(CRedWine::RequestLivLatestPriceList_S);

	// 请求全部指数成分股索引数据
	sRequestFunVt.push_back(CRedWine::RequestAllIndexMemPosList_S);

	// 请求全部Liv价格周期为日的数据
	sRequestFunVt.push_back(CRedWine::RequestLivexDayPrice_S);

	// 请求拍卖价格周期为日的数据
	sRequestFunVt.push_back(CRedWine::RequestWineAuctionDayPrice_S);
	
	// 请求全部月统计数据
	sRequestFunVt.push_back(CRedWine::RequestMonthPrice_S);

	// 请求拍卖会日程数据
	sRequestFunVt.push_back(CRedWine::RequestAuctionSchedule_S);

	// 请求拍卖结果查询酒标识
	sRequestFunVt.push_back(CRedWine::RequestAuctionResSearchMemEnum_S);

	// 请求拍卖结果关键字
	sRequestFunVt.push_back(CRedWine::RequestAuctionResSearchKey_S);

	// 请求拍卖结果原始数据
	sRequestFunVt.push_back(CRedWine::RequestAuctionResSearchData_S);

	// 拍卖结果按照红酒种类统计的数据
	sRequestFunVt.push_back(CRedWine::RequestAuctionResSingleTypeData_S);
}

// 执行取数据函数
void
CRedWine::DoRequestStaticFunList()
{
	size_t iCount = sRequestFunVt.size();
	if (iCount < 1)
	{
		return;
	}

	// 计算取数据的起始位置
	size_t iStart = 0;
	if (m_iLastestErrorCode < 0 && EmptyPackMark != m_iLastestErrorCode)
	{
		iStart = -1 * (m_iLastestErrorCode + 1);
	}

	bool bRes = true;
	for (size_t i = iStart; i < iCount; i++)
	{
		if (NULL != sRequestFunVt[i])
		{
			bRes = (*sRequestFunVt[i])();
			if (!bRes)
			{
				m_iLastestErrorCode = -1 * (i + 1);
				return;
			}
		}
	}
	m_iLastestErrorCode = 0;
}

// 获取全部静态数据
bool 
CRedWine::RequestAllRemotetData()
{
	DoRequestStaticFunList();
	return HaveGotTotalData();
}

// 一次请求数据处理
const int 
CRedWine::ProcGetTotalTypeData(const std::string &DataType, const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const
{
	// 数据标示位判断
	if (!HaveGotTotalData())
	{
		return -1;
	}

	if (NULL == head)
	{
		return -2;
	}

	// 获取指定类型的数据
	const ResponseDataBuf *pBaseRespData = GetTypeWineData(DataType);
	if (NULL == pBaseRespData)
	{
		return -3;
	}

	*DataBuf = (char*)pBaseRespData->DataBuf;
	DataLength = pBaseRespData->GetDataLength();

	return 0;
}

// 分次请求数据处理
const int 
CRedWine::ProcGetSerialTypeData(const std::string &DataType, const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const
{
	// 数据标示位判断
	if (!HaveGotTotalData())
	{
		return -1;
	}

	// 获取指定类型的数据
	const ResponseDataBuf *pBaseRespData = GetTypeWineData(DataType);
	if (NULL == pBaseRespData)
	{
		return -2;
	}


	// 数据量
	size_t iDataCount = pBaseRespData->SegCount;
	if (iDataCount < 1)
	{
		return -3;
	}

	// 初始条件判断
	if (NULL == head
		|| head->m_nExData < 0 || head->m_nExData >= iDataCount
		|| head->m_nAttr <= 0)
	{
		return -4;
	}

	// 计算取数据数量
	size_t iRecordCount = (head->m_nExData + head->m_nAttr) > iDataCount ? (iDataCount - head->m_nExData) : head->m_nAttr;
	if (iRecordCount < 1)
	{
		return -5;
	}

	*DataBuf = (char*)(pBaseRespData->GetIndexHead(head->m_nExData));
	DataLength = pBaseRespData->GetRangeLength(head->m_nExData, head->m_nExData + iRecordCount - 1);
	
	return 0;
}

// 请求红酒指数列表
const int 
CRedWine::ProcWineIndexList(const ZXCMDHEAD *head, char **DataBuf, int &DataLength) const
{
	return ProcGetTotalTypeData("WineIndexList", head, DataBuf, DataLength);
}

// 请求指数历史数据
const int 
CRedWine::ProcWineIndexHistory(const ZXCMDHEAD *head, char **DataBuf, int &DataLength) const
{
	return ProcGetSerialTypeData("WineIndexHistory", head, DataBuf, DataLength);	
}

// 请求红酒酒评家及机构数据
const int 
CRedWine::ProcWineCritic(const ZXCMDHEAD *head, char **DataBuf, int &DataLength) const
{
	return ProcGetTotalTypeData("RedWineCritic", head, DataBuf, DataLength);
}

// 请求红酒产区分类数据
const int 
CRedWine::ProcWineAreaClassified(const ZXCMDHEAD *head, char **DataBuf, int &DataLength) const
{
	return ProcGetTotalTypeData("RedWineAreaClassified", head, DataBuf, DataLength);
}

// 请求红酒产大产区数据
const int 
CRedWine::ProcWineParentArea(const ZXCMDHEAD *head, char **DataBuf, int &DataLength) const
{
	return ProcGetTotalTypeData("RedWineParentArea", head, DataBuf, DataLength);
}

// 请求红酒产子产区数据
const int 
CRedWine::ProcWineChildArea(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const
{
	return ProcGetTotalTypeData("RedWineChildArea", head, DataBuf, DataLength);
}

// 请求红酒八大酒庄数据
const int 
CRedWine::ProcWineEightWineries(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const
{	
	return ProcGetTotalTypeData("EightWineries", head, DataBuf, DataLength);
}

// 请求红酒销售区域
const int 
CRedWine::ProcWineSaleArea(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const
{
	return ProcGetTotalTypeData("RedWineSaleArea", head, DataBuf, DataLength);
}

// 请求红酒Liv30最新报价列表
const int 
CRedWine::ProcWineLiv30Lastest(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const
{
	return ProcGetTotalTypeData("LivLastestPriceList", head, DataBuf, DataLength);	
}

// 请求红酒指数成分股数据
const int  
CRedWine::ProcWineIndexMem(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const
{
	return ProcGetSerialTypeData("MemListIndex", head, DataBuf, DataLength);	
}

// 请求红酒Liv周期为天统计数据
const int  
CRedWine::ProcWineLivDayPrice(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const
{
	return ProcGetSerialTypeData("WineLivexDayPrice", head, DataBuf, DataLength);	
}

// 请求红酒Liv周期为月统计数据
const int  
CRedWine::ProcWineLivMonthPrice(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const
{
	return ProcGetSerialTypeData("LivMonthPriceHisData", head, DataBuf, DataLength);	
}

// 请求红酒拍卖日数据
const int  
CRedWine::ProcWineAuctionDay(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const
{
	return ProcGetSerialTypeData("WineAuctionDayPrice", head, DataBuf, DataLength);	
}

// 请求红酒拍卖月数据
const int  
CRedWine::ProcWineAuctionMonth(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const
{
	return ProcGetSerialTypeData("AuctionMonthPriceHisData", head, DataBuf, DataLength);	
}

// 请求红酒零售月数据
const int  
CRedWine::ProcWineRetailMonth(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const
{
	return ProcGetSerialTypeData("RetailMonthPriceHisData", head, DataBuf, DataLength);	
}

// 请求货币单位数据
const int 
CRedWine::ProcWineMoneyUnit(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const
{
	return ProcGetTotalTypeData("RedwineMoneyUnit", head, DataBuf, DataLength);
}

// 请求拍卖行数据
const int 
CRedWine::ProcWineAuctionHouse(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const
{
	return ProcGetTotalTypeData("RedWineAuctionHouse", head, DataBuf, DataLength);
}

// 请求拍卖会日程数据
const int 
CRedWine::ProcWineAuctionSchedule(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const
{
	return ProcGetTotalTypeData("RedWineAuctionSchedule", head, DataBuf, DataLength);
}

// 请求销售地点数据
const int 
CRedWine::ProcWineSaleAddress(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const
{
	return ProcGetTotalTypeData("RedWineSaleAddress", head, DataBuf, DataLength);
}

// 请求名称列表
const int 
CRedWine::ProcWineNameList(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const
{
	return ProcGetTotalTypeData("RedWineNameList", head, DataBuf, DataLength);
}

// 请求容量列表
const int
CRedWine::ProcWineCapacityList(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const
{
	return ProcGetTotalTypeData("RedWineCapacityList", head, DataBuf, DataLength);
}

// 请求红酒基本信息列表
const int 
CRedWine::ProcWineBaseInfoList(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const
{
	return ProcGetTotalTypeData("RedWineBaseInforList", head, DataBuf, DataLength);
}

// 请求红酒Liv市场成分酒列表
const int 
CRedWine::ProcWineLivMemList(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const
{
	return ProcGetTotalTypeData("RedWineLivMemList", head, DataBuf, DataLength);
}

// 请求红酒拍卖市场成分酒列表
const int 
CRedWine::ProcWineAuctionMemList(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const
{
	return ProcGetTotalTypeData("RedWineAutionMemList", head, DataBuf, DataLength);
}

// 请求红酒零售市场成分酒列表
const int
CRedWine::ProcWineRetailMemList(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const
{
	return ProcGetTotalTypeData("RedWineRetailMemList", head, DataBuf, DataLength);
}

// 请求零售最新报价
const int 
CRedWine::ProcWineRetailLatestPrice(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const
{
	return ProcGetTotalTypeData("RedwineRetailLatestPrice", head, DataBuf, DataLength);
}

// 请求三个市场酒名称索引列表
const int 
CRedWine::ProcWineClassifiedNameListIndex(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const
{
	return ProcGetSerialTypeData("RedWineClassifiedNameListIndex", head, DataBuf, DataLength);
}

// 请求红酒拍卖结果查询结果数据酒标识列表
const int 
CRedWine::ProcWineAuctionResSearchMemEnum(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const
{
	return ProcGetTotalTypeData("AuctionResSearchMemEnum", head, DataBuf, DataLength);
}

// 请求红酒拍卖结果查询关键字
const int 
CRedWine::ProcWineAuctionResSearchKey(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const
{
	return ProcGetTotalTypeData("AuctionResSearchKeyList", head, DataBuf, DataLength);
}

// 获取拍卖结果全部数据
const int 
CRedWine::ProcWineAuctionResSearchData(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const
{
	return ProcGetSerialTypeData("AuctionResSearchDataList", head, DataBuf, DataLength);
}

// 获取按酒种类归类的拍卖结果全部数据
const int 
CRedWine::ProcWineAuctionResByWineTypeData(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const
{
	return ProcGetSerialTypeData("AuctionResByWineTypeDataList", head, DataBuf, DataLength);
}

// 处理外部请求
const int 
CRedWine::ProcRedwineRequest(const ZXCMDHEAD *head, char **DataBuf, int &DataLength)const
{
	if (NULL == head)
	{
		return -1;
	}

	switch(head->m_wCmdType)
	{
	case ZXCMD_REDWINE_INDEX_LIST:
		return ProcWineIndexList(head, DataBuf, DataLength);

	case ZXCMD_REDWINE_INDEX_HIS_DATA:
		return ProcWineIndexHistory(head, DataBuf, DataLength);

	case ZXCMD_REDWINE_CRITIC_LIST:
		return ProcWineCritic(head, DataBuf, DataLength);

	case ZXCMD_REDWINE_AREACLASSIFIED_LIST:
		return ProcWineAreaClassified(head, DataBuf, DataLength);
	
	case ZXCMD_REDWINE_PARENTAREA_LIST:
		return ProcWineParentArea(head, DataBuf, DataLength);

	case ZXCMD_REDWINE_CHILDAREA_LIST:
		return ProcWineChildArea(head, DataBuf, DataLength);

	case ZXCMD_REDWINE_EIGHTWINERIES_LIST:
		return ProcWineEightWineries(head, DataBuf, DataLength);

	case ZXCMD_REDWINE_SALEAREA:
		return ProcWineSaleArea(head, DataBuf, DataLength);

	case ZXCMD_REDWINE_LIV30_LIST:
		return ProcWineLiv30Lastest(head, DataBuf, DataLength);

	case ZXCMD_REDWINE_INDEX_ELEMENT:
		return ProcWineIndexMem(head, DataBuf, DataLength);

	case ZXCMD_REDWINE_LIV_DAY_DATA:
		return ProcWineLivDayPrice(head, DataBuf, DataLength);

	case ZXCMD_REDWINE_LIV_MONTH_DATA:
		return ProcWineLivMonthPrice(head, DataBuf, DataLength);

	case ZXCMD_REDWINE_AUCTION_DAY_DATA:
		return ProcWineAuctionDay(head, DataBuf, DataLength);

	case ZXCMD_REDWINE_AUCTION_MONTH_DATA:
		return ProcWineAuctionMonth(head, DataBuf, DataLength);

	case ZXCMD_REDWINE_RETAIL_MONTH_DATA:
		return ProcWineRetailMonth(head, DataBuf, DataLength);

	case ZXCMD_REDWINE_MONEY_UNIT_DATA:
		return ProcWineMoneyUnit(head, DataBuf, DataLength);

	case ZXCMD_REDWINE_SALE_ADDRESS_DATA:
		return ProcWineSaleAddress(head, DataBuf, DataLength);

	case ZXCMD_REDWINE_AUCTION_HOUSE_DATA:
		return ProcWineAuctionHouse(head, DataBuf, DataLength);

	case ZXCMD_REDWINE_AUCTION_SCHEDULE_DATA:
		return ProcWineAuctionSchedule(head, DataBuf, DataLength);

	case ZXCMD_REDWINE_NAME_LIST_DATA:
		return ProcWineNameList(head, DataBuf, DataLength);

	case ZXCMD_REDWINE_CAPACITY_LIST_DATA:
		return ProcWineCapacityList(head, DataBuf, DataLength);

	case ZXCMD_REDWINE_BASE_INFOR_DATA:
		return ProcWineBaseInfoList(head, DataBuf, DataLength);

	case ZXCMD_REDWINE_LIV_MEMBER_DATA:
		return ProcWineLivMemList(head, DataBuf, DataLength);

	case ZXCMD_REDWINE_AUCTION_MEMBER_DATA:
		return ProcWineAuctionMemList(head, DataBuf, DataLength);

	case ZXCMD_REDWINE_RETAIL_MEMBER_DATA:
		return ProcWineRetailMemList(head, DataBuf, DataLength);

	case ZXCMD_REDWINE_CLASSIFIED_NAME_LIST:
		return ProcWineClassifiedNameListIndex(head, DataBuf, DataLength);

	case ZXCMD_REDWINE_AUCTION_SEARCH_KEY_DATA:	
		return ProcWineAuctionResSearchKey(head, DataBuf, DataLength);

	case ZXCMD_REDWINE_AUCTION_SEARCH_MEM_DATA:	
		return ProcWineAuctionResSearchMemEnum(head, DataBuf, DataLength);

	case ZXCMD_REDWINE_AUCTION_SEARCH_HIS_DATA:
		return ProcWineAuctionResSearchData(head, DataBuf, DataLength);

	case ZXCMD_REDWINE_RETAIL_LATEST_PRICE:
		return ProcWineRetailLatestPrice(head, DataBuf, DataLength);

	case ZXCMD_REDWINE_AUCTION_SINGLE_TYPE_DATA:
		return ProcWineAuctionResByWineTypeData(head, DataBuf, DataLength);

	default:
		return -2;
	}

	return -3;
}

