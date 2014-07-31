#ifndef AFX_STDAFX_H__90BF33DF_559A_4307_8B90_0F8E2DA02B36__INCLUDED_
#define AFX_STDAFX_H__90BF33DF_559A_4307_8B90_0F8E2DA02B36__INCLUDED_


#define SELFMANAGEMEMORY 1

#define MAXFILENAMELEN         512

#define NEWPOLLMODE 1
#define	LENSTLABEL		10
#define	LENSTNAME		32

//////////////////////

#define MARKET_BI	0x4942		// 'IB'

#ifndef WM_USER
#define  WM_USER       (1000)
#endif

#define  WM_ASKMESSAGE       (WM_USER+123)
#define  WM_MESSAGERESPONSE      (WM_USER+124)


#define WORKCLASS			204
///////////////
#define  WM_WTRESPONSEMESSAGE       (WM_USER+225)//ί����Ӧ��WPARAM������ʱ��Ĳ���WTASKINFO��LAPARAM�Ǵ�ί�з��������صĲ��������ΪNULL��ʾί��ʧ��
#define  WM_WTRESPONSEMESSAGE_NETERROR       (WM_USER+226)//�������
//#define  WM_WTRESPONSEMESSAGE_TIMEOUT       (WM_USER+227)//��ʱ

extern 	char*  MapMemFile (const char* szFileName, DWORD len);

#pragma pack(1)

enum
{
	CLIENTDATABUFLEN = 1024,
	MAXMULDATABUFFNUM = 32,
	MAXSENDINFONUM = MAXMULDATABUFFNUM/3,	//һ����෢�;��û��߹�Ʊ��Ϣ��
};


//64λ����
union MAKELONGLONGST
{
	unsigned long long	m_llValue;
	struct
	{
		unsigned int	m_dwLow;
		unsigned int	m_dwHigh;
	};
};

//32λ����
union MAKEDWORDST
{
	unsigned int	m_dwValue;
	struct
	{
		unsigned short	m_wLow;
		unsigned short	m_wHigh;
	};
};

//16λ����
union MAKEWORDST
{
	unsigned short	m_wValue;
	struct
	{
		unsigned char	m_cLow;
		unsigned char	m_cHigh;
	};
};

union  USERNUMANDTHREAD
{
	struct
	{
		WORD     m_nThreadIndex;
		WORD     m_nClientNum;
	};
	DWORD    m_dwValue;
}; 

struct SELFMEMORYBLOCK
{
	char*  m_pBlockBuff;//�ڴ滺����
	DWORD  m_nFreePos;//���������е�λ��
	DWORD  m_nFreeLen;//���еĳ���, m_nFreePos+m_nFreeLen = �û��������ܳ���
};

#define	SELFMEMORYBOLCKCELL	2048

#ifdef WRITEVOP

#define BUFFANDLEN	struct iovec

#else

struct BUFFANDLEN
{
	void*	iov_base;
	size_t	iov_len;
};

#endif

struct CLIENTDATA
{
	WORD	m_nRecvLen;//��ʾ���յ����ݳ��Ȼ�����δ�������������
	WORD	m_nSendBufLen;//���ͻ������ĳ���
	char    m_pRecvBuff[CLIENTDATABUFLEN];
	time_t  m_tAliveTime;

	//BYTE    m_nAskTypeState;//������¼�ͻ���һЩ��Ǻ�״̬
	BYTE    m_nDataBuffNum;//��ʾ�������ͻ�����һ���ж��ٸ����ݣ�����رվͰ������0
	BYTE    m_nDataBuffPos;//��ʾ�´η��͵�ʱ����Ǹ���������ʼ��������

	BYTE    m_nSaveBufNum;//���д洢��Ҫɾ���Ļ�������Ŀ
	const char*	m_pSaveBuff[MAXMULDATABUFFNUM];
	BUFFANDLEN	m_pMulSendBuff[MAXMULDATABUFFNUM];
};

//////////////////////////////////////////////////////////////
//��Ѷ�ӿڵĶ���

//����
enum
{
	ROLLTXTMAXLEN = 1000,
	ROLLMAXNUM	=	10,
	MULROLLTXTLEN	=	64000,
	PREPICTUREMAXLEN	=	64000,
	PICTUREMAXLEN	=	640000,
	MAXECONOMICLEN = 8192,
	MAINWMLINFOMAXLEN	=	32000,
	MAXSTKINFOLENGTH = 32000,
	FACENOTICEMAXLEN = 16000,

	MAXFILEBUFFLEN = 10000000,	//��󻺳����ĳ���

	MULROLLCONTENTLEN = 300,

	BROADCASTMAXLEN = 1002,
	MAXROLLWMLLEN = 8192,

	MAXCRMPOPUPLEN = 2048,	//crm������Ѷ��󳤶�

	//ͼƬ����
	PICTURETYPE_PNG = 1,
	PICTURETYPE_JPG = 2,
	//
	PREMAINXWLINFONUM = 12,
	MAINXWLINFONUM = 13,
	BULLETINDATANUM = 2,
	FACENOTICENUM = 2,	//������ʾ��������Ŀ.
	TRANSWMLNUM = 2,
};

