#ifndef _INCLUDE_BUFFER_SERVER_H
#define _INCLUDE_BUFFER_SERVER_H

// 大缓冲区
class LargeMemoryCache
{
private:
	// 初始内存大小
	const int mInitialLen;
	// 内存区域
	char* mDataBuf;
	// 当前数据位置
	int mCurrentPos;
	// 最新一次的警告信息
	int m_iWarning;

private:
	bool Initial();

public:
	LargeMemoryCache(const int InitialLen);
	virtual ~LargeMemoryCache();

	void * GetRawMemPointer(const int StartPos);
	void ClearAll();
	// 清空一段内存
	void ClearRange(const int startPos, const int length);
	// 移动一段内存的内容到开始位置
	bool MoveRangeMemToStart(const size_t SrcStartPos, const size_t length);
	// 获取固定大小的缓冲区
	char* GetAllocMem(const int length);
	// 获取全部剩余缓冲区
	char* GetRemainMem(int &RemainLength);
	// 设置偏移量
	void SetRemainMemStart(const int offset);

	// 获取数据长度
	const int GetUsedMemoryLen()const
	{
		return mCurrentPos;
	}

	// 清空剩余的数据部分
	void ClearRemainMem()const;

	const int GetLatestWarning()const
	{
		return m_iWarning;
	}

};

// 数据读取
class CBuffReader 
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
	// 输出结束时的位置
	size_t m_iEndPos;

public:
	CBuffReader()
	{
	}

	// 重置
	void Initial(void *buf, const size_t BufSize, const size_t StartPos);

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

	// 获取当前位置
	const size_t GetCurrentPos()const
	{
		return m_iCurrentPos;
	}

	// 是否到末尾
	bool IsEnd()const
	{
		return m_iCurrentPos == m_iEndPos;
	}

	// 读取指定类型
	template<typename T> 
	T* ReadPoiner()
	{
		int iSizeLen = sizeof(T);
		T* OutRes = (T*)NULL;
		if (NULL == m_pBuf || m_iTotalDataBufSize <= 0 || IsEnd()
			|| (m_iCurrentPos + iSizeLen) > m_iTotalDataBufSize)
		{
			return OutRes;
		}

		OutRes = (T*)((char*)m_pBuf + m_iCurrentPos);
		if (NULL != OutRes) 
		{
			m_iCurrentPos += iSizeLen;
		}

		return OutRes;
	}
};


class CBuffServer
{
public:
	CBuffServer();
	~CBuffServer(void);
};

#endif		/* _INCLUDE_BUFFER_SERVER_H */