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
#define  WM_WTRESPONSEMESSAGE       (WM_USER+225)//委托响应，WPARAM是请求时候的参数WTASKINFO；LAPARAM是从委托服务器返回的参数，如果为NULL表示委托失败
#define  WM_WTRESPONSEMESSAGE_NETERROR       (WM_USER+226)//网络出错
//#define  WM_WTRESPONSEMESSAGE_TIMEOUT       (WM_USER+227)//超时

extern 	char*  MapMemFile (const char* szFileName, DWORD len);

#pragma pack(1)

enum
{
	CLIENTDATABUFLEN = 1024,
	MAXMULDATABUFFNUM = 32,
	MAXSENDINFONUM = MAXMULDATABUFFNUM/3,	//一次最多发送经济或者股票信息数
};


//64位整数
union MAKELONGLONGST
{
	unsigned long long	m_llValue;
	struct
	{
		unsigned int	m_dwLow;
		unsigned int	m_dwHigh;
	};
};

//32位整数
union MAKEDWORDST
{
	unsigned int	m_dwValue;
	struct
	{
		unsigned short	m_wLow;
		unsigned short	m_wHigh;
	};
};

//16位整数
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
	char*  m_pBlockBuff;//内存缓冲区
	DWORD  m_nFreePos;//缓冲区空闲的位置
	DWORD  m_nFreeLen;//空闲的长度, m_nFreePos+m_nFreeLen = 该缓冲区的总长度
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
	WORD	m_nRecvLen;//表示接收的数据长度或者是未处理的请求数据
	WORD	m_nSendBufLen;//发送缓冲区的长度
	char    m_pRecvBuff[CLIENTDATABUFLEN];
	time_t  m_tAliveTime;

	//BYTE    m_nAskTypeState;//用来记录客户的一些标记和状态
	BYTE    m_nDataBuffNum;//表示下来发送缓冲区一共有多少个数据，如果关闭就把这个置0
	BYTE    m_nDataBuffPos;//表示下次发送的时候从那个缓冲区开始发送数据

	BYTE    m_nSaveBufNum;//下列存储的要删除的缓冲区数目
	const char*	m_pSaveBuff[MAXMULDATABUFFNUM];
	BUFFANDLEN	m_pMulSendBuff[MAXMULDATABUFFNUM];
};

//////////////////////////////////////////////////////////////
//资讯接口的定义

//常量
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

	MAXFILEBUFFLEN = 10000000,	//最大缓冲区的长度

	MULROLLCONTENTLEN = 300,

	BROADCASTMAXLEN = 1002,
	MAXROLLWMLLEN = 8192,

	MAXCRMPOPUPLEN = 2048,	//crm弹出资讯最大长度

	//图片类型
	PICTURETYPE_PNG = 1,
	PICTURETYPE_JPG = 2,
	//
	PREMAINXWLINFONUM = 12,
	MAINXWLINFONUM = 13,
	BULLETINDATANUM = 2,
	FACENOTICENUM = 2,	//界面提示性文字数目.
	TRANSWMLNUM = 2,
};

//数据头
struct	ZXCMDHEAD
{
	WORD	m_wCmdType;	//数据类型
	DWORD	m_nAttr;	//数据属性；
	DWORD	m_nLen;		//后面数据长度
	DWORD	m_nExData;	//附加数据，响应方通常原样返回；特殊用途除外
};
//命令类型
enum
{
	ZXCMDXC_ALIVE	=	0,
	ZXCMDXC_JIEPAN	=	1,

	ZXCMD_ALIVE	=	0x8080,
	ZXCMD_ECONOMIC,
	ZXCMD_MULROLLTXT,	//多滚动资讯
	ZXCMD_MAINXWL,		//主页
	ZXCMD_RMBRATE,		//人民币汇率
	ZXCMD_ROLLTXT,		//滚动广告
	ZXCMD_PICTURE,		//广告图片
	ZXCMD_STKINFO,		//股票相关信息，其中也包含实时解盘
	ZXCMD_FILE,			//传输文件
	ZXCMD_BROADCAST,	//传输公告

	ZXCMD_ROLLWML,		//滚动wml信息
	ZXCMD_FACENOTICE,	//界面提示信息，= 11 b

	ZXCMD_STKPOOLCFG,	//股票池配置信息
	ZXCMD_STKPOOLSTK,	//股票池信息