//����ͷ
struct	ZXCMDHEAD
{
	WORD	m_wCmdType;	//��������
	DWORD	m_nAttr;	//�������ԣ�
	DWORD	m_nLen;		//�������ݳ���
	DWORD	m_nExData;	//�������ݣ���Ӧ��ͨ��ԭ�����أ�������;����
};
//��������
enum
{
	ZXCMDXC_ALIVE	=	0,
	ZXCMDXC_JIEPAN	=	1,

	ZXCMD_ALIVE	=	0x8080,
	ZXCMD_ECONOMIC,
	ZXCMD_MULROLLTXT,	//�������Ѷ
	ZXCMD_MAINXWL,		//��ҳ
	ZXCMD_RMBRATE,		//����һ���
	ZXCMD_ROLLTXT,		//�������
	ZXCMD_PICTURE,		//���ͼƬ
	ZXCMD_STKINFO,		//��Ʊ�����Ϣ������Ҳ����ʵʱ����
	ZXCMD_FILE,			//�����ļ�
	ZXCMD_BROADCAST,	//���乫��

	ZXCMD_ROLLWML,		//����wml��Ϣ
	ZXCMD_FACENOTICE,	//������ʾ��Ϣ��= 11 b

	ZXCMD_STKPOOLCFG,	//��Ʊ��������Ϣ
	ZXCMD_STKPOOLSTK,	//��Ʊ����Ϣ

	ZXCMD_TRANSFERWML,	//��ת��wml
	//////
	ZXCMD_CRMPOPUP,		//ȯ�̵�����Ѷ
	ZXCMD_MULPICTURE,	//����ֱ��ʹ��ͼƬ

	ZXCMD_WTADDRINFO,	//����ί����Ϣ

	////////��ƽӿ�����
	ZXCMD_REDWINE_INDEX_LIST,				// ������ָ���б� һ�����󣬷��ؽṹΪ RedwineIndexListTag
	ZXCMD_REDWINE_INDEX_HIS_DATA,			// ������ָ����ʷ����, �ִ����󣬷��ؽṹΪ RedwineIndexHistTag
	ZXCMD_REDWINE_INDEX_ELEMENT,			// ������ָ���ɷֹ�����, �ִ����󣬷��� sizeof(WORD)*N

	ZXCMD_REDWINE_CRITIC_LIST,				// �����ƾ����Ҽ��������ݣ�һ�����󣬷��� RedWineCriticTag
	ZXCMD_REDWINE_NAME_LIST_DATA,			// �����������б�һ�����󣬷��� RedwineNameTag
	ZXCMD_REDWINE_AREACLASSIFIED_LIST,		// �����Ʋ�����������, һ�����󣬷��� RedWineAreaClassifiedTag
	ZXCMD_REDWINE_PARENTAREA_LIST,			// �����Ʋ���������ݣ�һ�����󣬷��� RedWineParentAreaTag
	ZXCMD_REDWINE_CHILDAREA_LIST,			// �����Ʋ��Ӳ������ݣ�һ�����󣬷��� RedWineChildAreaTag
	ZXCMD_REDWINE_EIGHTWINERIES_LIST,		// �����ư˴��ׯ���ݣ�һ�δ����󣬷��� RedwineManorListTag
	ZXCMD_REDWINE_SALEAREA,					// ��������������һ�����󣬷��� RedWineSaleAreaTag
	ZXCMD_REDWINE_CAPACITY_LIST_DATA,		// �����������б�һ�����󣬷��� RedwineCapacityTag
	ZXCMD_REDWINE_MONEY_UNIT_DATA,			// �����ƻ��ҵ�λ���ݣ�һ�����󣬷��� RedwineMoneyUnitTag

	ZXCMD_REDWINE_BASE_INFOR_DATA,			// ����ȫ����ƻ�����Ϣ��һ�����󣬷��� RedwineBaseInforTag
	ZXCMD_REDWINE_CLASSIFIED_NAME_LIST,		// �����Ʒ����������Ϣ����(m_nExData��ʶ֤ȯ�����������Ա�ʶ���Ʒ�������0:Liv 1:���� 2:����)���ִ����󣬷���sizeof(WORD)*N

	ZXCMD_REDWINE_LIV_MEMBER_DATA,			// ����ȫ�����Liv�г��ɷ־ƣ�һ�����󣬷��� RedwineLivMemEnumTag
	ZXCMD_REDWINE_LIV30_LIST,				// ������Liv30���±����б�, һ�����󣬷��ؽṹΪ RedwineLatestLivListTag
 	ZXCMD_REDWINE_LIV_DAY_DATA,				// ������Liv����Ϊ��ͳ�����ݣ��ִ����󣬷��� RedwinePriceLivDayHistDataTag
 	ZXCMD_REDWINE_LIV_MONTH_DATA,			// ������Liv����Ϊ��ͳ�����ݣ��ִ����󣬷��� RedwinePriceLivMonthHistDataTag

