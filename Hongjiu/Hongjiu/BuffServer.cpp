#include "StdAfx.h"
#include "BuffServer.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

// 数据写入
class CBuffWriter
{
private:
	// 数据缓存
	void *m_pBuf;
	// 输出初始位置
	size_t m_iStartPos;
	// 缓存全部输出空间
	size_t m_iTotalDataBufSize;
	// 数据当前位置
	size_t m_iCurrentPos;
	// 新输出的数据的长度
	size_t m_iDataLength;
	// 输出结束时的位置
	size_t m_iEndPos;

public:
	CBuffWriter()
	{
	}

	void Initial(void *buf, const size_t BufSize, const size_t StartPos)
	{
		m_pBuf= buf;
		m_iStartPos = StartPos;
		m_iTotalDataBufSize = BufSize;
		m_iCurrentPos = m_iStartPos;
		m_iDataLength = 0;
		m_iEndPos = StartPos;
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



CBuffServer::CBuffServer()
{
	
}

CBuffServer::~CBuffServer(void)
{
	
}

LargeMemoryCache::LargeMemoryCache(const int InitialLen)
	: mInitialLen(InitialLen),
	mDataBuf(NULL),
	mCurrentPos(0),
	m_iWarning(0)
{
	Initial();
}

LargeMemoryCache::~LargeMemoryCache()
{
	if (NULL != mDataBuf)
	{
		delete []mDataBuf;
		mDataBuf = NULL;
	}
}

bool 
LargeMemoryCache::Initial()
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

void * 
LargeMemoryCache::GetRawMemPointer(const int StartPos)
{
	if (StartPos >= mInitialLen || StartPos <= (mInitialLen * -1))
	{
		return NULL;
	}
	return (mDataBuf + StartPos);
}

void 
LargeMemoryCache::ClearAll()
{
	if (NULL != mDataBuf)
	{
		memset(mDataBuf, 0, mInitialLen);
	}
	mCurrentPos = 0;
	m_iWarning = 0;
}

void 
LargeMemoryCache::ClearRange(const int startPos, const int length)
{
	if (NULL != mDataBuf)
	{
		if (startPos >= 0 && startPos < mInitialLen && (startPos + length) <= mInitialLen)
		{
			memset(mDataBuf + startPos, 0, length);
		}
	}
}

bool 
LargeMemoryCache::MoveRangeMemToStart(const size_t SrcStartPos, const size_t length)
{
	if (SrcStartPos >= mCurrentPos || length >= mCurrentPos)
	{
		return false;
	}
	memcpy(mDataBuf, mDataBuf + SrcStartPos, length);
	mCurrentPos = length;
	return true;
}

// 获取固定大小的缓冲区
char* 
LargeMemoryCache::GetAllocMem(const int length)
{
	char* outP = NULL;
	if (mCurrentPos >= 0 && mCurrentPos < mInitialLen && (mCurrentPos + length) <= mInitialLen)
	{
		outP = (char*)(mDataBuf + mCurrentPos);
	}
	mCurrentPos += length;
	if (mCurrentPos >= (mInitialLen - 256))
	{
		m_iWarning = 1;
	}
	return outP;
}

// 获取全部剩余缓冲区
char* 
LargeMemoryCache::GetRemainMem(int &RemainLength)
{
	char* outP = NULL;
	if (mCurrentPos >= 0 && mCurrentPos < mInitialLen)
	{
		outP = (char*)(mDataBuf + mCurrentPos);
	}

	RemainLength = mInitialLen - mCurrentPos;
	if (mCurrentPos >= (mInitialLen - 256))
	{
		m_iWarning = 1;
	}
	return outP;
}

// 设置偏移量
void 
LargeMemoryCache::SetRemainMemStart(const int offset)
{
	if (mCurrentPos >= 0 && mCurrentPos < mInitialLen && (mCurrentPos + offset) <= mInitialLen)
	{
		mCurrentPos += offset;
	}

	if (mCurrentPos >= (mInitialLen - 256))
	{
		m_iWarning = 1;
	}
}

// 清空剩余的数据部分
void 
LargeMemoryCache::ClearRemainMem()const
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


// 重置
void 
CBuffReader::Initial(void *buf, const size_t BufSize, const size_t StartPos)
{
	m_pBuf = buf;
	m_iStartPos = StartPos;
	m_iTotalDataBufSize = BufSize;
	m_iCurrentPos = m_iStartPos;
	m_iEndPos = BufSize - 1;
}