	ZXCMD_TRANSFERWML,	//中转的wml
	//////
	ZXCMD_CRMPOPUP,		//券商弹出资讯
	ZXCMD_MULPICTURE,	//多个分辨率广告图片

	ZXCMD_WTADDRINFO,	//请求委托信息

	////////红酒接口数据
	ZXCMD_REDWINE_INDEX_LIST,				// 请求红酒指数列表 一次请求，返回结构为 RedwineIndexListTag
	ZXCMD_REDWINE_INDEX_HIS_DATA,			// 请求红酒指数历史数据, 分次请求，返回结构为 RedwineIndexHistTag
	ZXCMD_REDWINE_INDEX_ELEMENT,			// 请求红酒指数成分股数据, 分次请求，返回 sizeof(WORD)*N

	ZXCMD_REDWINE_CRITIC_LIST,				// 请求红酒酒评家及机构数据，一次请求，返回 RedWineCriticTag
	ZXCMD_REDWINE_NAME_LIST_DATA,			// 请求红酒名称列表，一次请求，返回 RedwineNameTag
	ZXCMD_REDWINE_AREACLASSIFIED_LIST,		// 请求红酒产区分类数据, 一次请求，返回 RedWineAreaClassifiedTag
	ZXCMD_REDWINE_PARENTAREA_LIST,			// 请求红酒产大产区数据，一次请求，返回 RedWineParentAreaTag
	ZXCMD_REDWINE_CHILDAREA_LIST,			// 请求红酒产子产区数据，一次请求，返回 RedWineChildAreaTag
	ZXCMD_REDWINE_EIGHTWINERIES_LIST,		// 请求红酒八大酒庄数据，一次次请求，返回 RedwineManorListTag
	ZXCMD_REDWINE_SALEAREA,					// 请求红酒销售区域，一次请求，返回 RedWineSaleAreaTag
	ZXCMD_REDWINE_CAPACITY_LIST_DATA,		// 请求红酒容量列表，一次请求，返回 RedwineCapacityTag
	ZXCMD_REDWINE_MONEY_UNIT_DATA,			// 请求红酒货币单位数据，一次请求，返回 RedwineMoneyUnitTag

	ZXCMD_REDWINE_BASE_INFOR_DATA,			// 请求全部红酒基本信息，一次请求，返回 RedwineBaseInforTag
	ZXCMD_REDWINE_CLASSIFIED_NAME_LIST,		// 请求红酒分类的名称信息索引(m_nExData标识证券索引部分用以标识名称分类类型0:Liv 1:拍卖 2:零售)，分次请求，返回sizeof(WORD)*N

	ZXCMD_REDWINE_LIV_MEMBER_DATA,			// 请求全部红酒Liv市场成分酒，一次请求，返回 RedwineLivMemEnumTag
	ZXCMD_REDWINE_LIV30_LIST,				// 请求红酒Liv30最新报价列表, 一次请求，返回结构为 RedwineLatestLivListTag
 	ZXCMD_REDWINE_LIV_DAY_DATA,				// 请求红酒Liv周期为天统计数据，分次请求，返回 RedwinePriceLivDayHistDataTag
 	ZXCMD_REDWINE_LIV_MONTH_DATA,			// 请求红酒Liv周期为月统计数据，分次请求，返回 RedwinePriceLivMonthHistDataTag

	ZXCMD_REDWINE_SALE_ADDRESS_DATA,		// 请求红酒销售地点数据，一次请求，返回 RedwineSaleAddressTag

	ZXCMD_REDWINE_AUCTION_MEMBER_DATA,		// 请求全部红酒拍卖市场成分酒，一次请求，返回 RedwineAuctionMemEnumTag
	ZXCMD_REDWINE_AUCTION_DAY_DATA,			// 请求红酒拍卖日数据, 分次请求，返回 RedwinePriceAuctionDayHistTag
	ZXCMD_REDWINE_AUCTION_MONTH_DATA,		// 请求红酒拍卖月数据, 分次请求，返回 RedwinePriceHistDataTag

	ZXCMD_REDWINE_AUCTION_HOUSE_DATA,		// 请求红酒拍卖行数据，一次请求，返回 RedwineAuctionHouseTag
	ZXCMD_REDWINE_AUCTION_SCHEDULE_DATA,	// 请求红酒拍卖会日程，一次请求，返回 RedwineAuctionScheduleTag
		