	ZXCMD_REDWINE_SALE_ADDRESS_DATA,		// ���������۵ص����ݣ�һ�����󣬷��� RedwineSaleAddressTag

	ZXCMD_REDWINE_AUCTION_MEMBER_DATA,		// ����ȫ����������г��ɷ־ƣ�һ�����󣬷��� RedwineAuctionMemEnumTag
	ZXCMD_REDWINE_AUCTION_DAY_DATA,			// ����������������, �ִ����󣬷��� RedwinePriceAuctionDayHistTag
	ZXCMD_REDWINE_AUCTION_MONTH_DATA,		// ����������������, �ִ����󣬷��� RedwinePriceHistDataTag

	ZXCMD_REDWINE_AUCTION_HOUSE_DATA,		// ���������������ݣ�һ�����󣬷��� RedwineAuctionHouseTag
	ZXCMD_REDWINE_AUCTION_SCHEDULE_DATA,	// �������������ճ̣�һ�����󣬷��� RedwineAuctionScheduleTag
		
	ZXCMD_REDWINE_RETAIL_MEMBER_DATA,		// ����ȫ����������г��ɷ־ƣ�һ�����󣬷��� RedwineRetailMemEnumTag
	ZXCMD_REDWINE_RETAIL_MONTH_DATA,		// ����������������, �ִ����󣬷��� RedwinePriceHistDataTag
	ZXCMD_REDWINE_RETAIL_LATEST_PRICE,		// ��������������������ݣ�һ�����󣬷��� RedwineRetailLatestPriceTag

	ZXCMD_REDWINE_AUCTION_SEARCH_KEY_DATA,	// ���������������ѯ�ؼ���(�ص�ṹ)���, һ������, ���� RedwineAuctionResSearchKeyTag
	ZXCMD_REDWINE_AUCTION_SEARCH_MEM_DATA,	// ���������������ѯ������ݾƱ�ʶ�б�, һ������, ���� RedwineAuctionResSearchMemEnumTag
	ZXCMD_REDWINE_AUCTION_SEARCH_HIS_DATA,	// ���������������ѯ��ʷ����(���ص�ṹ), �ִ�����(˳����Ӧ��RedwineAuctionResSearchKeyTag�б�), ���� RedwineAuctionResSearchDataTag
	ZXCMD_REDWINE_AUCTION_SINGLE_TYPE_DATA,	// ���������������ʷ����(�����������), �ִ�����(˳����Ӧ��RedwineAuctionResSearchMemEnumTag�б�)������ RedwineAuctBySingleTypeDataTag
};

//ZXCMDXC_ALIVE��m_nLen=0���ͻ��˷��ͣ�������ԭ�����أ�������ʾ�����Ƿ���ڣ�10���ӿͻ���Ҫ��һ��
//ZXCMDXC_JIEPAN����Ʊ��Ѷ������m_nAttr=0,m_nExData=id�����Ϊ0�����أ�0��N������Ϣ���أ�m_nAttr=0��m_nExData=������Ŀǰʵʱ����id���´������ʱ��������յ��ĸ�id����


struct STKZXINFO //��Ʊ��Ѷ��Ϣ�ṹ
{
	char	m_type;	//����
	time_t	m_time;	//����ʱ��
	char	m_szStkLabel[10];
	WORD	m_nContentLen;
	char	m_pContent[0];
};

union	JIEPANEXDATA
{
	struct	
	{
		unsigned int	m_nNum:23;
		unsigned int	m_day:5;
		unsigned int	m_mon:4;
	};
	unsigned int	m_value;
};


//ZXCMD_ALIVE��m_nLen=0���ͻ��˷��ͣ�������ԭ�����أ�������ʾ�����Ƿ���ڣ�10���ӿͻ���Ҫ��һ��

//ZXCMD_ECONOMIC������m_nAttr=0,m_nExData=id(����Ŀ��ʾ)�����أ�m_nAttr=��ʾ���滹Ҫ���ٸ��������ݣ�0��ʾ�����ʹ��������m_nExData=������Ŀǰid
struct	ECONOMICZX
{
	char	m_type;	//����
	time_t	m_time;	//����ʱ��
	WORD	m_nTitleLen;
	WORD	m_nContentLen;
	char	m_pTitle[0];
	//char	content;//����������
};

//ZXCMD_MULROLLTXT��������ı���m_nAttr��ʾ�������߷Ǵ�����m_nExData��ʾ�����ı���crc��ZXCMDHEAD+char*n

//ZXCMD_MAINXWL����ҳ��m_nAttr��ʾ��ҳ���壻m_nExData��ʾ��ҳ��crc��ZXCMDHEAD+char*n

//ZXCMD_RMBRATE������һ��ʣ�m_nExData��ʾcrc��ZXCMDHEAD+RMBRATEINFO*n

