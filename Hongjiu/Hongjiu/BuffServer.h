#ifndef _INCLUDE_BUFFER_SERVER_H
#define _INCLUDE_BUFFER_SERVER_H

// �󻺳���
class LargeMemoryCache
{
private:
	// ��ʼ�ڴ��С
	const int mInitialLen;
	// �ڴ�����
	char* mDataBuf;
	// ��ǰ����λ��
	int mCurrentPos;
	// ����һ�εľ�����Ϣ
	int m_iWarning;

private:
	bool Initial();

public:
	LargeMemoryCache(const int InitialLen);
	virtual ~LargeMemoryCache();

	void * GetRawMemPointer(const int StartPos);
	void ClearAll();
	// ���һ���ڴ�
	void ClearRange(const int startPos, const int length);
	// �ƶ�һ���ڴ�����ݵ���ʼλ��
	bool MoveRangeMemToStart(const size_t SrcStartPos, const size_t length);
	// ��ȡ�̶���С�Ļ�����
	char* GetAllocMem(const int length);
	// ��ȡȫ��ʣ�໺����
	char* GetRemainMem(int &RemainLength);
	// ����ƫ����
	void SetRemainMemStart(const int offset);

	// ��ȡ���ݳ���
	const int GetUsedMemoryLen()const
	{
		return mCurrentPos;
	}

	// ���ʣ������ݲ���
	void ClearRemainMem()const;

	const int GetLatestWarning()const
	{
		return m_iWarning;
	}

};

// ���ݶ�ȡ
class CBuffReader 
{
private:
	// ���ݻ���
	void *m_pBuf;
	// �����ʼλ��
	size_t m_iStartPos;
	// ����ȫ������ռ�
	size_t m_iTotalDataBufSize;
	// ���ݵ�ǰλ��
	size_t m_iCurrentPos;
	// �������ʱ��λ��
	size_t m_iEndPos;

public:
	CBuffReader()
	{
	}

	// ����
	void Initial(void *buf, const size_t BufSize, const size_t StartPos);

	// ��λ����ʼ
	void SeekToBegin()
	{
		m_iCurrentPos = m_iStartPos;
	}

	// ��λ��ĩβ
	void SeekToEnd()
	{
		m_iCurrentPos = m_iEndPos;
	}

	// ��ȡ��ǰλ��
	const size_t GetCurrentPos()const
	{
		return m_iCurrentPos;
	}

	// �Ƿ�ĩβ
	bool IsEnd()const
	{
		return m_iCurrentPos == m_iEndPos;
	}

	// ��ȡָ������
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