	ZXCMD_REDWINE_RETAIL_MEMBER_DATA,		// 请求全部红酒零售市场成分酒，一次请求，返回 RedwineRetailMemEnumTag
	ZXCMD_REDWINE_RETAIL_MONTH_DATA,		// 请求红酒零售月数据, 分次请求，返回 RedwinePriceHistDataTag
	ZXCMD_REDWINE_RETAIL_LATEST_PRICE,		// 请求红酒零售最近报价数据，一次请求，返回 RedwineRetailLatestPriceTag

	ZXCMD_REDWINE_AUCTION_SEARCH_KEY_DATA,	// 请求红酒拍卖结果查询关键字(地点结构)组合, 一次请求, 返回 RedwineAuctionResSearchKeyTag
	ZXCMD_REDWINE_AUCTION_SEARCH_MEM_DATA,	// 请求红酒拍卖结果查询结果数据酒标识列表, 一次请求, 返回 RedwineAuctionResSearchMemEnumTag
	ZXCMD_REDWINE_AUCTION_SEARCH_HIS_DATA,	// 请求红酒拍卖结果查询历史数据(按地点结构), 分次请求(顺序相应于RedwineAuctionResSearchKeyTag列表), 返回 RedwineAuctionResSearchDataTag
	ZXCMD_REDWINE_AUCTION_SINGLE_TYPE_DATA,	// 请求红酒拍卖结果历史数据(按酒种类分类), 分次请求(顺序相应于RedwineAuctionResSearchMemEnumTag列表)，返回 RedwineAuctBySingleTypeDataTag
};

//ZXCMDXC_ALIVE，m_nLen=0，客户端发送，服务器原样返回；用来表示连接是否存在，10秒钟客户端要发一次
//ZXCMDXC_JIEPAN，股票资讯，请求：m_nAttr=0,m_nExData=id，最初为0；返回：0～N个该消息返回，m_nAttr=0，m_nExData=服务器目前实时解盘id，下次请求的时候把最后接收到的该id带上