struct RMBRATEINFO
{
	DWORD m_nDate;
	DWORD m_nRate;
	BYTE  m_nPriceDigit;
	char  m_szName[LENSTNAME];
	char  m_szUtf8Name[sizeof(WORD)+LENSTNAME];
};

//ZXCMD_ROLLTXT��������棻���أ�m_nAttr=��ʾ���滹Ҫ���ٸ��������ݣ�0��ʾ�����ʹ��������m_nExData��ʾ���µĹ������������ZXCMDHEAD+id+char*n

//ZXCMD_PICTURE�����ͼƬ��m_nExData��ʾ���µĹ��ͼƬid��ZXCMDHEAD+char*n

//ZXCMD_STKINFO����Ʊ��Ѷ������m_nAttr=0,m_nExData=id(����Ŀ��ʾ)�����أ�m_nAttr=���Ҫ���͵�STKZXINFO������Ŀ��m_nExData=������Ŀǰid

//ZXCMD_ROLLWML���������wml��m_nExData�Ѿ���õ�wml��crc��ZXCMDHEAD+char*n

//ZXCMD_FACENOTICE�����������ʾ���֣�m_nExData�Ѿ���õ���ʾ���ֵ�crc��ZXCMDHEAD+char*n

//ZXCMD_STKPOOLCFG�������Ʊ��������Ϣ��m_nExData�Ѿ���õĹ�Ʊ��������Ϣ��crc��ZXCMDHEAD+char*n

//ZXCMD_STKPOOLSTK�������Ʊ����Ϣ��m_nExData�Ѿ���õĹ�Ʊ��������Ϣ��crc��m_nAttr�ǹ�Ʊ��id��ZXCMDHEAD+char*n

//ZXCMD_TRANSFERWML, ��ת��wml��m_nAttr��ʾ�����id�� m_nExData�Ѿ���õ�wml��crc��ZXCMDHEAD+char*n

//ZXCMD_MULPICTURE, ����ͬ�ֱ��ʵ�ͼƬ��m_nAttr��ʾ�ֱ���,0��ʾ��������ͼƬ�Ļ�����Ϣ��m_nExData��ʾͼƬ����,m_nLen=0;����m_nLen=sizeof(MULPICTUREINFO)+char*N

//ZXCMD_WTADDRINFO, m_nAttr����,m_nExData��ʾcrc��m_nLen=0;����m_nAttr����,m_nExData>>16��ʾ��Ŀ��m_nExData&0xFFFF��ʾcrc��m_nLen=sizeof(WTADDRINFO)+char*N�����m_nLen=0��ʾû�����ݻ�����ͬ

//AH�ɶ��մ洢�ṹ�ṹ
struct AHSTOCKINFO
{
	char  szHCode[sizeof(WORD)+LENSTLABEL];//�����г�����
	char  szACode[sizeof(WORD)+LENSTLABEL];//�����г�����
};

//H�ɷ����ļ��ṹ
enum
{
	BLUESTK,	//�����
	REDSTK,		//����
	GQSTK,		//�����
	UNKNOWSTK,		//δ֪����
};

struct	HKSTKCLASS
{
	char	m_type;
	char	m_szLabel[LENSTLABEL];
};

//ZXCMD_FILE�������ļ�������m_nAttr��ʾ�ļ������� m_nExData��ʾ���ļ�crc�����أ�m_nAttr��2�ֽڱ�ʾ���������ļ�������λ2�ֽڱ�ʾ�ļ�������m_nExData��ʾ���ļ�crc��ZXFILE+char*N
struct ZXFILE
{
	char	m_szFileName[32];
	char	m_pContent[0];
};

/////////////

#define DATASOURCEIPLEN  31
struct DATASOURCESTATE
{
	DWORD           m_nPreHQPackNo; //����
	DWORD           m_nHQPackNo;    //����
	time_t          m_nRecvKeepALive; // �ڵȴ� KEEPRCVALIVEMINSEC
	time_t          m_nPreRecvDynaTime; // �ϴν��յ���̬���ݵ�ʱ��
	int             m_nIPIndex; //����ʹ�õ������ļ���ip���������ֻ��������������
	int             m_SocketState;//����Դ������״̬
	char            m_szDataSourceIp[DATASOURCEIPLEN+1];//����Դ��IP
	unsigned short  m_usDataPort;//����Դ��״̬
	unsigned short  m_nLevel;//
};


struct MULPICTUREINFO
{
	DWORD	m_len;			//ͼƬ����
	union
	{
		DWORD	m_nRes;			//��߷ֱ���
		struct
		{
			WORD	m_nHigthRes;	//�߷ֱ���
			WORD	m_nWidthRes;	//��ֱ���
		};
	};
	DWORD	m_nCRC;			//crc
	unsigned char	m_nPitType;		//ͼƬ����
	char	m_pData[0];
};

enum
{
	MULPICTURE_NONE = 0,
	MULPICTURE_ADD = 1,
	MULPICTURE_MODIFY = 2,
};

struct WTADDRINFO
{
	WORD	m_nLen;//�����m_nCrc��ʼ�ĳ���
	WORD	m_nCrc;
	WORD	m_nNum;
	char	m_pData[0];
};

struct	WTATTRVALUE
{
	unsigned char	m_nAttrNo;		//
	unsigned char	m_nAttrValue;		//
};

struct	WTADDRST
{
	std::string	m_strAddr;
	unsigned short	m_port;		//
	unsigned char	m_ipClass;		//
	unsigned char	m_addrType;		//
};


struct	SINGLEQSINFO
{
	std::string	m_strName;
	char		m_py;
	std::vector<WTATTRVALUE>	m_vAttr;
	std::vector<WTADDRST>		m_vAddr;
};
enum
{	
	REDWINE_INDEX_NAME_LEN = 32,						// ���ָ�����Ƴ���
	REDWINE_CRITIC_NAME_LEN = 56,						// ��ƾ��������Ƴ���
	REDWINE_AREACLASSIFIED_NAME_LEN = 32,				// ��Ʋ����������Ƴ���
	REDWINE_PARENTAREA_NAME_LEN = 32,					// ��ƴ�������Ƴ���
	REDWINE_CHILDAREA_NAME_LEN = 32,					// ����Ӳ������Ƴ���
	REDWINE_MANOR_NAME_LEN = 32,						// ��ƾ�ׯ���Ƴ���
	REDWINE_NAME_LEN = 56,								// ������Ƴ���
	REDWINE_CAPACITY_LEN = 16,							// �����������
	REDWINE_SALEAREA_LEN = 32,							// ����������򻮷ֳ���
	REDWINE_MONEYUNIT_LEN = 16,							// ��ƻ��ҵ�λ����
	REDWINE_SALE_ADDR_LEN = 24,							// ������۵ص㳤��
	REDWINE_AUCTION_HOUSE_LEN = 16,						// ��������г���
	REDWINE_AUCTION_NAME_LEN = 128,						// ������������Ƴ���
};