struct STKZXINFO //股票资讯信息结构
{
	char	m_type;	//类型
	time_t	m_time;	//数据时间
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


//ZXCMD_ALIVE，m_nLen=0，客户端发送，服务器原样返回；用来表示连接是否存在，10秒钟客户端要发一次

//ZXCMD_ECONOMIC，请求：m_nAttr=0,m_nExData=id(用数目表示)；返回：m_nAttr=表示后面还要多少该类型数据，0表示该类型传输结束；m_nExData=服务器目前id
struct	ECONOMICZX
{
	char	m_type;	//类型
	time_t	m_time;	//数据时间
	WORD	m_nTitleLen;
	WORD	m_nContentLen;
	char	m_pTitle[0];
	//char	content;//后面是数据
};

//ZXCMD_MULROLLTXT，多滚动文本；m_nAttr表示触屏或者非触屏；m_nExData表示滚动文本的crc；ZXCMDHEAD+char*n

//ZXCMD_MAINXWL，主页；m_nAttr表示主页字体；m_nExData表示主页的crc；ZXCMDHEAD+char*n

//ZXCMD_RMBRATE，人民币汇率；m_nExData表示crc；ZXCMDHEAD+RMBRATEINFO*n

struct RMBRATEINFO
{
	DWORD m_nDate;
	DWORD m_nRate;
	BYTE  m_nPriceDigit;
	char  m_szName[LENSTNAME];
	char  m_szUtf8Name[sizeof(WORD)+LENSTNAME];
};

//ZXCMD_ROLLTXT，滚动广告；返回：m_nAttr=表示后面还要多少该类型数据，0表示该类型传输结束，m_nExData表示最新的滚动广告总数；ZXCMDHEAD+id+char*n

//ZXCMD_PICTURE，广告图片；m_nExData表示最新的广告图片id；ZXCMDHEAD+char*n

//ZXCMD_STKINFO，股票资讯，请求：m_nAttr=0,m_nExData=id(用数目表示)；返回：m_nAttr=这次要发送的STKZXINFO数据数目；m_nExData=服务器目前id

//ZXCMD_ROLLWML，请求滚动wml；m_nExData已经获得的wml的crc；ZXCMDHEAD+char*n

//ZXCMD_FACENOTICE，请求界面提示文字；m_nExData已经获得的提示文字的crc；ZXCMDHEAD+char*n

//ZXCMD_STKPOOLCFG，请求股票池配置信息；m_nExData已经获得的股票池配置信息的crc；ZXCMDHEAD+char*n

//ZXCMD_STKPOOLSTK，请求股票池信息；m_nExData已经获得的股票池配置信息的crc，m_nAttr是股票池id；ZXCMDHEAD+char*n

//ZXCMD_TRANSFERWML, 中转的wml；m_nAttr表示请求的id， m_nExData已经获得的wml的crc；ZXCMDHEAD+char*n

//ZXCMD_MULPICTURE, 请求不同分辨率的图片；m_nAttr表示分辨率,0表示请求所有图片的基本信息，m_nExData表示图片类型,m_nLen=0;返回m_nLen=sizeof(MULPICTUREINFO)+char*N

//ZXCMD_WTADDRINFO, m_nAttr索引,m_nExData表示crc，m_nLen=0;返回m_nAttr索引,m_nExData>>16表示数目，m_nExData&0xFFFF表示crc，m_nLen=sizeof(WTADDRINFO)+char*N，如果m_nLen=0表示没有数据或者相同

//AH股对照存储结构结构
struct AHSTOCKINFO
{
	char  szHCode[sizeof(WORD)+LENSTLABEL];//包括市场代码
	char  szACode[sizeof(WORD)+LENSTLABEL];//包括市场代码
};

//H股分类文件结构
enum
{
	BLUESTK,	//蓝筹股
	REDSTK,		//红筹股
	GQSTK,		//国企股
	UNKNOWSTK,		//未知类型
};

struct	HKSTKCLASS
{
	char	m_type;
	char	m_szLabel[LENSTLABEL];
};

//ZXCMD_FILE，传输文件；请求：m_nAttr表示文件索引， m_nExData表示该文件crc；返回：m_nAttr高2字节表示服务器端文件数，低位2字节表示文件索引，m_nExData表示该文件crc；ZXFILE+char*N
struct ZXFILE
{
	char	m_szFileName[32];
	char	m_pContent[0];
};

/////////////

#define DATASOURCEIPLEN  31
struct DATASOURCESTATE
{
	DWORD           m_nPreHQPackNo; //包号
	DWORD           m_nHQPackNo;    //包号
	time_t          m_nRecvKeepALive; // 在等待 KEEPRCVALIVEMINSEC
	time_t          m_nPreRecvDynaTime; // 上次接收到动态数据的时间
	int             m_nIPIndex; //连接使用的配置文件中ip索引，这个只对主动连接有用
	int             m_SocketState;//数据源的连接状态
	char            m_szDataSourceIp[DATASOURCEIPLEN+1];//数据源的IP
	unsigned short  m_usDataPort;//数据源的状态
	unsigned short  m_nLevel;//
};


struct MULPICTUREINFO
{
	DWORD	m_len;			//图片长度
	union
	{
		DWORD	m_nRes;			//宽高分辨率
		struct
		{
			WORD	m_nHigthRes;	//高分辨率
			WORD	m_nWidthRes;	//宽分辨率
		};
	};
	DWORD	m_nCRC;			//crc
	unsigned char	m_nPitType;		//图片类型
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
	WORD	m_nLen;//下面从m_nCrc开始的长度
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
	REDWINE_INDEX_NAME_LEN = 32,						// 红酒指数名称长度
	REDWINE_CRITIC_NAME_LEN = 56,						// 红酒酒评家名称长度
	REDWINE_AREACLASSIFIED_NAME_LEN = 32,				// 红酒产区分类名称长度
	REDWINE_PARENTAREA_NAME_LEN = 32,					// 红酒大产区名称长度
	REDWINE_CHILDAREA_NAME_LEN = 32,					// 红酒子产区名称长度
	REDWINE_MANOR_NAME_LEN = 32,						// 红酒酒庄名称长度
	REDWINE_NAME_LEN = 56,								// 红酒名称长度
	REDWINE_CAPACITY_LEN = 16,							// 红酒容量长度
	REDWINE_SALEAREA_LEN = 32,							// 红酒销售区域划分长度
	REDWINE_MONEYUNIT_LEN = 16,							// 红酒货币单位长度
	REDWINE_SALE_ADDR_LEN = 24,							// 红酒销售地点长度
	REDWINE_AUCTION_HOUSE_LEN = 16,						// 红酒拍卖行长度
	REDWINE_AUCTION_NAME_LEN = 128,						// 红酒拍卖会名称长度
};

////////////////////////////////////////// 红酒接口结构定义 //////////////////////////////
//	分次请求规则定义：
//				ZXCMDHEAD命令头
//				请求：m_nAttr=表示请求数目, m_nExData请求的起始位置(从0开始标号)，m_nLen=0; 
//				返回：m_nAttr表示该类数据总个数,为0表示最后一个数据;
//					  m_nExData最高BYTE表示数据的货币单位(一组数据存在同一货币单位时应用，否则未定义)，m_nExData除最高BYTE外其余BYTE表示证券索引;
//					  m_nLen=sizeof(返回结构)*N;
//
// ZXCMD_REDWINE_INDEX_LIST,				// 请求红酒指数列表 一次请求，返回结构为 RedwineIndexListTag
// ZXCMD_REDWINE_INDEX_HIS_DATA,			// 请求红酒指数历史数据, 分次请求，返回结构为 RedwineIndexHistTag
// ZXCMD_REDWINE_INDEX_ELEMENT,				// 请求红酒指数成分股数据, 分次请求，返回 sizeof(WORD)*N
// 
// ZXCMD_REDWINE_CRITIC_LIST,				// 请求红酒酒评家及机构数据，一次请求，返回 RedWineCriticTag
// ZXCMD_REDWINE_NAME_LIST_DATA,			// 请求红酒名称列表，一次请求，返回 RedwineNameTag
// ZXCMD_REDWINE_AREACLASSIFIED_LIST,		// 请求红酒产区分类数据, 一次请求，返回 RedWineAreaClassifiedTag
// ZXCMD_REDWINE_PARENTAREA_LIST,			// 请求红酒产大产区数据，一次请求，返回 RedWineParentAreaTag
// ZXCMD_REDWINE_CHILDAREA_LIST,			// 请求红酒产子产区数据，一次请求，返回 RedWineChildAreaTag
// ZXCMD_REDWINE_EIGHTWINERIES_LIST,		// 请求红酒八大酒庄数据，一次次请求，返回 RedwineManorListTag
// ZXCMD_REDWINE_SALEAREA,					// 请求红酒销售区域，一次请求，返回 RedWineSaleAreaTag
// ZXCMD_REDWINE_CAPACITY_LIST_DATA,		// 请求红酒容量列表，一次请求，返回 RedwineCapacityTag
// ZXCMD_REDWINE_MONEY_UNIT_DATA,			// 请求红酒货币单位数据，一次请求，返回 RedwineMoneyUnitTag
// 
// ZXCMD_REDWINE_BASE_INFOR_DATA,			// 请求全部红酒基本信息，一次请求，返回 RedwineBaseInforTag
// ZXCMD_REDWINE_CLASSIFIED_NAME_LIST,		// 请求红酒分类的名称信息索引(m_nExData标识证券索引部分用以标识名称分类类型0:Liv 1:拍卖 2:零售)，分次请求，返回sizeof(WORD)*N
// 
// ZXCMD_REDWINE_LIV_MEMBER_DATA,			// 请求全部红酒Liv市场成分酒，一次请求，返回 RedwineLivMemEnumTag
// ZXCMD_REDWINE_LIV30_LIST,				// 请求红酒Liv30最新报价列表, 一次请求，返回结构为 RedwineLatestLivListTag
// ZXCMD_REDWINE_LIV_DAY_DATA,				// 请求红酒Liv周期为天统计数据，分次请求，返回 RedwinePriceLivDayHistDataTag
// ZXCMD_REDWINE_LIV_MONTH_DATA,			// 请求红酒Liv周期为月统计数据，分次请求，返回 RedwinePriceLivMonthHistDataTag
// 
// ZXCMD_REDWINE_SALE_ADDRESS_DATA,			// 请求红酒销售地点数据，一次请求，返回 RedwineSaleAddressTag
// 
// ZXCMD_REDWINE_AUCTION_MEMBER_DATA,		// 请求全部红酒拍卖市场成分酒，一次请求，返回 RedwineAuctionMemEnumTag
// ZXCMD_REDWINE_AUCTION_DAY_DATA,			// 请求红酒拍卖日数据, 分次请求，返回 RedwinePriceAuctionDayHistTag
// ZXCMD_REDWINE_AUCTION_MONTH_DATA,		// 请求红酒拍卖月数据, 分次请求，返回 RedwinePriceHistDataTag
// 
// ZXCMD_REDWINE_AUCTION_HOUSE_DATA,		// 请求红酒拍卖行数据，一次请求，返回 RedwineAuctionHouseTag
// ZXCMD_REDWINE_AUCTION_SCHEDULE_DATA,		// 请求红酒拍卖会日程，一次请求，返回 RedwineAuctionScheduleTag
// 	
// ZXCMD_REDWINE_RETAIL_MEMBER_DATA,		// 请求全部红酒零售市场成分酒，一次请求，返回 RedwineRetailMemEnumTag
// ZXCMD_REDWINE_RETAIL_MONTH_DATA,			// 请求红酒零售月数据, 分次请求，返回 RedwinePriceHistDataTag
// ZXCMD_REDWINE_RETAIL_LATEST_PRICE,		// 请求红酒零售最近报价数据，一次请求，返回 RedwineRetailLatestPriceTag
// 
// ZXCMD_REDWINE_AUCTION_SEARCH_KEY_DATA,	// 请求红酒拍卖结果查询关键字(地点结构)组合, 一次请求, 返回 RedwineAuctionResSearchKeyTag
// ZXCMD_REDWINE_AUCTION_SEARCH_MEM_DATA,	// 请求红酒拍卖结果查询结果数据酒标识列表, 一次请求, 返回 RedwineAuctionResSearchMemEnumTag
// ZXCMD_REDWINE_AUCTION_SEARCH_HIS_DATA,	// 请求红酒拍卖结果查询历史数据, 分次请求(顺序相应于RedwineAuctionResSearchKeyTag列表), 返回 RedwineAuctionResSearchDataTag
// ZXCMD_REDWINE_AUCTION_SINGLE_TYPE_DATA,	// 请求红酒拍卖结果历史数据(按酒种类分类), 分次请求(顺序相应于RedwineAuctionResSearchMemEnumTag列表)，返回 RedwineAuctBySingleTypeDataTag
// 
/////////////////////////////////////////////////////////////////////////////////////////

//红酒指数列表
struct RedwineIndexListTag
{
	char	wIndexCode[REDWINE_INDEX_NAME_LEN];							// 红酒指数名称代码
	char	wIndexChnName[REDWINE_INDEX_NAME_LEN];						// 红酒指数中文名称
};                                              		
                                                		
// 红酒指数历史数据                             		
struct RedwineIndexHistTag
{                                               		
	DWORD	m_nDate;                            		
	DWORD	m_nValue; // *100
};                                              		
                                                		
// 酒评家及机构数据                             		
struct RedWineCriticTag                         		
{                                               		
	char	wCriticName[REDWINE_CRITIC_NAME_LEN];						// 酒评家名称
};                                              		
                                                		
// 红酒产区分类数据                             		
struct RedWineAreaClassifiedTag                 		
{                                               		
	char	wClassifiedName[REDWINE_AREACLASSIFIED_NAME_LEN];			// 产区分类名称
};                                              		
                                                		
// 红酒大产区数据                               		
struct RedWineParentAreaTag                     		
{                                               		
	char	wParentAreaName[REDWINE_PARENTAREA_NAME_LEN];				// 大产区名称
};                                              		
                                                		
// 红酒子产区数据                               		
struct RedWineChildAreaTag                      		
{                                               		
	char	wChildAreaName[REDWINE_CHILDAREA_NAME_LEN];					// 子产区名称
};                                              		
                                                		
//红酒酒庄分类列表                              		
struct RedwineManorListTag                      		
{                                               		
	char	wWineryEngName[REDWINE_MANOR_NAME_LEN];						// 酒庄英文名称
	char	wWineryChnName[REDWINE_MANOR_NAME_LEN];						// 酒庄中文名称
};

// 红酒名称
struct RedwineNameTag
{
	char	wWineEngName[REDWINE_NAME_LEN];								// 红酒英文名称
	char	wWineChnName[REDWINE_NAME_LEN];								// 红酒中文名称
};

// 红酒容量
struct RedwineCapacityTag
{
	char	wCapacity[REDWINE_CAPACITY_LEN];							// 容量
};

// 红酒销售区域数据                             		
struct RedWineSaleAreaTag                       		
{                                               		
	char	wSaleAreaName[REDWINE_SALEAREA_LEN];						// 销售区域划分名称
};

// 红酒基本信息
struct RedwineBaseInforTag
{
	WORD	nRedwineName;												// 红酒名称(RedwineNameTag)	
	BYTE	wRedwineType;												// 红酒种类(1:烈酒  2:葡萄酒)	