////////////////////////////////////////// ��ƽӿڽṹ���� //////////////////////////////
//	�ִ���������壺
//				ZXCMDHEAD����ͷ
//				����m_nAttr=��ʾ������Ŀ, m_nExData�������ʼλ��(��0��ʼ���)��m_nLen=0; 
//				���أ�m_nAttr��ʾ���������ܸ���,Ϊ0��ʾ���һ������;
//					  m_nExData���BYTE��ʾ���ݵĻ��ҵ�λ(һ�����ݴ���ͬһ���ҵ�λʱӦ�ã�����δ����)��m_nExData�����BYTE������BYTE��ʾ֤ȯ����;
//					  m_nLen=sizeof(���ؽṹ)*N;
//
// ZXCMD_REDWINE_INDEX_LIST,				// ������ָ���б� һ�����󣬷��ؽṹΪ RedwineIndexListTag
// ZXCMD_REDWINE_INDEX_HIS_DATA,			// ������ָ����ʷ����, �ִ����󣬷��ؽṹΪ RedwineIndexHistTag
// ZXCMD_REDWINE_INDEX_ELEMENT,				// ������ָ���ɷֹ�����, �ִ����󣬷��� sizeof(WORD)*N
// 
// ZXCMD_REDWINE_CRITIC_LIST,				// �����ƾ����Ҽ��������ݣ�һ�����󣬷��� RedWineCriticTag
// ZXCMD_REDWINE_NAME_LIST_DATA,			// �����������б�һ�����󣬷��� RedwineNameTag
// ZXCMD_REDWINE_AREACLASSIFIED_LIST,		// �����Ʋ�����������, һ�����󣬷��� RedWineAreaClassifiedTag
// ZXCMD_REDWINE_PARENTAREA_LIST,			// �����Ʋ���������ݣ�һ�����󣬷��� RedWineParentAreaTag
// ZXCMD_REDWINE_CHILDAREA_LIST,			// �����Ʋ��Ӳ������ݣ�һ�����󣬷��� RedWineChildAreaTag
// ZXCMD_REDWINE_EIGHTWINERIES_LIST,		// �����ư˴��ׯ���ݣ�һ�δ����󣬷��� RedwineManorListTag
// ZXCMD_REDWINE_SALEAREA,					// ��������������һ�����󣬷��� RedWineSaleAreaTag
// ZXCMD_REDWINE_CAPACITY_LIST_DATA,		// �����������б�һ�����󣬷��� RedwineCapacityTag
// ZXCMD_REDWINE_MONEY_UNIT_DATA,			// �����ƻ��ҵ�λ���ݣ�һ�����󣬷��� RedwineMoneyUnitTag
// 
// ZXCMD_REDWINE_BASE_INFOR_DATA,			// ����ȫ����ƻ�����Ϣ��һ�����󣬷��� RedwineBaseInforTag
// ZXCMD_REDWINE_CLASSIFIED_NAME_LIST,		// �����Ʒ����������Ϣ����(m_nExData��ʶ֤ȯ�����������Ա�ʶ���Ʒ�������0:Liv 1:���� 2:����)���ִ����󣬷���sizeof(WORD)*N
// 
// ZXCMD_REDWINE_LIV_MEMBER_DATA,			// ����ȫ�����Liv�г��ɷ־ƣ�һ�����󣬷��� RedwineLivMemEnumTag
// ZXCMD_REDWINE_LIV30_LIST,				// ������Liv30���±����б�, һ�����󣬷��ؽṹΪ RedwineLatestLivListTag
// ZXCMD_REDWINE_LIV_DAY_DATA,				// ������Liv����Ϊ��ͳ�����ݣ��ִ����󣬷��� RedwinePriceLivDayHistDataTag
// ZXCMD_REDWINE_LIV_MONTH_DATA,			// ������Liv����Ϊ��ͳ�����ݣ��ִ����󣬷��� RedwinePriceLivMonthHistDataTag
// 
// ZXCMD_REDWINE_SALE_ADDRESS_DATA,			// ���������۵ص����ݣ�һ�����󣬷��� RedwineSaleAddressTag
// 
// ZXCMD_REDWINE_AUCTION_MEMBER_DATA,		// ����ȫ����������г��ɷ־ƣ�һ�����󣬷��� RedwineAuctionMemEnumTag
// ZXCMD_REDWINE_AUCTION_DAY_DATA,			// ����������������, �ִ����󣬷��� RedwinePriceAuctionDayHistTag
// ZXCMD_REDWINE_AUCTION_MONTH_DATA,		// ����������������, �ִ����󣬷��� RedwinePriceHistDataTag
// 
// ZXCMD_REDWINE_AUCTION_HOUSE_DATA,		// ���������������ݣ�һ�����󣬷��� RedwineAuctionHouseTag
// ZXCMD_REDWINE_AUCTION_SCHEDULE_DATA,		// �������������ճ̣�һ�����󣬷��� RedwineAuctionScheduleTag
// 	
// ZXCMD_REDWINE_RETAIL_MEMBER_DATA,		// ����ȫ����������г��ɷ־ƣ�һ�����󣬷��� RedwineRetailMemEnumTag
// ZXCMD_REDWINE_RETAIL_MONTH_DATA,			// ����������������, �ִ����󣬷��� RedwinePriceHistDataTag
// ZXCMD_REDWINE_RETAIL_LATEST_PRICE,		// ��������������������ݣ�һ�����󣬷��� RedwineRetailLatestPriceTag
// 
// ZXCMD_REDWINE_AUCTION_SEARCH_KEY_DATA,	// ���������������ѯ�ؼ���(�ص�ṹ)���, һ������, ���� RedwineAuctionResSearchKeyTag
// ZXCMD_REDWINE_AUCTION_SEARCH_MEM_DATA,	// ���������������ѯ������ݾƱ�ʶ�б�, һ������, ���� RedwineAuctionResSearchMemEnumTag
// ZXCMD_REDWINE_AUCTION_SEARCH_HIS_DATA,	// ���������������ѯ��ʷ����, �ִ�����(˳����Ӧ��RedwineAuctionResSearchKeyTag�б�), ���� RedwineAuctionResSearchDataTag
// ZXCMD_REDWINE_AUCTION_SINGLE_TYPE_DATA,	// ���������������ʷ����(�����������), �ִ�����(˳����Ӧ��RedwineAuctionResSearchMemEnumTag�б�)������ RedwineAuctBySingleTypeDataTag
// 
/////////////////////////////////////////////////////////////////////////////////////////

//���ָ���б�
struct RedwineIndexListTag
{
	char	wIndexCode[REDWINE_INDEX_NAME_LEN];							// ���ָ�����ƴ���
	char	wIndexChnName[REDWINE_INDEX_NAME_LEN];						// ���ָ����������
};                                              		
                                                		
// ���ָ����ʷ����                             		
struct RedwineIndexHistTag
{                                               		
	DWORD	m_nDate;                            		
	DWORD	m_nValue; // *100
};                                              		
                                                		
// �����Ҽ���������                             		
struct RedWineCriticTag                         		
{                                               		
	char	wCriticName[REDWINE_CRITIC_NAME_LEN];						// ����������
};                                              		
                                                		
// ��Ʋ�����������                             		
struct RedWineAreaClassifiedTag                 		
{                                               		
	char	wClassifiedName[REDWINE_AREACLASSIFIED_NAME_LEN];			// ������������
};                                              		
                                                		
// ��ƴ��������                               		
struct RedWineParentAreaTag                     		
{                                               		
	char	wParentAreaName[REDWINE_PARENTAREA_NAME_LEN];				// ���������
};                                              		
                                                		
// ����Ӳ�������                               		
struct RedWineChildAreaTag                      		
{                                               		
	char	wChildAreaName[REDWINE_CHILDAREA_NAME_LEN];					// �Ӳ�������
};                                              		
                                                		
//��ƾ�ׯ�����б�                              		
struct RedwineManorListTag                      		
{                                               		
	char	wWineryEngName[REDWINE_MANOR_NAME_LEN];						// ��ׯӢ������
	char	wWineryChnName[REDWINE_MANOR_NAME_LEN];						// ��ׯ��������
};