	BYTE	wArea;														// 红酒产区分类(不属于为-1)
	BYTE	wParentArea;												// 红酒大产区分类(不属于为-1)
	WORD	wChildArea;													// 红酒子产区分类(不属于为-1)
	BYTE	wEightWineriesIdx;											// 八大酒庄索引(不属于为-1)
};

// 红酒Liv成分酒标识
struct RedwineLivMemEnumTag
{
	WORD	nRedwineBaseInfor;											// 红酒基本信息标识
	WORD	wYear;														// 红酒年份
	BYTE	nRedwineCapacity;											// 红酒容量(RedwineCapacityTag)
	BYTE	wNumber;													// 规格数量
	BYTE	cSaleArea;													// 销售区域

	BYTE	nParkLRating;												// 帕克最低分
	BYTE	nParkHRating;												// 帕克最高分
};
                                                		
// 红酒Liv30最新行情报价列表(序列对应RedwineLivMemEnumTag 的列表)
struct RedwineLatestLivListTag                  		
{       	                                                		
	DWORD	wLastestDate;												// 最新报价日期
	DWORD	wLowestPriceDate;											// 最低报价日期
	DWORD	wLowestPrice;												// 最低报价 * 100
	DWORD	wAvgPrice;													// 平均报价 * 100
	DWORD	tUnit;														// 报价单总量
                                                		
	DWORD	nPrice;														// 最新成交价 * 100
	DWORD	nAmount;													// 最新成交量
	DWORD	wAvgAuctionPrice;											// 平均拍卖价 * 100
	DWORD	wAuctionAmount;												// 拍卖数量
		
	DWORD	lobPrice;													// 30平均报价 * 100
	DWORD	ltbPrice;													// 90平均报价 * 100
};                                              		
                                                		
// 红酒拍卖成分酒标识(列表用于取拍卖全部数据)
struct RedwineAuctionMemEnumTag
{
	WORD	nRedwineBaseInfor;											// 红酒基本信息标识
	WORD	wYear;														// 红酒年份
	BYTE	nRedwineCapacity;											// 红酒容量(RedwineCapacityTag)

	BYTE	nParkLRating;												// 帕克最低分
	BYTE	nParkHRating;												// 帕克最高分
};

// 红酒零售成分酒标识(列表用于取零售全部数据)
struct RedwineRetailMemEnumTag
{
	WORD	nRedwineBaseInfor;											// 红酒基本信息标识
	WORD	wYear;														// 红酒年份
	BYTE	nRedwineCapacity;											// 红酒容量(RedwineCapacityTag)
	BYTE	wNumber;													// 规格数量

	BYTE	nParkLRating;												// 帕克最低分
	BYTE	nParkHRating;												// 帕克最高分
};
                                                		
// 红酒价格历史数据(用于拍卖零售)                             		
struct RedwinePriceHistDataTag                  		
{                                               		
	DWORD	qTime;														// 报价日期	
	DWORD	wAvgPrice;													// 均价 * 100
	DWORD	wLowestPrice;												// 最低价 * 100
	DWORD	wHighestPrice;												// 最高价 * 100
	DWORD	wVolume;													// 成交量
	BYTE	cSaleArea;													// 销售区域

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

// 红酒Liv价格周期为日的历史数据
struct RedwinePriceLivDayHistDataTag
{
	DWORD	qTime;														// 报价日期	
	DWORD	wAvgPrice;													// 均价 * 100
	DWORD	wLowestPrice;												// 最低价 * 100
	DWORD	wAvgAuctionPrice;											// 平均拍卖价 * 100
	DWORD	wVolume;													// 成交量

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

// 红酒Liv价格周期为月的历史数据
struct RedwinePriceLivMonthHistDataTag
{
	DWORD	qTime;														// 报价日期	
	DWORD	wAvgPrice;													// 均价 * 100
	DWORD	wLowestPrice;												// 最低价 * 100
	DWORD	wHighestPrice;												// 最高价 * 100
	DWORD	wVolume;													// 成交量
};

// 红酒拍卖周期为日的历史数据
struct RedwinePriceAuctionDayHistTag
{
	DWORD	qTime;														// 报价日期	
	DWORD	wAvgPrice;													// 均价 * 100
	DWORD	wLowestPrice;												// 最低价 * 100
	DWORD	wHighestPrice;												// 最高价 * 100
	