// �������
struct RedwineNameTag
{
	char	wWineEngName[REDWINE_NAME_LEN];								// ���Ӣ������
	char	wWineChnName[REDWINE_NAME_LEN];								// �����������
};

// �������
struct RedwineCapacityTag
{
	char	wCapacity[REDWINE_CAPACITY_LEN];							// ����
};

// ���������������                             		
struct RedWineSaleAreaTag                       		
{                                               		
	char	wSaleAreaName[REDWINE_SALEAREA_LEN];						// �������򻮷�����
};

// ��ƻ�����Ϣ
struct RedwineBaseInforTag
{
	WORD	nRedwineName;												// �������(RedwineNameTag)	
	BYTE	wRedwineType;												// �������(1:�Ҿ�  2:���Ѿ�)	

	BYTE	wArea;														// ��Ʋ�������(������Ϊ-1)
	BYTE	wParentArea;												// ��ƴ��������(������Ϊ-1)
	WORD	wChildArea;													// ����Ӳ�������(������Ϊ-1)
	BYTE	wEightWineriesIdx;											// �˴��ׯ����(������Ϊ-1)
};

// ���Liv�ɷ־Ʊ�ʶ
struct RedwineLivMemEnumTag
{
	WORD	nRedwineBaseInfor;											// ��ƻ�����Ϣ��ʶ
	WORD	wYear;														// ������
	BYTE	nRedwineCapacity;											// �������(RedwineCapacityTag)
	BYTE	wNumber;													// �������
	BYTE	cSaleArea;													// ��������

	BYTE	nParkLRating;												// ������ͷ�
	BYTE	nParkHRating;												// ������߷�
};
                                                		
// ���Liv30�������鱨���б�(���ж�ӦRedwineLivMemEnumTag ���б�)
struct RedwineLatestLivListTag                  		
{       	                                                		
	DWORD	wLastestDate;												// ���±�������
	DWORD	wLowestPriceDate;											// ��ͱ�������
	DWORD	wLowestPrice;												// ��ͱ��� * 100
	DWORD	wAvgPrice;													// ƽ������ * 100
	DWORD	tUnit;														// ���۵�����
                                                		
	DWORD	nPrice;														// ���³ɽ��� * 100
	DWORD	nAmount;													// ���³ɽ���
	DWORD	wAvgAuctionPrice;											// ƽ�������� * 100
	DWORD	wAuctionAmount;												// ��������
		
	DWORD	lobPrice;													// 30ƽ������ * 100
	DWORD	ltbPrice;													// 90ƽ������ * 100
};                                              		
                                                		
// ��������ɷ־Ʊ�ʶ(�б�����ȡ����ȫ������)
struct RedwineAuctionMemEnumTag
{
	WORD	nRedwineBaseInfor;											// ��ƻ�����Ϣ��ʶ
	WORD	wYear;														// ������
	BYTE	nRedwineCapacity;											// �������(RedwineCapacityTag)

	BYTE	nParkLRating;												// ������ͷ�
	BYTE	nParkHRating;												// ������߷�
};

// ������۳ɷ־Ʊ�ʶ(�б�����ȡ����ȫ������)
struct RedwineRetailMemEnumTag
{
	WORD	nRedwineBaseInfor;											// ��ƻ�����Ϣ��ʶ
	WORD	wYear;														// ������
	BYTE	nRedwineCapacity;											// �������(RedwineCapacityTag)
	BYTE	wNumber;													// �������

	BYTE	nParkLRating;												// ������ͷ�
	BYTE	nParkHRating;												// ������߷�
};
                                                		
// ��Ƽ۸���ʷ����(������������)                             		
struct RedwinePriceHistDataTag                  		
{                                               		
	DWORD	qTime;														// ��������	
	DWORD	wAvgPrice;													// ���� * 100
	DWORD	wLowestPrice;												// ��ͼ� * 100
	DWORD	wHighestPrice;												// ��߼� * 100
	DWORD	wVolume;													// �ɽ���
	BYTE	cSaleArea;													// ��������

public:
	RedwinePriceHistDataTag & operator = (const RedwinePriceHistDataTag &item)
	{
		qTime = item.qTime;
		wAvgPrice = item.wAvgPrice;
		wLowestPrice = item.wLowestPrice;
		wHighestPrice = item.wHighestPrice;
		wVolume = item.wVolume;
		cSaleArea = item.cSaleArea;
		return *this;
	}
};

// ���Liv�۸�����Ϊ�յ���ʷ����
struct RedwinePriceLivDayHistDataTag
{
	DWORD	qTime;														// ��������	
	DWORD	wAvgPrice;													// ���� * 100
	DWORD	wLowestPrice;												// ��ͼ� * 100
	DWORD	wAvgAuctionPrice;											// ƽ�������� * 100
	DWORD	wVolume;													// �ɽ���

public:
	RedwinePriceLivDayHistDataTag & operator = (const RedwinePriceLivDayHistDataTag &item)
	{
		qTime = item.qTime;
		wAvgPrice = item.wAvgPrice;
		wLowestPrice = item.wLowestPrice;
		wAvgAuctionPrice = item.wAvgAuctionPrice;
		wVolume = item.wVolume;
		return *this;
	}
};