	DWORD	wVolume;													// 成交量
	WORD	wSaleAddr;													// 销售地点 RedwineSaleAddressTag
	BYTE	wAuctionAddr;												// 所属拍卖行
};

// 红酒货币单位
struct RedwineMoneyUnitTag
{
	char	wMoneyUnitName[REDWINE_MONEYUNIT_LEN];						// 货币单位名称
};

// 红酒销售地点
struct RedwineSaleAddressTag
{
	char	wSaleAddrChnName[REDWINE_SALE_ADDR_LEN];					// 销售地点中文名称
	BYTE	nBelongArea;												// 所属销售区域

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

// 红酒拍卖行
struct RedwineAuctionHouseTag
{
	char	wAuctionAddrName[REDWINE_AUCTION_HOUSE_LEN];				// 拍卖行名称
};

// 红酒拍卖会日程
struct RedwineAuctionScheduleTag
{
	DWORD	qTime;														// 拍卖会时间
	BYTE	wAuctionHouse;												// 拍卖行 
	char	cAuctionName[REDWINE_AUCTION_NAME_LEN];						// 拍卖会名称
	char	cAuctionSpecialName[REDWINE_AUCTION_NAME_LEN];				// 拍卖专场名称
	WORD	wAddress;													// 拍卖会地点 RedwineSaleAddressTag
	DWORD	wAmount;													// 成交额 * 100
	BYTE	wMoneyUnit;													// 货币单位
	BYTE	wAuctionType;												// 拍卖类型(1:烈酒  2:葡萄酒)

};

// 红酒拍卖结果查询关键字(地点+拍卖机构)组合
struct RedwineAuctionResSearchKeyTag
{
	WORD	wSaleAddr;													// 销售地点 RedwineSaleAddressTag
	BYTE	wAuctionAddr;												// 所属拍卖行
};

// 红酒拍卖结果查询结果数据酒标识列表
struct RedwineAuctionResSearchMemEnumTag
{
	WORD	nRedwineBaseInfor;											// 红酒基本信息标识
	WORD	wYear;														// 红酒年份
	BYTE	nRedwineCapacity;											// 红酒容量(RedwineCapacityTag -1:其它)
};

// 红酒拍卖结果查询数据归类
struct RedwineAuctionResSearchDataTag
{
	DWORD	qTime;														// 拍卖会时间
	WORD	nRedwineMark;												// 红酒标识信息(相应于RedwineAuctionResSearchMemEnumTag的索引)
	BYTE	wMoneyUnit;													// 货币单位
	DWORD	wAmount;													// 成交额 * 100
	DWORD	wSingleUnitPrice;											// 每瓶成交价 * 100
	DWORD	wLValuationPrice;											// 最低估价 * 100
	DWORD	wHValuationPrice;											// 最高估价 * 100
	WORD	nAuctionQuantity;											// 拍卖瓶数
};

// 单一品种红酒拍卖结果数据归类
struct RedwineAuctBySingleTypeDataTag
{
	WORD	nAuctAddKeyIndex;											// 拍卖地点关键字索引(对应RedwineAuctionResSearchKeyTag数据)
	WORD	nAuctionQuantity;											// 拍卖瓶数
	DWORD	wAmount;													// 成交额 * 100
	BYTE	wMoneyUnit;													// 货币单位	
	DWORD	qTime;														// 拍卖时间
};

// 红酒零售最新报价统计
struct RedwineRetailLatestPriceTag
{
	DWORD	qTime;														// 报价时间
	DWORD	wQuotePrice;												// 报价 * 100
	BYTE	wMoneyUnit;													// 货币单位
};

#pragma pack()

#endif 