// ���Liv�۸�����Ϊ�µ���ʷ����
struct RedwinePriceLivMonthHistDataTag
{
	DWORD	qTime;														// ��������	
	DWORD	wAvgPrice;													// ���� * 100
	DWORD	wLowestPrice;												// ��ͼ� * 100
	DWORD	wHighestPrice;												// ��߼� * 100
	DWORD	wVolume;													// �ɽ���
};

// �����������Ϊ�յ���ʷ����
struct RedwinePriceAuctionDayHistTag
{
	DWORD	qTime;														// ��������	
	DWORD	wAvgPrice;													// ���� * 100
	DWORD	wLowestPrice;												// ��ͼ� * 100
	DWORD	wHighestPrice;												// ��߼� * 100
	
	DWORD	wVolume;													// �ɽ���
	WORD	wSaleAddr;													// ���۵ص� RedwineSaleAddressTag
	BYTE	wAuctionAddr;												// ����������
};

// ��ƻ��ҵ�λ
struct RedwineMoneyUnitTag
{
	char	wMoneyUnitName[REDWINE_MONEYUNIT_LEN];						// ���ҵ�λ����
};

// ������۵ص�
struct RedwineSaleAddressTag
{
	char	wSaleAddrChnName[REDWINE_SALE_ADDR_LEN];					// ���۵ص���������
	BYTE	nBelongArea;												// ������������

	RedwineSaleAddressTag & operator = (const RedwineSaleAddressTag &item)
	{
		if (NULL != item.wSaleAddrChnName)
		{
			memcpy(wSaleAddrChnName, item.wSaleAddrChnName, REDWINE_SALE_ADDR_LEN);
		}
		nBelongArea = item.nBelongArea;
		return *this;
	}
};

// ���������
struct RedwineAuctionHouseTag
{
	char	wAuctionAddrName[REDWINE_AUCTION_HOUSE_LEN];				// ����������
};

// ����������ճ�
struct RedwineAuctionScheduleTag
{
	DWORD	qTime;														// ������ʱ��
	BYTE	wAuctionHouse;												// ������ 
	char	cAuctionName[REDWINE_AUCTION_NAME_LEN];						// ����������
	char	cAuctionSpecialName[REDWINE_AUCTION_NAME_LEN];				// ����ר������
	WORD	wAddress;													// ������ص� RedwineSaleAddressTag
	DWORD	wAmount;													// �ɽ��� * 100
	BYTE	wMoneyUnit;													// ���ҵ�λ
	BYTE	wAuctionType;												// ��������(1:�Ҿ�  2:���Ѿ�)

};

// ������������ѯ�ؼ���(�ص�+��������)���
struct RedwineAuctionResSearchKeyTag
{
	WORD	wSaleAddr;													// ���۵ص� RedwineSaleAddressTag
	BYTE	wAuctionAddr;												// ����������
};

// ������������ѯ������ݾƱ�ʶ�б�
struct RedwineAuctionResSearchMemEnumTag
{
	WORD	nRedwineBaseInfor;											// ��ƻ�����Ϣ��ʶ
	WORD	wYear;														// ������
	BYTE	nRedwineCapacity;											// �������(RedwineCapacityTag -1:����)
};

// ������������ѯ���ݹ���
struct RedwineAuctionResSearchDataTag
{
	DWORD	qTime;														// ������ʱ��
	WORD	nRedwineMark;												// ��Ʊ�ʶ��Ϣ(��Ӧ��RedwineAuctionResSearchMemEnumTag������)
	BYTE	wMoneyUnit;													// ���ҵ�λ
	DWORD	wAmount;													// �ɽ��� * 100
	DWORD	wSingleUnitPrice;											// ÿƿ�ɽ��� * 100
	DWORD	wLValuationPrice;											// ��͹��� * 100
	DWORD	wHValuationPrice;											// ��߹��� * 100
	WORD	nAuctionQuantity;											// ����ƿ��
};

// ��һƷ�ֺ������������ݹ���
struct RedwineAuctBySingleTypeDataTag
{
	WORD	nAuctAddKeyIndex;											// �����ص�ؼ�������(��ӦRedwineAuctionResSearchKeyTag����)
	WORD	nAuctionQuantity;											// ����ƿ��
	DWORD	wAmount;													// �ɽ��� * 100
	BYTE	wMoneyUnit;													// ���ҵ�λ	
	DWORD	qTime;														// ����ʱ��
};

// ����������±���ͳ��
struct RedwineRetailLatestPriceTag
{
	DWORD	qTime;														// ����ʱ��
	DWORD	wQuotePrice;												// ���� * 100
	BYTE	wMoneyUnit;													// ���ҵ�λ
};

#pragma pack()

#endif 
